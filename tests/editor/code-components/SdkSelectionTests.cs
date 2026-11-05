using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using PixieEditor;
using PixieEditor.Native;
using PixieEditor.Ui;

internal static class SdkSelectionTests
{
    // Deliberately non-executable payloads exercise C2 metadata/state only, never native admission.
    private sealed class Fixture(string folder, string id = "a", string build = "b", string distribution = "public", string profile = "authoring")
    {
        internal readonly string Root = Path.GetFullPath(folder);
        internal readonly string ProfilePath = $"profiles/{profile}/Release/profile.json";
        internal JsonObject Identity = new()
        {
            ["schemaVersion"] = 1, ["sdkId"] = new string(id[0], 64), ["sdkBuildId"] = new string(build[0], 64),
            ["productVersion"] = "0.3.0.0", ["distributionClass"] = distribution, ["profile"] = profile,
            ["configuration"] = "Release", ["crt"] = "MD", ["platform"] = "windows", ["architecture"] = "x64",
            ["compilerId"] = "MSVC", ["compilerVersion"] = "19.50.00000", ["msvcFullVersion"] = 195000000,
            ["stlVersion"] = 145, ["stlUpdate"] = 202600, ["iteratorDebugLevel"] = 0, ["cppStandard"] = 20,
            ["cApiVersion"] = 28, ["moduleIdentityContract"] = 1, ["exceptions"] = true, ["rtti"] = true,
            ["windowsSdkVersion"] = "10.0.26100.0", ["dependencyInventorySha256"] = new string('c', 64),
            ["cpuFeatures"] = new JsonArray("avx2", "fma", "f16c"),
            ["capabilities"] = new JsonObject { ["moduleLoading"] = true, ["nativeAutomation"] = distribution == "local-private", ["candidateAuthoring"] = true }
        };

        internal void Write()
        {
            Directory.CreateDirectory(Root);
            var runtime = $"profiles/{profile}/Release/bin/pixie.dll";
            var player = $"profiles/{profile}/Release/bin/pixie-player.exe";
            foreach (var path in new[] { runtime, player })
            {
                var absolute = Path.Combine(Root, path); Directory.CreateDirectory(Path.GetDirectoryName(absolute)!);
                File.WriteAllText(absolute, "SYNTHETIC NONEXECUTABLE C2 FIXTURE " + path + id);
            }
            JsonObject Reference(string path) => new() { ["path"] = path, ["sha256"] = EngineSdk.HashFile(Path.Combine(Root, path)) };
            Identity["runtime"] = Reference(runtime); Identity["player"] = Reference(player);
            File.WriteAllText(Path.Combine(Root, ProfilePath), Identity.ToJsonString());
            var files = new JsonArray();
            foreach (var path in new[] { runtime, player, ProfilePath })
                files.Add(new JsonObject { ["path"] = path, ["size"] = new FileInfo(Path.Combine(Root, path)).Length,
                    ["sha256"] = EngineSdk.HashFile(Path.Combine(Root, path)), ["role"] = path.EndsWith(".json") ? "profile" : "runtime" });
            var catalog = new JsonObject
            {
                ["schemaVersion"] = 1, ["sdkId"] = Identity["sdkId"]!.DeepClone(), ["productVersion"] = "0.3.0.0",
                ["distributionClass"] = distribution, ["files"] = files,
                ["profiles"] = new JsonArray(new JsonObject { ["profile"] = profile, ["configuration"] = "Release",
                    ["sdkBuildId"] = Identity["sdkBuildId"]!.DeepClone(), ["manifest"] = ProfilePath,
                    ["manifestSha256"] = EngineSdk.HashFile(Path.Combine(Root, ProfilePath)) })
            };
            File.WriteAllText(Path.Combine(Root, "sdk.json"), catalog.ToJsonString());
        }
        internal void Catalog(Action<JsonObject> change)
        {
            var path = Path.Combine(Root, "sdk.json"); var data = JsonNode.Parse(File.ReadAllText(path))!.AsObject();
            change(data); File.WriteAllText(path, data.ToJsonString());
        }
        internal void Pair()
        {
            const string shipping = "profiles/shipping/Release/";
            const string tool = "tools/export/Release/";
            var shippingBuild = new string('d', 64);
            Catalog(catalog =>
            {
                JsonObject Payload(string path, string contents, string role)
                {
                    var absolute = Path.Combine(Root, path);
                    Directory.CreateDirectory(Path.GetDirectoryName(absolute)!);
                    File.WriteAllText(absolute, contents);
                    var hash = EngineSdk.HashFile(absolute);
                    catalog["files"]!.AsArray().Add(new JsonObject { ["path"] = path, ["size"] = new FileInfo(absolute).Length,
                        ["sha256"] = hash, ["role"] = role });
                    return new() { ["path"] = path, ["sha256"] = hash };
                }
                var paired = Identity.DeepClone().AsObject();
                paired["profile"] = "shipping"; paired["sdkBuildId"] = shippingBuild;
                paired["capabilities"]!["moduleLoading"] = false;
                paired["runtime"] = Payload(shipping + "bin/pixie.dll", "Synthetic shipping runtime", "runtime");
                paired["player"] = Payload(shipping + "bin/pixie-player.exe", "Synthetic shipping player", "player");
                paired["nativeDependencies"] = new JsonArray(
                    Payload(shipping + "bin/glfw3.dll", "Synthetic GLFW", "runtime"),
                    Payload(shipping + "bin/msvcp140_atomic_wait.dll", "Synthetic CRT satellite", "runtime"));
                paired["externalRuntimeDependencies"] = new JsonArray();
                var profile = Payload(shipping + "profile.json", paired.ToJsonString(), "profile");
                catalog["profiles"]!.AsArray().Add(new JsonObject { ["profile"] = "shipping", ["configuration"] = "Release",
                    ["sdkBuildId"] = shippingBuild, ["manifest"] = profile["path"]!.DeepClone(), ["manifestSha256"] = profile["sha256"]!.DeepClone() });
                var export = Payload(tool + "pixie-export.exe", "Synthetic non-executable tool", "tool");
                export["schemaVersion"] = 1; export["toolVersion"] = "1.2.3.4";
                export["sdkId"] = Identity["sdkId"]!.DeepClone(); export["sdkBuildId"] = shippingBuild;
                export["profile"] = "shipping"; export["configuration"] = "Release";
                export["descriptorSchema"] = 1; export["buildProductSchema"] = 2; export["exportManifestSchema"] = 2;
                export["runtime"] = Payload(tool + "pixie.dll", "Synthetic shipping runtime", "tool-dependency");
                var glfw = Payload(tool + "glfw3.dll", "Synthetic GLFW", "tool-dependency"); glfw["kind"] = "native";
                var crt = Payload(tool + "msvcp140_atomic_wait.dll", "Synthetic CRT satellite", "tool-dependency"); crt["kind"] = "crt";
                export["dependencies"] = new JsonArray(glfw, crt);
                catalog["features"] = new JsonObject { ["pairedExport"] = true };
                catalog["tools"] = new JsonObject { ["exporter"] = export };
            });
        }
        internal void Support(bool badComponentVersion = false)
        {
            const string componentHeader = "// Installed header for @PIXIE_COMPONENT@\n";
            const string componentSource = "// Installed source for @PIXIE_COMPONENT@\n";
            Catalog(catalog =>
            {
                var features = catalog["features"] as JsonObject;
                if (features == null) catalog["features"] = features = new JsonObject();
                features["projectGeneration"] = true;
                foreach (var path in new[] { "cmake/PixieAddCode.cmake", "templates/project-code/sdk/CMakeLists.txt.in",
                    "cmake/PixieProject.cmake", "cmake/PixieGame.cmake", "cmake/PixieTargetDefaults.cmake", "cmake/PixieComponentRegistry.cmake",
                    "lib/cmake/PixieEngine/PixieEngineConfig.cmake", "support/player/PlayerMain.cpp", "support/module/GameModuleEntry.cpp",
                    "templates/project-code/versions.json", "templates/project-code/Component.hpp.in", "templates/project-code/Component.cpp.in",
                    "templates/project-code/Project.hpp.in", "templates/project-code/Project.cpp.in",
                    "templates/project-code/Components.hpp.in", "templates/project-code/Components.cpp.in", "templates/project-code/Components.cmake.in" })
                {
                    var absolute = Path.Combine(Root, path);
                    Directory.CreateDirectory(Path.GetDirectoryName(absolute)!);
                    File.WriteAllText(absolute, Path.GetFileName(path) switch
                    {
                        "CMakeLists.txt.in" => "# Synthetic sdk-v1\npixie_game(@PIXIE_ID@ @PIXIE_GENERATED_CANDIDATE_OPTION@)\n# BEGIN PIXIE COMPONENT SOURCES (managed)\npixie_project_components()\n# END PIXIE COMPONENT SOURCES (managed)\n",
                        "Component.hpp.in" => componentHeader,
                        "Component.cpp.in" => componentSource,
                        "versions.json" => JsonSerializer.Serialize(new { schemaVersion = 1, hashPolicy = "utf8-lf",
                            families = new Dictionary<string, object> { ["sdk-v1"] = new { registrySchema = 1, templates = new[] { "Component.hpp.in", "Component.cpp.in" } } },
                            sha256 = new Dictionary<string, string> { ["Component.hpp.in"] = EngineSdk.HashBytes(System.Text.Encoding.UTF8.GetBytes(componentHeader)),
                                ["Component.cpp.in"] = badComponentVersion ? new string('0', 64) : EngineSdk.HashBytes(System.Text.Encoding.UTF8.GetBytes(componentSource)) } }),
                        _ => "Synthetic metadata input: " + path
                    });
                    catalog["files"]!.AsArray().Add(new JsonObject { ["path"] = path, ["size"] = new FileInfo(absolute).Length,
                        ["sha256"] = EngineSdk.HashFile(absolute), ["role"] = "support" });
                }
            });
        }
        internal EngineSdk Load() => EngineSdk.Load(Root, profile);
    }

    internal static int Run(string directory)
    {
        var work = Path.GetFullPath(directory); Directory.CreateDirectory(work);
        if (Environment.GetEnvironmentVariable("PIXIE_M2_TEST_SDK") is { } actual)
            return PrepareActualBuild(work, actual);
        var passed = new List<string>();
        void Check(bool value, string name)
        { if (!value) throw new InvalidOperationException(name); passed.Add(name); Console.WriteLine("PASS " + name); }
        void Refuses(Action action, string name, string? diagnostic = null)
        {
            try { action(); } catch (EngineException error) { Check(diagnostic == null || error.Message.Contains(diagnostic, StringComparison.Ordinal), name); return; }
            throw new InvalidOperationException("Expected refusal: " + name);
        }
        var metadata = typeof(EngineSession).Assembly.GetCustomAttributes(typeof(System.Reflection.AssemblyMetadataAttribute), false)
            .Cast<System.Reflection.AssemblyMetadataAttribute>().ToDictionary(x => x.Key, x => x.Value);
        Check(metadata.GetValueOrDefault("PixieNativeApiVersion") == "28" && metadata.GetValueOrDefault("PixieSdkIdentitySchema") == "1",
            "managed-support-metadata-comes-from-explicit-header");
        var a = new Fixture(Path.Combine(work, "sdk-a")); a.Write(); var sdk = a.Load();
        var b = new Fixture(Path.Combine(work, "sdk-b"), "d", "e"); b.Write(); var sdkB = b.Load();
        Check(sdk.Id != sdkB.Id && sdk.RuntimePath != sdkB.RuntimePath, "two-independent-installed-selections");
        Check(EngineSdk.Override(null, null) == null, "missing-overrides-require-ui");
        Check(EngineSdk.Override(a.Root, b.Root) == (a.Root, "launch"), "launch-wins-over-environment");
        Check(EngineSdk.Override(null, b.Root) == (b.Root, "environment"), "optional-environment-route");
        var invalid = EngineSdk.Override(Path.Combine(work, "absent"), a.Root)!.Value;
        Refuses(() => EngineSdk.Load(invalid.Root), "invalid-explicit-never-falls-through");
        Refuses(() => EngineSdk.Load(a.Root, configuration: "Debug"), "unavailable-native-configuration-refused");
        Refuses(() => EngineSdk.Load(a.Root, expectedId: sdkB.Id), "wrong-expected-sdk-refused");
        Refuses(() => EngineSdk.Load(a.Root, expectedBuildId: sdkB.BuildId), "wrong-restart-build-refused");
        Refuses(() => EngineSdk.Load(a.Root, expectedManifestSha256: sdkB.ManifestSha256), "wrong-restart-manifest-refused");
        Check(PixieEditor.Program.TryParse(["--native-configuration", "Debug", "--sdk", a.Root], out var options) &&
            options.NativeConfiguration == "Debug" && options.SdkRoot == a.Root, "native-options-independent-of-managed-configuration");
        Check(!PixieEditor.Program.TryParse(["--sdk-profile", "shipping"], out _), "shipping-cli-host-refused");
        Check(!PixieEditor.Program.TryParse(["--sdk-id", "invalid"], out _), "malformed-restart-identity-refused");
        void Broken(string name, Action<Fixture> change)
        {
            var fixture = new Fixture(Path.Combine(work, name)); fixture.Write(); change(fixture);
            Refuses(() => fixture.Load(), name);
        }
        Broken("missing-payload", f => File.Delete(Path.Combine(f.Root, "profiles/authoring/Release/bin/pixie.dll")));
        Broken("modified-payload", f => File.AppendAllText(Path.Combine(f.Root, "profiles/authoring/Release/bin/pixie.dll"), "changed"));
        Broken("unlisted-payload", f => File.WriteAllText(Path.Combine(f.Root, "unexpected.dll"), "unexpected"));
        Broken("declared-private-header-in-public-sdk", f =>
        {
            var path = Path.Combine(f.Root, "LocalCommandChannel.hpp"); File.WriteAllText(path, "private source input");
            f.Catalog(x => x["files"]!.AsArray().Add(new JsonObject { ["path"] = "LocalCommandChannel.hpp", ["size"] = new FileInfo(path).Length,
                ["sha256"] = EngineSdk.HashFile(path), ["role"] = "header" }));
        });
        Broken("path-traversal", f => f.Catalog(x => x["files"]![0]!["path"] = "../outside.dll"));
        Broken("absolute-payload-path", f => f.Catalog(x => x["files"]![0]!["path"] = "C:/producer/pixie.dll"));
        Broken("case-collision", f => f.Catalog(x => { var clone = x["files"]![0]!.DeepClone(); clone["path"] = clone["path"]!.GetValue<string>().ToUpperInvariant(); x["files"]!.AsArray().Add(clone); }));
        Broken("mixed-profile-manifest-hash", f => f.Catalog(x => x["profiles"]![0]!["manifestSha256"] = new string('0', 64)));
        Broken("public-private-profile", f => f.Catalog(x => x["profiles"]![0]!["profile"] = "local-development"));
        Broken("duplicate-metadata-property", f =>
        {
            var path = Path.Combine(f.Root, "sdk.json"); var text = File.ReadAllText(path);
            File.WriteAllText(path, "{\"schemaVersion\":1," + text[1..]);
        });
        foreach (var (field, value) in new (string, JsonNode?)[] {
            ("sdkId", JsonValue.Create(new string('f', 64))), ("architecture", JsonValue.Create("arm64")),
            ("cApiVersion", JsonValue.Create(27)), ("crt", JsonValue.Create("MDd")),
            ("msvcFullVersion", JsonValue.Create(1.5)),
            ("cpuFeatures", new JsonArray("fma", "avx2", "f16c")) })
        {
            var f = new Fixture(Path.Combine(work, "identity-" + field)); f.Identity[field] = value; f.Write();
            if (field == "sdkId") f.Catalog(x => x["sdkId"] = sdk.Id);
            Refuses(() => f.Load(), "identity-" + field);
        }
        var privateSdk = new Fixture(Path.Combine(work, "private-sdk"), "f", "a", "local-private", "local-development"); privateSdk.Write();
        Check(privateSdk.Load().CapabilityBits == 7, "explicit-private-profile-has-local-bits");
        Broken("public-native-automation", f => { f.Identity["capabilities"]!["nativeAutomation"] = true; f.Write(); });
        var runtime = JsonNode.Parse(sdk.Identity.GetRawText())!.AsObject();
        runtime["productVersion"] = "99.99.99.99"; runtime["files"] = new JsonArray();
        using (var doc = JsonDocument.Parse(runtime.ToJsonString())) sdk.ValidateRuntime(doc.RootElement, 5);
        Check(true, "runtime-uses-canonical-projection-not-whole-json");
        runtime["sdkBuildId"] = sdkB.BuildId;
        using (var doc = JsonDocument.Parse(runtime.ToJsonString())) Refuses(() => sdk.ValidateRuntime(doc.RootElement, 5), "runtime-build-mismatch-refused");
        using (var doc = JsonDocument.Parse(sdk.Identity.GetRawText())) Refuses(() => sdk.ValidateRuntime(doc.RootElement, 7), "runtime-capability-mismatch-refused");

        var project = Path.Combine(work, "legacy-project"); Directory.CreateDirectory(project);
        var scene = Path.Combine(project, "legacy.scene"); File.WriteAllText(scene, "{\"version\":1,\"opaque\":true}");
        var beforeScene = EngineSdk.HashFile(scene);
        Check(!SdkRequirements.Validate(project, sdk) && !File.Exists(Path.Combine(project, SdkRequirements.RelativePath)), "legacy-open-does-not-adopt");
        SdkRequirements.Adopt(project, sdk);
        Check(SdkRequirements.Validate(project, sdk), "explicit-portable-adoption-validates");
        var requirement = Path.Combine(project, SdkRequirements.RelativePath); var requirementHash = EngineSdk.HashFile(requirement);
        Check(!File.ReadAllText(requirement).Contains(a.Root, StringComparison.Ordinal) && EngineSdk.HashFile(scene) == beforeScene, "adoption-preserves-scene-and-excludes-install-path");
        Refuses(() => SdkRequirements.Validate(project, sdkB), "project-requirement-rejects-other-sdk");
        Refuses(() => SdkRequirements.Adopt(project, sdkB), "existing-requirement-never-overwritten");
        Check(EngineSdk.HashFile(requirement) == requirementHash, "requirement-refusal-preserves-preimage");
        var data = JsonNode.Parse(File.ReadAllText(requirement))!.AsObject(); data["installationPath"] = a.Root;
        File.WriteAllText(requirement, data.ToJsonString());
        Refuses(() => SdkRequirements.Validate(project, sdk), "machine-path-in-project-requirement-refused");

        var settingsPath = Path.Combine(work, "user/settings.json");
        var settings = new EditorSettings { StoragePath = settingsPath, SdkRoot = a.Root, RecentProjects = [project] };
        var screen = new StartScreen(settings);
        var pending = new ProjectChoice(Path.Combine(work, "not-created-project"), true);
        screen.RequestProject(pending);
        Check(screen.SelectedSdk == null && screen.PendingChoice == pending && !Directory.Exists(pending.Folder) && !File.Exists(settingsPath), "ui-pending-create-does-not-write-or-load");
        Refuses(() => screen.ConfirmSdk(Path.Combine(work, "not-an-sdk")), "ui-invalid-choice-refused");
        Check(screen.PendingChoice == pending && !File.Exists(settingsPath), "ui-invalid-choice-keeps-pending-and-settings");
        Check(screen.ConfirmSdk(a.Root) == pending && screen.SelectedSdk?.Id == sdk.Id, "ui-confirm-resumes-pending-choice");
        var restored = EditorSettings.Load(settingsPath);
        Check(restored.SdkRoot == a.Root && restored.RecentProjects.SequenceEqual(new[] { project }), "machine-selection-preserves-recents");
        Check(!Directory.Exists(pending.Folder), "sdk-confirm-does-not-create-project");
        var pinned = new StartScreen(settings, expectedId: sdkB.Id, expectedBuildId: sdkB.BuildId);
        var savedSettingsHash = EngineSdk.HashFile(settingsPath);
        Refuses(() => pinned.ConfirmSdk(a.Root), "wrong-restart-sdk-refused-before-settings-write");
        Check(EngineSdk.HashFile(settingsPath) == savedSettingsHash, "restart-refusal-preserves-ui-preference");
        GameLibrary.SelectSdk(sdk);
        Refuses(() => EngineFolder.Find(), "selected-sdk-forbids-source-checkout-builder");
        Check(GameLibrary.Find(GameLibrary.SceneGame) == sdk.RuntimePath, "no-code-runtime-is-selected-exact-path");
        Refuses(() => GameLibrary.Find("boids"), "named-module-has-no-ancestor-fallback");
        Check(!GameLibrary.IsLoaded, "synthetic-selection-tests-never-load-native-code");
        var editorPath = Path.Combine(work, "Å space", "PixieEditor.exe");
        var projectPath = Path.Combine(work, "Project with spaces Å");
        var action = new[] { "--project", projectPath, "--resume-code", Path.Combine(projectPath, ".local/handoff.json"), "--mode", "Select" };
        var launch = EditorLaunch.Create(sdk, action, 123, editorPath);
        Check(!launch.UseShellExecute && launch.FileName == editorPath && launch.ArgumentList.Take(action.Length).SequenceEqual(action),
            "apphost-preserves-action-and-unicode-tokens");
        Check(PixieEditor.Program.TryParse(launch.ArgumentList.ToArray(), out var child) && child.SdkRoot == sdk.Root && child.SdkId == sdk.Id &&
            child.SdkBuildId == sdk.BuildId && child.SdkManifestSha256 == sdk.ManifestSha256 && child.SdkProfile == sdk.Profile &&
            child.NativeConfiguration == sdk.Configuration && child.After == 123 && child.Project == projectPath,
            "child-retains-complete-immutable-selection-and-reopen-arguments");
        var editorAssembly = Path.Combine(work, "Å space", "PixieEditor.dll");
        var dotnet = EditorLaunch.Create(sdk, action, processPath: Path.Combine(work, "dotnet.exe"), assemblyPath: editorAssembly);
        Check(dotnet.ArgumentList[0] == editorAssembly && dotnet.ArgumentList.Skip(1).Take(action.Length).SequenceEqual(action),
            "dotnet-launch-prefixes-explicit-editor-assembly");
        var sibling = EditorLaunch.SiblingModule(Path.Combine(work, "Games Å", "first-module.dll"), "Other game Å");
        var gameLaunch = EditorLaunch.Create(sdk, ["--game", sibling], processPath: editorPath);
        Check(Path.IsPathFullyQualified(sibling) && PixieEditor.Program.TryParse(gameLaunch.ArgumentList.ToArray(), out var gameOptions) &&
            gameOptions.Game == sibling && gameOptions.SdkBuildId == sdk.BuildId, "sibling-game-uses-explicit-module-and-same-sdk");
        Refuses(() => EditorLaunch.SiblingModule("relative-module.dll", "other"), "sibling-does-not-discover-relative-parent");
        Refuses(() => EditorLaunch.SiblingModule(Path.Combine(work, "first-module.dll"), "../other"), "sibling-path-escape-refused");
        Refuses(() => EditorLaunch.Create(sdk, ["--sdk", b.Root], processPath: editorPath), "action-cannot-override-immutable-selection");

        Check(sdk.Exporter == null && privateSdk.Load().Exporter == null, "unadvertised-public-and-private-sdk-retain-authoring");
        var outPath = Path.Combine(work, "Export output Å");
        File.WriteAllText(Path.Combine(work, "pixie-export.exe"), "Unselected same-name fixture");
        Refuses(() => ProjectExportJob.CreateLaunch(sdk, projectPath, outPath), "unadvertised-sdk-never-discovers-exporter");
        var absentPair = new Fixture(Path.Combine(work, "explicitly-unavailable-export")); absentPair.Write();
        absentPair.Catalog(x => { x["features"] = new JsonObject { ["pairedExport"] = false }; x["tools"] = new JsonObject(); });
        Check(absentPair.Load().Exporter == null, "false-flag-without-tool-is-valid");
        var pair = new Fixture(Path.Combine(work, "Paired SDK Å")); pair.Write(); pair.Pair();
        var pairedSdk = pair.Load();
        var toolPath = Path.Combine(pair.Root, "tools", "export", "Release", "pixie-export.exe");
        Check(pairedSdk.Exporter is { } exportTool && exportTool == new EngineSdk.ExportTool(toolPath, new string('d', 64)) &&
            pairedSdk.Profile == "authoring" && pairedSdk.BuildId != exportTool.ShippingBuildId,
            "paired-tool-binds-shipping-without-changing-authoring-host");
        var exportLaunch = ProjectExportJob.CreateLaunch(pairedSdk, projectPath, outPath);
        Check(!exportLaunch.UseShellExecute && exportLaunch.FileName == toolPath &&
            exportLaunch.ArgumentList.SequenceEqual(new[] { "--sdk", pair.Root, projectPath, outPath }), "export-launch-keeps-exact-root-executable-and-unicode-arguments");
        void BrokenPair(string name, Action<JsonObject> change)
        {
            var fixture = new Fixture(Path.Combine(work, name)); fixture.Write(); fixture.Pair(); fixture.Catalog(change);
            Refuses(() => fixture.Load(), name);
        }
        BrokenPair("export-true-without-entry", x => x["tools"]!.AsObject().Remove("exporter"));
        BrokenPair("export-entry-without-flag", x => x.Remove("features"));
        BrokenPair("export-entry-with-false-flag", x => x["features"]!["pairedExport"] = false);
        BrokenPair("export-null-entry", x => x["tools"]!["exporter"] = null);
        BrokenPair("export-invalid-features-container", x => x["features"] = new JsonArray());
        BrokenPair("export-invalid-tool-container", x => x["tools"] = "unexpected");
        BrokenPair("export-invalid-flag-type", x => x["features"]!["pairedExport"] = "true");
        BrokenPair("export-build-id-mismatch", x => x["tools"]!["exporter"]!["sdkBuildId"] = sdk.BuildId);
        BrokenPair("export-descriptor-schema-mismatch", x => x["tools"]!["exporter"]!["descriptorSchema"] = 2);
        BrokenPair("export-tool-path-substitution", x => x["tools"]!["exporter"]!["path"] = "profiles/shipping/Release/bin/pixie-player.exe");
        BrokenPair("export-runtime-hash-mismatch", x => x["tools"]!["exporter"]!["runtime"]!["sha256"] = new string('0', 64));
        BrokenPair("export-omitted-crt-satellite", x => x["tools"]!["exporter"]!["dependencies"]!.AsArray().RemoveAt(1));
        BrokenPair("export-dependency-kind-mismatch", x => x["tools"]!["exporter"]!["dependencies"]![1]!["kind"] = "native");
        BrokenPair("export-duplicate-dependency", x =>
        {
            var dependencies = x["tools"]!["exporter"]!["dependencies"]!.AsArray(); dependencies.Add(dependencies[0]!.DeepClone());
        });
        privateSdk.Catalog(x => { x["features"] = new JsonObject { ["pairedExport"] = true }; x["tools"] = new JsonObject { ["exporter"] = new JsonObject() }; });
        Refuses(() => privateSdk.Load(), "private-sdk-cannot-advertise-public-exporter");
        pair.Catalog(x => x["note"] = "Manifest changed after Editor admission");
        Refuses(() => ProjectExportJob.CreateLaunch(pairedSdk, projectPath, outPath), "export-launch-refuses-changed-selected-manifest");
        var refreshedPair = pair.Load();
        File.AppendAllText(toolPath, "changed after admission");
        Refuses(() => ProjectExportJob.CreateLaunch(refreshedPair, projectPath, outPath), "export-launch-revalidates-admitted-payloads");

        Check(sdk.Generation == null, "older-sdk-retains-unadvertised-code-support");
        Broken("missing-advertised-generation-support", f => f.Catalog(x => x["features"] = new JsonObject { ["projectGeneration"] = true }));
        var generation = new Fixture(Path.Combine(work, "Installed generation SDK Å")); generation.Write(); generation.Pair(); generation.Support();
        var generatedSdk = generation.Load();
        var codeProject = Path.Combine(work, "Generated code project Å"); Directory.CreateDirectory(codeProject);
        var noCode = ProjectExportJob.CreateLaunch(generatedSdk, codeProject, outPath, () => throw new InvalidOperationException("No-code used CMake discovery"));
        Check(noCode.ArgumentList.SequenceEqual(new[] { "--sdk", generatedSdk.Root, codeProject, outPath }), "no-code-export-does-not-resolve-cmake-or-change-argv");
        var cmake = Path.Combine(work, "toolchain", "cmake.exe"); Directory.CreateDirectory(Path.GetDirectoryName(cmake)!);
        File.WriteAllText(cmake, "Synthetic CMake path; never execute");
        var temp = Path.Combine(work, "temporary");
        Refuses(() => ProjectBuild.Prepare(codeProject, generatedSdk, () => cmake, temp), "code-build-requires-prior-explicit-adoption");
        SdkRequirements.Adopt(codeProject, generatedSdk);
        var portablePath = Path.Combine(codeProject, SdkRequirements.RelativePath);
        var portable = JsonNode.Parse(File.ReadAllText(portablePath))!.AsObject();
        void Cpu(params string[] features)
        {
            portable["cpuFeatures"] = new JsonArray(features.Select(x => (JsonNode?)JsonValue.Create(x)).ToArray());
            File.WriteAllText(portablePath, portable.ToJsonString());
        }
        Check(SdkRequirements.Validate(codeProject, generatedSdk), "legacy-portable-cpu-order-admitted");
        Cpu("avx2", "f16c", "fma");
        var portableHash = EngineSdk.HashFile(portablePath);
        Check(SdkRequirements.Validate(codeProject, generatedSdk) && EngineSdk.HashFile(portablePath) == portableHash, "c3-portable-cpu-order-admitted-without-rewrite");
        Cpu("avx2", "f16c", "fma", "fma"); Refuses(() => SdkRequirements.Validate(codeProject, generatedSdk), "portable-cpu-duplicate-refused");
        Cpu("avx2", "fma"); Refuses(() => SdkRequirements.Validate(codeProject, generatedSdk), "portable-cpu-missing-refused");
        Cpu("avx2", "f16c", "fma", "sse2"); Refuses(() => SdkRequirements.Validate(codeProject, generatedSdk), "portable-cpu-extra-refused");
        Cpu("AVX2", "f16c", "fma"); Refuses(() => SdkRequirements.Validate(codeProject, generatedSdk), "portable-cpu-case-refused");
        Cpu("avx2", "f16c", "fma");
        Directory.CreateDirectory(Path.Combine(codeProject, "src"));
        var generatedHeader = Path.Combine(codeProject, "src/PixieComponents.generated.hpp");
        var generatedSource = Path.Combine(codeProject, "src/PixieComponents.generated.cpp");
        var generatedCmake = Path.Combine(codeProject, ".pixie-code/Components.cmake");
        foreach (var path in new[] { generatedHeader, generatedSource, generatedCmake }) File.WriteAllText(path, "Owned " + Path.GetFileName(path));
        File.WriteAllText(Path.Combine(codeProject, ".pixie-code/manifest.json"), JsonSerializer.Serialize(new { schemaVersion = 1,
            generatorFamily = "sdk-v1", projectId = "TestGame", components = Array.Empty<string>(), headerHash = EngineSdk.HashFile(generatedHeader),
            sourceHash = EngineSdk.HashFile(generatedSource), cmakeHash = EngineSdk.HashFile(generatedCmake) }));
        var cmakelists = Path.Combine(codeProject, "CMakeLists.txt");
        var originalCmake = File.ReadAllText(generatedSdk.Generation!.CMakeTemplate).Replace("@PIXIE_ID@", "TestGame").Replace("@PIXIE_GENERATED_CANDIDATE_OPTION@", "");
        File.WriteAllText(cmakelists, originalCmake);
        var plan = ProjectBuild.Prepare(codeProject, generatedSdk, () => cmake, temp);
        Check(plan.Build.SequenceEqual(new[] { "--build", Path.Combine(plan.Cache, "build"), "--config", "Release", "--target", "TestGame-module" }) &&
            plan.Configure.Contains("-DPIXIE_PROJECT_BUILD_ROLE=authoring") && plan.Configure.Contains("-DPIXIE_SDK_ROOT=" + generatedSdk.Root) &&
            plan.Library.EndsWith(Path.Combine("Release", "TestGame-module.dll")), "installed-build-keeps-exact-sdk-native-tuple-and-module-target");
        Check(plan.Cache == ProjectBuild.Prepare(codeProject, generatedSdk, () => cmake, temp).Cache && !Directory.Exists(plan.Cache),
            "installed-build-preflight-is-read-only-and-reuses-exact-cache-key");
        var codeLaunch = ProjectExportJob.CreateLaunch(generatedSdk, codeProject, outPath, () => cmake, temp);
        var shippingCache = ProjectBuild.Cache(codeProject, generatedSdk, "shipping-export", temp);
        Check(codeLaunch.Environment["PIXIE_EXPORT_CMAKE"] == cmake && shippingCache != plan.Cache &&
            codeLaunch.ArgumentList.SequenceEqual(new[] { "--sdk", generatedSdk.Root, "--work", shippingCache, codeProject, outPath }),
            "code-export-passes-discovered-cmake-and-separate-owned-cache");
        Refuses(() => ProjectExportJob.CreateLaunch(generatedSdk, codeProject, outPath, () => null, temp), "code-export-refuses-missing-discovered-cmake");
        Refuses(() => ProjectExportJob.CreateLaunch(generatedSdk, codeProject, outPath, () => "cmake.exe", temp), "code-export-refuses-relative-cmake");
        File.AppendAllText(cmakelists, "# Unfamiliar CMake\n"); var unfamiliar = EngineSdk.HashFile(cmakelists);
        Refuses(() => ProjectBuild.Prepare(codeProject, generatedSdk, () => cmake, temp), "installed-build-refuses-unfamiliar-cmake");
        Check(EngineSdk.HashFile(cmakelists) == unfamiliar && !Directory.Exists(plan.Cache), "unfamiliar-cmake-refusal-preserves-preimage-and-cache");
        File.WriteAllText(cmakelists, originalCmake);
        var originalRegistry = File.ReadAllBytes(generatedHeader); File.AppendAllText(generatedHeader, "Modified");
        Refuses(() => ProjectBuild.Prepare(codeProject, generatedSdk, () => cmake, temp), "installed-build-refuses-registry-preimage-change");
        File.WriteAllBytes(generatedHeader, originalRegistry);
        Directory.CreateDirectory(plan.Cache); File.WriteAllText(plan.Owner, "unrelated ownership");
        Refuses(() => ProjectBuild.Prepare(codeProject, generatedSdk, () => cmake, temp), "installed-build-refuses-mixed-cache-ownership");
        var codeBefore = EngineSdk.HashFile(cmakelists);
        var requirementBefore = EngineSdk.HashFile(portablePath);
        var installedSource = ProjectCode.CreateComponent(codeProject, "InstalledProof", generatedSdk);
        Check(File.ReadAllText(Path.Combine(codeProject, installedSource)) == "// Installed source for InstalledProof\n" &&
            File.ReadAllText(Path.Combine(codeProject, "src/components/InstalledProof.hpp")) == "// Installed header for InstalledProof\n",
            "component-source-and-header-use-versioned-selected-sdk-templates");
        Check(ProjectBuild.ReadTarget(codeProject, generatedSdk.Generation!) == "TestGame-module" &&
            EngineSdk.HashFile(cmakelists) == codeBefore && EngineSdk.HashFile(portablePath) == requirementBefore &&
            File.ReadAllText(generatedSource).Contains("InstalledProof::Describe", StringComparison.Ordinal),
            "installed-component-updates-owned-registration-and-preserves-cmake-and-requirement");
        var existing = Path.Combine(codeProject, installedSource);
        File.AppendAllText(existing, "// User-authored edit\n"); var edited = EngineSdk.HashFile(existing);
        Refuses(() => ProjectCode.CreateComponent(codeProject, "InstalledProof", generatedSdk), "installed-component-refuses-existing-user-code", "Refusing to overwrite");
        Check(EngineSdk.HashFile(existing) == edited, "installed-component-collision-preserves-user-code");
        var customCode = Path.Combine(work, "Unfamiliar installed project"); Directory.CreateDirectory(customCode);
        File.WriteAllText(Path.Combine(customCode, "Test.pixie"), """{"name":"Unfamiliar project"}""");
        File.WriteAllText(Path.Combine(customCode, "CMakeLists.txt"), "# User CMake\n"); SdkRequirements.Adopt(customCode, generatedSdk);
        Refuses(() => ProjectCode.CreateComponent(customCode, "DoNotBootstrap", generatedSdk), "installed-component-refuses-missing-owned-registry", "no owned sdk-v1 registry");
        Check(!Directory.Exists(Path.Combine(customCode, "src")) && File.ReadAllText(Path.Combine(customCode, "CMakeLists.txt")) == "# User CMake\n",
            "installed-component-refusal-does-not-bootstrap-unfamiliar-code");
        var originalGeneratedCmake = File.ReadAllBytes(generatedCmake);
        File.AppendAllText(generatedCmake, "# Changed owned registry\n");
        Refuses(() => ProjectCode.CreateComponent(codeProject, "DoNotOverwrite", generatedSdk), "installed-component-refuses-changed-registry", "registry no longer owns");
        Check(!File.Exists(Path.Combine(codeProject, "src/components/DoNotOverwrite.cpp")), "installed-component-preimage-refusal-before-new-code");
        File.WriteAllBytes(generatedCmake, originalGeneratedCmake);
        var badTemplates = new Fixture(Path.Combine(work, "Wrong component template SDK")); badTemplates.Write(); badTemplates.Support(badComponentVersion: true);
        var badTemplateSdk = badTemplates.Load();
        var preflightProject = Path.Combine(work, "Template preflight"); Directory.CreateDirectory(preflightProject); SdkRequirements.Adopt(preflightProject, badTemplateSdk);
        File.WriteAllText(Path.Combine(preflightProject, "Test.pixie"), """{"name":"Template preflight"}""");
        Refuses(() => ProjectCode.CreateComponent(preflightProject, "NoSourceFallback", badTemplateSdk), "installed-component-refuses-version-inventory-mismatch", "template is missing or changed");
        Check(!File.Exists(Path.Combine(preflightProject, "CMakeLists.txt")) && !Directory.Exists(Path.Combine(preflightProject, "src")),
            "installed-component-template-refusal-before-initial-generation");

        var ideBuild = Path.Combine(plan.Cache, "build");
        var ideCache = Path.Combine(ideBuild, "CMakeCache.txt");
        var presetsPath = Path.Combine(codeProject, "CMakeUserPresets.json");
        var ideSettingsPath = Path.Combine(codeProject, ".vscode/settings.json");
        var cacheVariables = plan.Configure.Where(argument => argument.StartsWith("-D", StringComparison.Ordinal))
            .Select(argument => argument[2..].Split('=', 2)).ToDictionary(pair => pair[0], pair => pair[1], StringComparer.Ordinal);
        var ideCacheValues = new Dictionary<string, string>(cacheVariables, StringComparer.Ordinal)
        {
            ["CMAKE_GENERATOR"] = "Visual Studio 18 2026", ["CMAKE_GENERATOR_PLATFORM"] = "x64",
            ["CMAKE_GENERATOR_TOOLSET"] = "v145", ["CMAKE_HOME_DIRECTORY"] = codeProject,
            ["CMAKE_CACHEFILE_DIR"] = ideBuild, ["CMAKE_COMMAND"] = cmake
        };
        void WriteCache(string? key = null, string? value = null) => File.WriteAllLines(ideCache,
            ideCacheValues.Select(pair => $"{pair.Key}:UNINITIALIZED={(key == pair.Key ? value : pair.Value)}"));
        Refuses(() => CodeWorkspace.Prepare(plan), "installed-ide-refuses-missing-configured-cache", "Build this project's code");
        Directory.CreateDirectory(ideBuild);
        WriteCache();
        Refuses(() => CodeWorkspace.Prepare(plan), "installed-ide-refuses-foreign-cache-owner", "unowned or belongs to another build");
        File.Delete(plan.Owner);
        Refuses(() => CodeWorkspace.Prepare(plan), "installed-ide-refuses-unowned-cache", "unowned or belongs to another build");
        File.WriteAllBytes(plan.Owner, plan.Ownership);
        File.AppendAllText(ideCache, "CMAKE_GENERATOR:INTERNAL=Conflicting generator\n");
        Refuses(() => CodeWorkspace.Prepare(plan), "installed-ide-refuses-malformed-cache");
        foreach (var (key, value, reason) in new[] {
            ("CMAKE_HOME_DIRECTORY", work, "another source folder"),
            ("CMAKE_CACHEFILE_DIR", work, "another directory"),
            ("CMAKE_COMMAND", Path.Combine(work, "foreign-cmake.exe"), "another CMake executable"),
            ("PIXIE_ENGINE_MODE", "SOURCE", "PIXIE_ENGINE_MODE"),
            ("PIXIE_SDK_ROOT", work, "PIXIE_SDK_ROOT"),
            ("PIXIE_SDK_PROFILE", "shipping", "PIXIE_SDK_PROFILE"),
            ("PIXIE_PROJECT_BUILD_ROLE", "shipping-export", "PIXIE_PROJECT_BUILD_ROLE"),
            ("CMAKE_CONFIGURATION_TYPES", "Debug;Release", "CMAKE_CONFIGURATION_TYPES")
        })
        {
            WriteCache(key, value);
            Refuses(() => CodeWorkspace.Prepare(plan), "installed-ide-refuses-mismatched-" + key.ToLowerInvariant(), reason);
        }
        Check(!File.Exists(presetsPath) && !File.Exists(ideSettingsPath), "installed-ide-cache-refusals-before-writing-either-file");
        WriteCache();
        Directory.CreateDirectory(Path.GetDirectoryName(ideSettingsPath)!);
        const string conflictingSettings = """{"cmake.cmakePath":"cmake.exe"}""";
        File.WriteAllText(ideSettingsPath, conflictingSettings);
        Refuses(() => CodeWorkspace.Prepare(plan), "installed-ide-refuses-conflicting-cmake-setting", "Existing cmake.cmakePath");
        Check(!File.Exists(presetsPath) && File.ReadAllText(ideSettingsPath) == conflictingSettings,
            "installed-ide-conflicting-setting-is-preserved");
        const string configureOverrides = """{"cmake.configureArgs":["-DPIXIE_ENGINE_MODE=SOURCE"]}""";
        File.WriteAllText(ideSettingsPath, configureOverrides);
        Refuses(() => CodeWorkspace.Prepare(plan), "installed-ide-refuses-configure-argument-overrides", "Existing cmake.configureArgs");
        Check(!File.Exists(presetsPath) && File.ReadAllText(ideSettingsPath) == configureOverrides,
            "installed-ide-configure-argument-overrides-are-preserved");
        const string userSettings = "{\n  // Keep the user's settings\n  \"cmake.configureOnOpen\": false,\n}\n";
        File.WriteAllText(ideSettingsPath, userSettings);
        CodeWorkspace.Prepare(plan);
        var idePreset = JsonNode.Parse(File.ReadAllBytes(presetsPath))!["configurePresets"]![0]!;
        var presetVariables = idePreset["cacheVariables"]!.AsObject();
        Check(idePreset["name"]!.GetValue<string>() == CodeWorkspace.PresetName &&
            idePreset["binaryDir"]!.GetValue<string>() == ideBuild.Replace('\\', '/') &&
            idePreset["generator"]!.GetValue<string>() == "Visual Studio 18 2026" &&
            idePreset["architecture"]!.GetValue<string>() == "x64" && idePreset["toolset"]!.GetValue<string>() == "v145" &&
            presetVariables.Count == cacheVariables.Count && cacheVariables.All(pair => presetVariables[pair.Key]!.GetValue<string>() == pair.Value.Replace('\\', '/')),
            "installed-ide-preset-keeps-exact-owned-cache-generator-and-configure-values");
        var presetHash = EngineSdk.HashFile(presetsPath);
        var settingsHash = EngineSdk.HashFile(ideSettingsPath);
        CodeWorkspace.Prepare(plan, () => true);
        Check(EngineSdk.HashFile(presetsPath) == presetHash && EngineSdk.HashFile(ideSettingsPath) == settingsHash &&
            File.ReadAllText(ideSettingsPath).EndsWith(userSettings[1..], StringComparison.Ordinal),
            "installed-ide-reprepare-keeps-preset-and-user-settings-byte-identical");
        presetVariables["PIXIE_SDK_PROFILE"] = "shipping";
        File.WriteAllText(presetsPath, JsonSerializer.Serialize(new { version = 6, configurePresets = new[] { idePreset } }));
        var foreignPreset = EngineSdk.HashFile(presetsPath);
        Refuses(() => CodeWorkspace.Prepare(plan, () => true), "installed-ide-refuses-conflicting-preset", "different settings");
        Check(EngineSdk.HashFile(presetsPath) == foreignPreset && EngineSdk.HashFile(ideSettingsPath) == settingsHash &&
            EngineSdk.HashFile(cmakelists) == codeBefore && EngineSdk.HashFile(portablePath) == requirementBefore,
            "installed-ide-conflict-keeps-user-preset-settings-and-portable-project-inputs");
        const string legacyPreset = """
            {"version":6,"configurePresets":[{"name":"pixie-editor-local","generator":"Visual Studio 18 2026",
            "binaryDir":"${sourceDir}/build","cacheVariables":{"PIXIE_ENGINE_DIR":"C:/Engine"}}]}
            """;
        File.WriteAllText(presetsPath, legacyPreset);
        Refuses(() => CodeWorkspace.Prepare(plan, () => true), "installed-ide-refuses-legacy-source-preset", "different settings");
        Check(File.ReadAllText(presetsPath) == legacyPreset && EngineSdk.HashFile(ideSettingsPath) == settingsHash,
            "installed-ide-legacy-source-preset-is-preserved");

        File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new
        {
            scope = "synthetic C2/L1/E2/M2/M3/M4 metadata, refusals, build/launch plans, component templates and IDE cache binding; no G4 evidence",
            passed = passed.Count, checks = passed, sdkId = sdk.Id, sdkBuildId = sdk.BuildId,
            manifestSha256 = sdk.ManifestSha256, profileSha256 = sdk.ProfileSha256
        }, new JsonSerializerOptions { WriteIndented = true }));
        return 0;
    }

    private static unsafe int PrepareActualBuild(string work, string root)
    {
        const string manifest = "dd16eda5077a23cc8d2b1cb68feac338a4109dd808a2b809bce570f356566247";
        const string build = "8778598c387956bfc46fc8dacf703383a88bbc68f7ebcce6e308bef740583fc7";
        var sdk = EngineSdk.Load(root, expectedBuildId: build, expectedManifestSha256: manifest);
        var folder = Path.Combine(work, "M2 project Å");
        Directory.CreateDirectory(Path.Combine(folder, "scenes"));
        File.WriteAllText(Path.Combine(folder, "Installed.pixie"), """{"format":"pixie-project","version":1,"name":"Installed M2","startScene":"scenes/main.scene"}""");
        File.WriteAllText(Path.Combine(folder, "scenes/main.scene"), """{"format":"pixie-scene","version":2,"seed":19,"sceneScopes":[{"id":"aa","name":"Empty room"}],"presentation":{"schemaVersion":1,"lightingState":"disabled"},"entities":[],"future":{"preserve":true}}""");
        SdkRequirements.Adopt(folder, sdk);
        var requirement = Path.Combine(folder, SdkRequirements.RelativePath);
        var before = EngineSdk.HashFile(requirement);
        var component = Environment.GetEnvironmentVariable("PIXIE_M3_TEST_COMPONENT");
        var prepareWorkspace = Environment.GetEnvironmentVariable("PIXIE_M4_TEST_WORKSPACE") == "1";
        var componentSource = component == null ? null : ProjectCode.CreateComponent(folder, component, sdk);
        if (component == null) ProjectBuild.AddCode(folder, sdk);
        if (EngineSdk.HashFile(requirement) != before) throw new InvalidOperationException("Add Code rewrote the admitted portable requirement.");
        var plan = ProjectBuild.Prepare(folder, sdk);
        File.WriteAllText(Path.Combine(work, "add-code.json"), JsonSerializer.Serialize(new { folder, root = sdk.Root, manifest, build,
            cmake = plan.CMake, cache = plan.Cache, target = plan.Target, module = plan.Library, configure = plan.Configure, moduleBuild = plan.Build },
            new JsonSerializerOptions { WriteIndented = true }));
        Console.WriteLine("PASS installed SDK Add Code through Editor adapter");
        if (component != null || prepareWorkspace)
        {
            using var buildJob = new ProjectBuild(plan); buildJob.Start();
            var timer = System.Diagnostics.Stopwatch.StartNew();
            while (!buildJob.Finished && timer.Elapsed < TimeSpan.FromSeconds(120)) Thread.Sleep(25);
            if (!buildJob.Finished || !buildJob.Succeeded) throw new InvalidOperationException(buildJob.Problem ?? "Installed module build did not finish.");
        }
        if (prepareWorkspace)
        {
            var cmakeList = Path.Combine(folder, "CMakeLists.txt");
            var cmakeBefore = EngineSdk.HashFile(cmakeList);
            CodeWorkspace.Prepare(plan);
            var preset = Path.Combine(folder, "CMakeUserPresets.json");
            var settings = Path.Combine(folder, ".vscode/settings.json");
            var presetBefore = EngineSdk.HashFile(preset);
            var settingsBefore = EngineSdk.HashFile(settings);
            CodeWorkspace.Prepare(plan);
            if (EngineSdk.HashFile(cmakeList) != cmakeBefore || EngineSdk.HashFile(requirement) != before ||
                EngineSdk.HashFile(preset) != presetBefore || EngineSdk.HashFile(settings) != settingsBefore)
                throw new InvalidOperationException("IDE preparation changed project inputs or rewrote the compatible local preset or settings.");
            Console.WriteLine("PASS installed SDK IDE workspace bound to the configured authoring cache");
        }
        if (component != null)
        {
            if (GLFW.Init() == 0) throw new InvalidOperationException("Could not start GLFW for the scoped component observation.");
            GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
            GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
            GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
            var window = GLFW.CreateWindow(320, 240, "Installed component observation", default, default);
            try
            {
                if (window.Handle == null) throw new InvalidOperationException("Could not create the hidden component observation window.");
                GLFW.MakeContextCurrent(window);
                GameLibrary.SelectSdk(sdk); GameLibrary.Use(plan.Library);
                using var session = EngineSession.OpenProject(320, 240, folder);
                var index = session.TypeIndex(component);
                var loaded = System.Runtime.InteropServices.NativeLibrary.Load(plan.Library);
                uint contract;
                try
                {
                    var address = System.Runtime.InteropServices.NativeLibrary.GetExport(loaded, "pixie_game_candidate_contract");
                    contract = ((delegate* unmanaged[Cdecl]<uint>)address)();
                }
                finally { System.Runtime.InteropServices.NativeLibrary.Free(loaded); }
                if (contract != 0) throw new InvalidOperationException("Installed component generation changed default candidate contract 0.");
                if (index >= 0 && session.ComponentSource(index) != componentSource) throw new InvalidOperationException("Installed component source provenance differs from its template output.");
                File.WriteAllText(Path.Combine(work, "component-observation.json"), JsonSerializer.Serialize(new { component, componentSource,
                    module = plan.Library, moduleSha256 = EngineSdk.HashFile(plan.Library), runtime = sdk.RuntimePath, manifest, build, contract,
                    visible = index >= 0, observedSource = index < 0 ? null : session.ComponentSource(index),
                    scope = "Descriptor visibility only; no candidate or attachment authority exercised" }, new JsonSerializerOptions { WriteIndented = true }));
            }
            finally { if (window.Handle != null) GLFW.DestroyWindow(window); GLFW.Terminate(); }
        }
        return 0;
    }
}
