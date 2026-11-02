using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using PixieEditor.Native;

internal static class CodeWorkspaceTests
{
    internal static int Run(string work, string receipt)
    {
        var checks = new List<string>();
        var engine = EngineFolder.Find();
        var cmake = ProjectBuild.FindCMake() ?? throw new Exception("CMake is required.");
        Directory.CreateDirectory(work);
        void Check(bool passed, string name) { if (!passed) throw new Exception(name); checks.Add(name); Console.WriteLine("PASS " + name); }
        string Make(string name)
        {
            var folder = Path.Combine(work, name); Directory.CreateDirectory(Path.Combine(folder, "build"));
            File.WriteAllText(Path.Combine(folder, "CMakeLists.txt"), "# User CMake is not rewritten.\n");
            File.WriteAllText(Path.Combine(folder, "build", "CMakeCache.txt"), $"CMAKE_GENERATOR:INTERNAL=Visual Studio 18 2026\nCMAKE_HOME_DIRECTORY:INTERNAL={folder}\n");
            return folder;
        }
        string Fingerprint(string folder) => string.Join("\n", Directory.GetFiles(folder, "*", SearchOption.AllDirectories).Order()
            .Select(path => Path.GetRelativePath(folder, path) + Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path)))));
        void Refuses(string folder, string name)
        {
            var before = Fingerprint(folder); var refused = false;
            try { CodeWorkspace.Prepare(folder, engine, cmake); } catch (EngineException) { refused = true; }
            Check(refused && before == Fingerprint(folder), name);
        }
        var clean = Make("project with spaces ação");
        CodeWorkspace.Prepare(clean, engine, cmake);
        using (var settings = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(clean, ".vscode", "settings.json"))))
            Check(settings.RootElement.GetProperty("C_Cpp.default.configurationProvider").GetString() == "ms-vscode.cmake-tools" && settings.RootElement.GetProperty("cmake.configureOnOpen").GetBoolean(), "default-workspace-uses-cmake-provider-and-configures-on-open");
        using (var presets = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(clean, "CMakeUserPresets.json"))))
        {
            var preset = presets.RootElement.GetProperty("configurePresets")[0];
            Check(preset.GetProperty("binaryDir").GetString() == "${sourceDir}/build" && preset.GetProperty("generator").GetString() == "Visual Studio 18 2026" &&
                preset.GetProperty("cacheVariables").GetProperty("PIXIE_ENGINE_DIR").GetString() == engine.Replace('\\', '/'), "preset-reuses-real-project-build-and-engine-with-space-safe-paths");
        }
        var fingerprint = Fingerprint(clean); CodeWorkspace.Prepare(clean, engine, cmake);
        Check(fingerprint == Fingerprint(clean), "repeated-preparation-is-byte-identical");
        var extendedPath = Path.Combine(clean, "CMakeUserPresets.json");
        var extended = JsonNode.Parse(File.ReadAllText(extendedPath))!;
        extended["configurePresets"]![0]!["cacheVariables"]!["USER_OPTION"] = "ON";
        File.WriteAllText(extendedPath, extended.ToJsonString());
        fingerprint = Fingerprint(clean); CodeWorkspace.Prepare(clean, engine, cmake);
        Check(fingerprint == Fingerprint(clean), "compatible-preset-retains-extra-user-cache-variables");
        var custom = Make("custom"); Directory.CreateDirectory(Path.Combine(custom, ".vscode"));
        var jsonc = "// User comment with { and ação\r\n{\r\n  // Keep this exactly\r\n  \"files.associations\": { \"*.mine\": \"cpp\" },\r\n  \"cmake.configureOnOpen\": false,\r\n}\r\n";
        var jsoncBytes = Encoding.UTF8.GetPreamble().Concat(Encoding.UTF8.GetBytes(jsonc)).ToArray();
        File.WriteAllBytes(Path.Combine(custom, ".vscode", "settings.json"), jsoncBytes);
        var presetText = "{\n\"version\":6,\"vendor\":{\"user\":42},\"configurePresets\":[\n{\"name\":\"custom\",\"generator\":\"Ninja\",\"binaryDir\":\"${sourceDir}/custom\"}\n]}";
        File.WriteAllText(Path.Combine(custom, "CMakeUserPresets.json"), presetText);
        CodeWorkspace.Prepare(custom, engine, cmake);
        var after = File.ReadAllBytes(Path.Combine(custom, ".vscode", "settings.json"));
        Check(after.AsSpan().StartsWith(Encoding.UTF8.GetPreamble()) && Encoding.UTF8.GetString(after).EndsWith(jsonc[(jsonc.IndexOf("{\r\n", StringComparison.Ordinal) + 1)..], StringComparison.Ordinal), "jsonc-bom-comments-unknown-values-and-formatting-preserved");
        Check(Encoding.UTF8.GetString(after).Contains("\"cmake.configureOnOpen\": false", StringComparison.Ordinal), "explicit-configure-on-open-choice-not-overridden");
        Check(File.ReadAllText(Path.Combine(custom, "CMakeUserPresets.json")).Contains(presetText[presetText.IndexOf("\n{\"name\"", StringComparison.Ordinal)..], StringComparison.Ordinal), "custom-preset-and-unrecognized-properties-retained");
        var backups = Directory.GetFiles(Path.Combine(custom, ".local", "code-editor-backups"), "*", SearchOption.AllDirectories);
        Check(backups.Any(path => File.ReadAllBytes(path).SequenceEqual(jsoncBytes)) && backups.Any(path => File.ReadAllText(path) == presetText), "config-preimages-retained-before-additive-write");
        foreach (var (name, content) in new[] { ("provider-conflict", "{\"C_Cpp.default.configurationProvider\":\"other.provider\"}"), ("presets-disabled", "{\"cmake.useCMakePresets\":\"never\"}"), ("invalid-jsonc", "{not json}"), ("duplicate-key", "{\"x\":1,\"x\":2}") })
        {
            var folder = Make(name); Directory.CreateDirectory(Path.Combine(folder, ".vscode")); File.WriteAllText(Path.Combine(folder, ".vscode", "settings.json"), content);
            Refuses(folder, name + "-refuses-before-writing-either-config");
        }
        var conflicting = Make("preset-conflict"); File.WriteAllText(Path.Combine(conflicting, "CMakeUserPresets.json"), "{\"version\":6,\"configurePresets\":[{\"name\":\"pixie-editor-local\",\"generator\":\"Ninja\"}]}");
        Refuses(conflicting, "preset-name-collision-does-not-rewrite-user-preset");
        var included = Make("included-collision");
        File.WriteAllText(Path.Combine(included, "CMakePresets.json"), "{\"version\":6,\"include\":[\"shared.json\"]}");
        File.WriteAllText(Path.Combine(included, "shared.json"), "{\"version\":6,\"configurePresets\":[{\"name\":\"pixie-editor-local\",\"generator\":\"Ninja\",\"binaryDir\":\"${sourceDir}/custom\"}]}");
        Refuses(included, "cmake-resolves-included-preset-collision-before-writing");
        var ancestor = Make("file-ancestor"); File.WriteAllText(Path.Combine(ancestor, ".vscode"), "unknown data"); Refuses(ancestor, "file-ancestor-preserved");
        var wrong = Make("foreign-cache"); File.WriteAllText(Path.Combine(wrong, "build", "CMakeCache.txt"), "CMAKE_GENERATOR:INTERNAL=Visual Studio 18 2026\nCMAKE_HOME_DIRECTORY:INTERNAL=C:/different\n"); Refuses(wrong, "foreign-cache-refused");
        File.WriteAllText(receipt, JsonSerializer.Serialize(new { passed = checks.Count, checks }, new JsonSerializerOptions { WriteIndented = true }));
        return 0;
    }
}
