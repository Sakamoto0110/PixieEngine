using System.Diagnostics;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using PixieEditor.Native;
using PixieEditor.Ui;
using PixieEditor.Editing;

internal static class Program
{
    private static readonly List<string> Passed = [];
    private static void Check(bool value, string name)
    {
        if (!value) throw new InvalidOperationException(name);
        Passed.Add(name); Console.WriteLine("PASS " + name);
    }
    private static string Fingerprint(string folder) => string.Join("\n", Directory.GetFiles(folder, "*", SearchOption.AllDirectories)
        .Order(StringComparer.Ordinal).Select(path => Path.GetRelativePath(folder, path) + ":" + Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path)))));
    private static void Refuses(Action action, string name)
    {
        var refused = false; try { action(); } catch (EngineException) { refused = true; }
        Check(refused, name);
    }
    private static int Main(string[] args)
    {
        if (args.Length != 0 && args[0] == "--installed-fixture")
        {
            if (args.Length != 7 || args[5] is not ("--visual-environment" or "--presentation" or "--scene-authoring"))
                return 2;
            var sdk = EngineSdk.Load(args[1], args[2], args[3], "fixture", expectedManifestSha256: args[4]);
            GameLibrary.SelectSdk(sdk);
            args = [args[5], sdk.RuntimePath, args[6]];
        }
        if (args.Length == 2 && args[0] == "--sdk-selection") return SdkSelectionTests.Run(args[1]);
        if (args.Length == 3 && args[0] == "--process-reopen")
            return ProcessReopenTests.Run(args[1], args[2]);
        if (args.Length == 3 && args[0] == "--module-process-reopen")
            return ProcessReopenTests.Run(args[1], args[2], true);
        if (args.Length == 2 && args[0] == "--prepare-workspace")
        {
            CodeWorkspace.Prepare(args[1], EngineFolder.Find(), ProjectBuild.FindCMake() ?? throw new EngineException("CMake was not found."));
            return 0;
        }
        if (args.Length < 3) return 2;
        if (args[0] == "--module-load-retry") return ModuleLoadRetryTests.Run(args[1], args[2]);
        if (args[0] == "--module-load-repair") return ModuleLoadRetryTests.Run(args[1], args[2], true);
        if (args[0] == "--module-scene") return ModuleSceneTests.Run(args[1], args[2]);
        if (args[0] == "--candidate-scene") return SceneCandidateTests.Run(args[1], args[2]);
        if (args[0] == "--scene-authoring") return SceneAuthoringTests.Run(args[1], args[2]);
        if (args[0] == "--presentation-visual") return PresentationVisualTests.Run(args[1], args[2]);
        if (args[0] == "--presentation") return PresentationTests.Run(args[1], args[2]);
        if (args[0] == "--candidate-measure") return SceneCandidateTests.Run(args[1], args[2], int.Parse(args[3]), args[4]);
        if (args[0] == "--scene-authoring-measure") return SceneCandidateTests.Run(args[1], args[2], int.Parse(args[3]), args[4], true);
        if (args[0] == "--module-authoring-measure") return SceneCandidateTests.Run(args[1], args[2], int.Parse(args[3]), args[4], true, true);
        if (args[0] == "--visual-layout") return VisualLayoutTests.Run(args[1], args[2]);
        if (args[0] == "--visual-environment") return EnvironmentViewportTests.Run(args[1], args[2]);
        if (args[0] == "--open-code") { Console.WriteLine(CodeEditor.Open(args[1], args[2]) ?? "VS Code opened with CMake Tools configuration."); return 0; }
        if (args[0] == "--workspace-tests") return CodeWorkspaceTests.Run(args[1], args[2]);
        if (args[0] == "--native") return Native(args);
        GameLibrary.Use(Path.GetFullPath(args[0]));
        var work = Path.GetFullPath(args[1]); Directory.CreateDirectory(work);
        var folder = Path.Combine(work, "Foo"); EngineSession.CreateProject(folder);
        foreach (var name in new[] { "Spinner", "FooProject", "class", "3Bad", "Bad/Path", "Bad__Name", "pixie" })
        {
            var before = Fingerprint(folder);
            Refuses(() => ProjectCode.CreateComponent(folder, name), "reject-identifier-" + name);
            Check(before == Fingerprint(folder), "name-refusal-preserves-bytes-" + name);
        }
        var source = ProjectCode.CreateComponent(folder, "UserLogic");
        Check(source == "src/components/UserLogic.cpp", "explicit-user-source-returned");
        var cpp = Path.Combine(folder, source);
        File.AppendAllText(cpp, "\n// User-owned implementation edit.\n");
        var userBytes = File.ReadAllBytes(cpp);
        File.AppendAllText(Path.Combine(folder, "CMakeLists.txt"), "\n# User CMake customization.\n");
        var cmakeBytes = File.ReadAllBytes(Path.Combine(folder, "CMakeLists.txt"));
        ProjectCode.CreateComponent(folder, "OtherLogic");
        ProjectCode.CreateComponent(folder, "ThirdLogic");
        Check(userBytes.SequenceEqual(File.ReadAllBytes(cpp)), "new-components-preserve-existing-user-cpp");
        Check(cmakeBytes.SequenceEqual(File.ReadAllBytes(Path.Combine(folder, "CMakeLists.txt"))), "registry-preserves-user-cmake-outside-managed-block");
        var manifest = Path.Combine(folder, ".pixie-code", "manifest.json");
        var valid = File.ReadAllBytes(manifest);
        foreach (var mutation in new Action<JsonObject>[]
        {
            data => data["schemaVersion"] = "wrong", data => data["projectId"] = false,
            data => data["components"] = new JsonArray((JsonNode?)null), data => data["components"] = new JsonArray(123),
            data => data["sourceHash"] = new JsonObject(), data => data["cmakeHash"] = true,
        })
        {
            var data = JsonNode.Parse(valid)!.AsObject(); mutation(data); File.WriteAllText(manifest, data.ToJsonString());
            var before = Fingerprint(folder);
            Refuses(() => ProjectCode.CreateComponent(folder, "MustNotWrite"), "reject-valid-json-wrong-schema");
            Check(before == Fingerprint(folder), "schema-refusal-preserves-all-bytes");
            File.WriteAllBytes(manifest, valid);
        }
        var registry = Path.Combine(folder, "src", "PixieComponents.generated.cpp"); var registryBytes = File.ReadAllBytes(registry);
        File.AppendAllText(registry, "// Unknown edit.\n"); var changed = Fingerprint(folder);
        Refuses(() => ProjectCode.CreateComponent(folder, "NoOverwrite"), "reject-edited-generated-registry");
        Check(changed == Fingerprint(folder), "registry-refusal-preserves-unknown-edit"); File.WriteAllBytes(registry, registryBytes);
        var collision = Path.Combine(work, "Collision"); EngineSession.CreateProject(collision);
        Directory.CreateDirectory(Path.Combine(collision, "src", "components"));
        File.WriteAllText(Path.Combine(collision, "src", "components", "Existing.cpp"), "unknown user source");
        changed = Fingerprint(collision);
        Refuses(() => ProjectCode.CreateComponent(collision, "Existing"), "reject-existing-source-before-add-code");
        Check(changed == Fingerprint(collision), "source-collision-does-not-create-base-code");
        Refuses(() => ProjectCode.ResolveSource(folder, "../engine.cpp"), "source-cannot-leave-project");
        Refuses(() => ProjectCode.ResolveSource(folder, ""), "engine-component-without-metadata-has-no-code-path");
        var info = CodeEditor.StartInfo("unlaunched-code-editor", folder, source);
        Check(!info.UseShellExecute && info.ArgumentList.SequenceEqual(new[] { "--reuse-window", folder, "--goto", Path.GetFullPath(cpp) }), "vscode-opens-exact-project-and-source-with-structured-arguments");
        foreach (var modified in new[] { false, true })
        {
            var legacy = Path.Combine(work, modified ? "CustomLegacy" : "Legacy"); EngineSession.CreateProject(legacy);
            var id = Path.GetFileName(legacy);
            var templates = Path.Combine(EngineFolder.Find(), "templates", "project-code", "legacy"); Directory.CreateDirectory(Path.Combine(legacy, "src"));
            foreach (var (input, output) in new[] { ("CMakeLists.txt.in", "CMakeLists.txt"), ("Project.hpp.in", $"src/{id}Project.hpp"), ("Project.cpp.in", $"src/{id}Project.cpp") })
                File.WriteAllText(Path.Combine(legacy, output), File.ReadAllText(Path.Combine(templates, input)).Replace("@PIXIE_ID@", id).Replace("@PIXIE_NAME_LITERAL@", id));
            if (modified)
            {
                File.AppendAllText(Path.Combine(legacy, $"src/{id}Project.cpp"), "// Unknown customization.\n"); changed = Fingerprint(legacy);
                Refuses(() => ProjectCode.CreateComponent(legacy, "LegacyActor"), "custom-legacy-needs-explicit-registration-hook");
                Check(changed == Fingerprint(legacy), "custom-legacy-refusal-preserves-user-source");
            }
            else Check(ProjectCode.CreateComponent(legacy, "LegacyActor").EndsWith("LegacyActor.cpp", StringComparison.Ordinal), "untouched-legacy-template-adopted-explicitly");
        }
        // Make this disposable consumer record callbacks; production templates themselves define no gameplay.
        var header = Path.Combine(folder, "src", "components", "UserLogic.hpp");
        File.WriteAllText(header, File.ReadAllText(header).Replace("bool enabled = true;", "bool enabled = true; int keyEvents = 0; int buttonEvents = 0;"));
        var text = File.ReadAllText(cpp).Replace("\r\n", "\n").Replace(".Source(\"src/components/UserLogic.cpp\")", ".Source(\"src/components/UserLogic.cpp\").Field<&UserLogic::keyEvents>(\"keyEvents\").Field<&UserLogic::buttonEvents>(\"buttonEvents\")");
        text = text.Replace("void UserLogic::OnKey(const pixie::KeyEvent& event, const pixie::Input&) {\n    if (!enabled) return;", "void UserLogic::OnKey(const pixie::KeyEvent& event, const pixie::Input&) {\n    if (!enabled) return;\n    ++keyEvents;");
        text = text.Replace("void UserLogic::OnMouseButton(const pixie::MouseButtonEvent& event, const pixie::Input&) {\n    if (!enabled) return;", "void UserLogic::OnMouseButton(const pixie::MouseButtonEvent& event, const pixie::Input&) {\n    if (!enabled) return;\n    ++buttonEvents;");
        Check(text.Contains("++keyEvents;", StringComparison.Ordinal) && text.Contains("++buttonEvents;", StringComparison.Ordinal), "consumer-callback-instrumentation-is-present-before-build");
        File.WriteAllText(cpp, text);
        File.WriteAllText(args[2], JsonSerializer.Serialize(new { passed = Passed.Count, checks = Passed, project = folder }, new JsonSerializerOptions { WriteIndented = true }));
        if (args.Length > 3 && args[3] == "--build")
        {
            using var build = new ProjectBuild(folder); build.Start();
            var timer = Stopwatch.StartNew();
            while (!build.Finished && timer.Elapsed < TimeSpan.FromMinutes(15)) Thread.Sleep(25);
            File.WriteAllLines(Path.Combine(work, "project-build.log"), build.Lines);
            Check(build.Finished && build.Succeeded, "generated-multiple-components-build-as-real-game");
            File.WriteAllText(args[2], JsonSerializer.Serialize(new { passed = Passed.Count, checks = Passed, project = folder }, new JsonSerializerOptions { WriteIndented = true }));
        }
        return 0;
    }

    private static unsafe int Native(string[] args)
    {
        // Separate process: one selected game factory, just like the editor and standalone player.
        var module = Path.GetFullPath(args[1]); var folder = Path.GetFullPath(args[2]);
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(320, 240, "User code qualification", default, default);
        if (window.Handle == null) return 4;
        GLFW.MakeContextCurrent(window); GameLibrary.Use(module);
        try
        {
            using var session = EngineSession.OpenProject(320, 240, folder);
            var first = session.Entities()[0]; var second = session.CreateEntity("device only, no movement assumption");
            var user = session.TypeIndex("UserLogic"); var other = session.TypeIndex("OtherLogic");
            var keyboard = session.TypeIndex("KeyboardControl"); var mouse = session.TypeIndex("MouseControl");
            Check(user >= 0 && other >= 0 && session.TypeIndex("ThirdLogic") >= 0, "generated-registry-describes-all-component-types");
            Check(session.ComponentSource(user) == "src/components/UserLogic.cpp" && session.ComponentSource(keyboard) == "", "native-source-provenance-distinguishes-user-and-engine-types");
            foreach (var entity in new[] { first, second }) { session.AddComponent(entity, user); session.AddComponent(entity, other); session.AddComponent(entity, mouse); }
            session.AddComponent(first, keyboard);
            var firstId = session.SceneId(first)!.Value; var secondId = session.SceneId(second)!.Value;
            session.Save();
            var cleanScene = session.ProjectSnapshot();
            var revision = session.BindingRevision;
            var notified = false;
            void OnBindingsReplacing() => notified = true;
            session.BindingsReplacing += OnBindingsReplacing;
            var refusedLiveRestore = false;
            try { session.ProjectRestore(cleanScene); }
            catch (EngineException) { refusedLiveRestore = true; }
            session.BindingsReplacing -= OnBindingsReplacing;
            Check(refusedLiveRestore && !notified && session.BindingRevision == revision &&
                session.Find(firstId) == first && session.ProjectSnapshot() == cleanScene && !session.Unsaved,
                "live-project-restore-refused-before-native-mutation");
            var v2Folder = Path.Combine(Path.GetDirectoryName(folder)!, "v2-handoff", "Foo");
            EngineSession.CreateProject(v2Folder);
            var savedScenePath = Directory.GetFiles(Path.Combine(v2Folder, "scenes"), "*.scene").Single();
            ulong firstV2Id, secondV2Id;
            using (var preparedV2 = EngineSession.OpenProject(320, 240, v2Folder))
            {
                var firstV2 = preparedV2.Entities()[0];
                var secondV2 = preparedV2.CreateEntity("second selection");
                firstV2Id = preparedV2.SceneId(firstV2)!.Value;
                secondV2Id = preparedV2.SceneId(secondV2)!.Value;
                preparedV2.Save();
            }
            var savedV1 = File.ReadAllBytes(savedScenePath);
            using (var authoredV2 = EngineSession.OpenProject(320, 240, v2Folder))
            {
                var scene = JsonNode.Parse(authoredV2.ProjectSnapshot())!.AsObject();
                Check(scene["version"]!.GetValue<int>() == 1, "handoff-v2-fixture-starts-from-v1-disk-scene");
                scene["version"] = 2;
                scene["sceneScopes"] = new JsonArray(new JsonObject
                {
                    ["id"] = "00000000000000aa", ["name"] = "Empty room"
                });
                authoredV2.ProjectRestore(scene.ToJsonString());
                Refuses(authoredV2.Save, "recovered-v2-save-refuses-implicit-v1-file-conversion");
                Check(savedV1.SequenceEqual(File.ReadAllBytes(savedScenePath)) && authoredV2.Unsaved &&
                    !File.Exists(savedScenePath + ".v1-backup"),
                    "recovered-v2-save-preserves-v1-bytes-and-dirty-state-without-implicit-backup");
                var rejectedHistory = new History();
                var invalidReplacement = "{\"format\":\"pixie-entities\",\"version\":2,\"sceneScopes\":[],\"entities\":[{\"id\":\"" + firstV2Id.ToString("x16") + "\",\"components\":{\"UserLogic\":{}}}]}";
                var preservedScene = authoredV2.ProjectSnapshot();
                var preservedHandle = authoredV2.Find(firstV2Id);
                var preservedRevision = authoredV2.BindingRevision;
                var notices = 0;
                void ReplacementNotice() => ++notices;
                authoredV2.BindingsReplacing += ReplacementNotice;
                try
                {
                    rejectedHistory.Record(new ReplaceStep("rejected v2 replacement", [firstV2Id], invalidReplacement, null));
                    Refuses(() => rejectedHistory.Undo(authoredV2), "v2-history-rejects-custom-type-before-destruction");
                    Refuses(() => authoredV2.Restore(invalidReplacement, newIds: false), "v2-import-rejects-before-binding-notification");
                }
                finally { authoredV2.BindingsReplacing -= ReplacementNotice; }
                Check(authoredV2.Find(firstV2Id) == preservedHandle && authoredV2.ProjectSnapshot() == preservedScene &&
                    authoredV2.BindingRevision == preservedRevision && notices == 0 && authoredV2.Unsaved &&
                    rejectedHistory.UndoCount == 1 && rejectedHistory.RedoCount == 0,
                    "rejected-v2-replacement-preserves-world-history-and-bindings");
                Check(authoredV2.SceneScopes() is [{ Id: 0xaa, Name: "Empty room" }] &&
                    authoredV2.ViewportEnvironmentSelection.Kind == PixieEnvironmentSelectionKind.LegacyWorld,
                    "editor-lists-empty-scene-with-world-environment-default");
                authoredV2.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, 0xaa);
                var chosenV2 = new Selection();
                chosenV2.Set(authoredV2.Find(firstV2Id)!.Value);
                chosenV2.Set(authoredV2.Find(secondV2Id)!.Value);
                Check(authoredV2.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL) &&
                    authoredV2.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld,
                    "viewport-environment-selection-is-independent-of-entity-selection");
                var noScope = JsonNode.Parse(authoredV2.ProjectSnapshot())!.AsObject();
                noScope["sceneScopes"] = new JsonArray();
                authoredV2.ProjectRestore(noScope.ToJsonString());
                Check(authoredV2.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL) &&
                    authoredV2.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.RemovedSceneFallback &&
                    authoredV2.ViewportEnvironment.Diagnostic.Contains("using World", StringComparison.Ordinal),
                    "missing-scene-intent-survives-world-replacement-with-world-fallback");
                authoredV2.ProjectRestore(scene.ToJsonString());
                Check(authoredV2.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld,
                    "restored-scene-definition-resolves-retained-environment-intent");
                var settings = authoredV2.CreateEntity("world defaults");
                var physics = authoredV2.TypeIndex("PhysicsSettings2D");
                authoredV2.AddComponent(settings, physics);
                var gravity = authoredV2.FieldIndex(physics, "gravity");
                authoredV2.Set(settings, physics, gravity, new PixieValue { X = 42, Y = 930 });
                var authoredSnapshot = JsonNode.Parse(authoredV2.ProjectSnapshot())!.AsObject();
                Check(authoredSnapshot["version"]!.GetValue<int>() == 2 &&
                    authoredSnapshot["sceneScopes"]!.AsArray().Count == 1 && authoredV2.Unsaved,
                    "v2-handoff-captures-empty-scope-and-unsaved-world-default");
                var v2Handoff = CodeReopen.Capture(authoredV2,
                    [authoredV2.Find(firstV2Id)!.Value, authoredV2.Find(secondV2Id)!.Value], null,
                    authoredV2.Find(secondV2Id));
                using var restoredV2 = EngineSession.OpenProject(320, 240, v2Folder);
                var state = CodeReopen.Restore(restoredV2, v2Handoff);
                Check(restoredV2.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL) &&
                    restoredV2.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld,
                    "dirty-handoff-preserves-scene-environment-selection");
                var selectionV2 = new Selection();
                selectionV2.Set(state.Selection.Select(restoredV2.Find).OfType<uint>(),
                    state.Anchor is { } anchorId ? restoredV2.Find(anchorId) : null);
                var restoredSnapshot = JsonNode.Parse(restoredV2.ProjectSnapshot())!.AsObject();
                var emptyScope = restoredSnapshot["sceneScopes"]!.AsArray().Single()!.AsObject();
                var restoredSettings = restoredV2.Find(authoredV2.SceneId(settings)!.Value)!.Value;
                Check(restoredV2.Unsaved && state.Selection.SequenceEqual(new[] { firstV2Id, secondV2Id }) &&
                    state.Anchor == secondV2Id && restoredV2.Find(firstV2Id) != null && restoredV2.Find(secondV2Id) != null &&
                    selectionV2.Items.SequenceEqual(new[] { restoredV2.Find(firstV2Id)!.Value, restoredV2.Find(secondV2Id)!.Value }) &&
                    selectionV2.Anchor == restoredV2.Find(secondV2Id) &&
                    restoredSnapshot["version"]!.GetValue<int>() == 2 &&
                    emptyScope["id"]!.GetValue<string>() == "00000000000000aa" &&
                    restoredV2.Get(restoredSettings, physics, gravity).X == 42 &&
                    restoredV2.Get(restoredSettings, physics, gravity).Y == 930 &&
                    savedV1.SequenceEqual(File.ReadAllBytes(savedScenePath)),
                    "v2-handoff-restores-empty-scope-world-default-selection-anchor-and-dirty-state");
            }
            session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, 0xffff);
            var cleanHandoff = CodeReopen.Capture(session, [first, second], null, second);
            using (var clean = EngineSession.OpenProject(320, 240, folder))
            {
                var data = CodeReopen.Restore(clean, cleanHandoff);
                Check(clean.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xffffUL) &&
                    clean.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.RemovedSceneFallback,
                    "clean-handoff-preserves-missing-environment-intent");
                Check(!clean.Unsaved && data.Selection.SequenceEqual(new[] { firstId, secondId }) && data.Anchor == secondId,
                    "clean-rebuild-preserves-ordered-selection-and-anchor-without-dirtying-scene");
            }
            var enabled = session.FieldIndex(user, "enabled");
            session.Set(first, user, enabled, new PixieValue { X = 0 });
            var disk = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single(); var saved = File.ReadAllBytes(disk);
            var pending = new CodeReopen.Request("ThirdLogic", "src/components/ThirdLogic.cpp", [firstId, secondId]);
            var handoff = CodeReopen.Capture(session, [first, second], pending, second);
            Check(saved.SequenceEqual(File.ReadAllBytes(disk)) && session.Unsaved, "code-handoff-never-implicitly-saves-unsaved-scene");
            foreach (var (kind, id) in new[] { (0, 1UL), (1, 0UL), (99, 0UL) })
            {
                var invalidChoice = JsonNode.Parse(File.ReadAllText(handoff))!.AsObject();
                invalidChoice["Environment"] = new JsonObject { ["Kind"] = kind, ["SceneId"] = id };
                var invalidChoicePath = Path.Combine(Path.GetDirectoryName(handoff)!, Guid.NewGuid().ToString("N") + ".json");
                File.WriteAllText(invalidChoicePath, invalidChoice.ToJsonString());
                using var unchanged = EngineSession.OpenProject(320, 240, folder);
                var before = unchanged.ProjectSnapshot();
                var originalHandle = unchanged.Find(firstId);
                var originalRevision = unchanged.BindingRevision;
                Refuses(() => CodeReopen.Restore(unchanged, invalidChoicePath), "invalid-environment-choice-rejected");
                Check(unchanged.ProjectSnapshot() == before && unchanged.Find(firstId) == originalHandle &&
                    unchanged.BindingRevision == originalRevision && !unchanged.Unsaved &&
                    unchanged.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.LegacyWorld, 0UL),
                    "invalid-environment-choice-rejected-before-scene-mutation");
            }
            var legacyChoice = JsonNode.Parse(File.ReadAllText(cleanHandoff))!.AsObject();
            legacyChoice.Remove("Environment");
            var legacyChoicePath = Path.Combine(Path.GetDirectoryName(cleanHandoff)!, Guid.NewGuid().ToString("N") + ".json");
            File.WriteAllText(legacyChoicePath, legacyChoice.ToJsonString());
            using (var legacy = EngineSession.OpenProject(320, 240, folder))
            {
                CodeReopen.Restore(legacy, legacyChoicePath);
                Check(!legacy.Unsaved && legacy.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.LegacyWorld, 0UL),
                    "older-handoff-without-environment-retains-world-default");
            }
            var invalid = JsonNode.Parse(File.ReadAllText(handoff))!.AsObject();
            invalid["Snapshot"] = "{broken";
            var invalidHandoff = Path.Combine(Path.GetDirectoryName(handoff)!, Guid.NewGuid().ToString("N") + ".json");
            File.WriteAllText(invalidHandoff, invalid.ToJsonString());
            using (var unchanged = EngineSession.OpenProject(320, 240, folder))
            {
                unchanged.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, 0xeeee);
                var before = unchanged.ProjectSnapshot();
                var originalHandle = unchanged.Find(firstId);
                Refuses(() => CodeReopen.Restore(unchanged, invalidHandoff), "invalid-v2-handoff-rejected");
                Check(File.Exists(invalidHandoff) && unchanged.ProjectSnapshot() == before &&
                    unchanged.Find(firstId) == originalHandle && !unchanged.Unsaved && saved.SequenceEqual(File.ReadAllBytes(disk)) &&
                    unchanged.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xeeeeUL),
                    "failed-v2-restore-retains-handoff-and-published-scene");
            }
            var validLegacy = JsonNode.Parse(File.ReadAllText(handoff))!.AsObject();
            validLegacy["Version"] = 1;
            var legacyFragment = JsonNode.Parse(validLegacy["Snapshot"]!.GetValue<string>())!.AsObject();
            legacyFragment["format"] = "pixie-entities";
            legacyFragment.Remove("seed");
            validLegacy["Snapshot"] = legacyFragment.ToJsonString();
            var validLegacyPath = Path.Combine(Path.GetDirectoryName(handoff)!, Guid.NewGuid().ToString("N") + ".json");
            File.WriteAllText(validLegacyPath, validLegacy.ToJsonString());
            using (var legacyRestored = EngineSession.OpenProject(320, 240, folder))
            {
                CodeReopen.Restore(legacyRestored, validLegacyPath);
                Check(legacyRestored.Unsaved && legacyRestored.Get(legacyRestored.Find(firstId)!.Value, user, enabled).X == 0 &&
                    JsonNode.Parse(legacyRestored.ProjectSnapshot())!["version"]!.GetValue<int>() == 1 &&
                    saved.SequenceEqual(File.ReadAllBytes(disk)), "valid-v1-handoff-retains-values-format-and-saved-file");
            }
            using (var resumed = EngineSession.OpenProject(320, 240, folder))
            {
                var legacyRejected = JsonNode.Parse(File.ReadAllText(handoff))!.AsObject();
                legacyRejected["Version"] = 1;
                legacyRejected["Snapshot"] = "{\"format\":\"pixie-entities\",\"version\":1,\"entities\":[{\"id\":\"0\"}]}";
                var legacyRejectedPath = Path.Combine(Path.GetDirectoryName(handoff)!, Guid.NewGuid().ToString("N") + ".json");
                File.WriteAllText(legacyRejectedPath, legacyRejected.ToJsonString());
                var legacyBefore = resumed.ProjectSnapshot();
                var legacyHandle = resumed.Find(firstId);
                var legacyRevision = resumed.BindingRevision;
                Refuses(() => CodeReopen.Restore(resumed, legacyRejectedPath), "legacy-handoff-rejects-invalid-replacement");
                Check(resumed.ProjectSnapshot() == legacyBefore && resumed.Find(firstId) == legacyHandle &&
                    resumed.BindingRevision == legacyRevision && !resumed.Unsaved,
                    "legacy-handoff-rejects-before-destroying-current-entities");
                var state = CodeReopen.Restore(resumed, handoff); var attach = CodeReopen.ResolveAttachment(resumed, state.Pending!);
                foreach (var entity in attach.Entities) resumed.AddComponent(entity, attach.Type);
                Check(resumed.Unsaved && state.Anchor == secondId && attach.Entities.Length == 2 &&
                    resumed.Get(resumed.Find(firstId)!.Value, user, enabled).X == 0,
                    "unsaved-rebuild-restores-values-anchor-and-multiple-stable-targets");
                resumed.Save();
            }
            using (var reopened = EngineSession.OpenProject(320, 240, folder))
                Check(reopened.Has(reopened.Find(firstId)!.Value, reopened.TypeIndex("ThirdLogic")) && reopened.Has(reopened.Find(secondId)!.Value, other), "code-components-save-reopen-with-multiple-components-per-entity");
            session.Set(first, user, enabled, new PixieValue { X = 1 });
            session.Play(); session.Paused = true; first = session.Find(firstId)!.Value; second = session.Find(secondId)!.Value;
            foreach (var action in new[] { 1, 0, 1, 2 }) session.InputEvent(new() { Kind = 0, Code = 65, ScanCode = 30, Action = action, Modifiers = 1 });
            session.InputEvent(new() { Kind = 1, Code = 0, Action = 1, Modifiers = 2 }); session.Step();
            var keys = session.FieldIndex(user, "keyEvents"); var buttons = session.FieldIndex(user, "buttonEvents");
            Check(session.Get(first, user, keys).X == 4 && session.Get(second, user, keys).X == 0, "real-capi-ordered-key-events-only-reach-enabled-keyboard-entities");
            Check(session.Get(first, user, buttons).X == 1 && session.Get(second, user, buttons).X == 1, "mouse-only-entity-receives-generic-mouse-callbacks");
            session.Step(); Check(session.Get(first, user, keys).X == 4, "fixed-step-consumes-event-callbacks-once");
            session.Set(first, keyboard, session.FieldIndex(keyboard, "enabled"), new PixieValue());
            session.InputEvent(new() { Kind = 0, Code = 65, Action = 0 }); session.Step();
            Check(session.Get(first, user, keys).X == 4, "native-disabled-keyboard-stops-callback-delivery");
            session.Stop(); first = session.Find(firstId)!.Value;
            Check(session.Get(first, user, keys).X == 0, "stop-discards-runtime-behaviour-state");
            if (args.Length > 3 && args[3] == "--ui") EditorUi.Run(window, session, [session.Find(firstId)!.Value, session.Find(secondId)!.Value], folder, Check);
            File.WriteAllText(Path.Combine(folder, "native-results.json"), JsonSerializer.Serialize(new { passed = Passed.Count, checks = Passed }, new JsonSerializerOptions { WriteIndented = true }));
            return 0;
        }
        finally { GLFW.DestroyWindow(window); GLFW.Terminate(); }
    }
}
