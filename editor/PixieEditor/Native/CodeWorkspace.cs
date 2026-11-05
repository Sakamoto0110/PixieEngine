using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Diagnostics;

namespace PixieEditor.Native;

// CMake owns compiler settings. This only connects the IDE to the editor's existing build.
internal static class CodeWorkspace
{
    internal const string PresetName = "pixie-editor-local";
    private static readonly JsonDocumentOptions JsonOptions = new() { CommentHandling = JsonCommentHandling.Skip, AllowTrailingCommas = true };
    private static readonly JsonSerializerOptions Pretty = new() { WriteIndented = true };

    internal static void Prepare(string folder, string engine, string cmake) =>
        Prepare(folder, cmake, engine, null, null);

    internal static void Prepare(ProjectBuild.BuildPlan plan, Func<bool>? presetExists = null) =>
        Prepare(plan.Folder, plan.CMake, null, plan, presetExists);

    private static void Prepare(string folder, string cmake, string? engine, ProjectBuild.BuildPlan? plan, Func<bool>? presetExists)
    {
        try { PrepareCore(Path.GetFullPath(folder), Path.GetFullPath(cmake), engine, plan, presetExists); }
        catch (Exception error) when (error is JsonException or IOException or UnauthorizedAccessException or InvalidOperationException or ArgumentException or System.ComponentModel.Win32Exception)
        { throw new EngineException($"Could not prepare CMake Tools: {error.Message}. Review the project configuration and any preimages before retrying, or configure CMake Tools manually."); }
    }

    private static void PrepareCore(string folder, string cmake, string? engine, ProjectBuild.BuildPlan? plan, Func<bool>? presetExists)
    {
        var settingsPath = ProjectCode.CheckedPath(folder, ".vscode/settings.json");
        var presetsPath = ProjectCode.CheckedPath(folder, "CMakeUserPresets.json");
        var cachePath = ProjectCode.CheckedPath(plan?.Cache ?? folder, "build/CMakeCache.txt");
        if (!File.Exists(cachePath)) throw new EngineException("Build this project's code from the editor once before configuring CMake Tools.");
        var cache = File.ReadLines(cachePath).Where(line => !line.StartsWith('#') && !line.StartsWith("//") && line.Contains(':') && line.Contains('='))
            .Select(line => (Name: line[..line.IndexOf(':')], Value: line[(line.IndexOf('=') + 1)..]))
            .ToDictionary(pair => pair.Name, pair => pair.Value, StringComparer.Ordinal);
        if (!cache.TryGetValue("CMAKE_GENERATOR", out var generator) || string.IsNullOrWhiteSpace(generator))
            throw new EngineException("The project's CMake cache has no generator. Rebuild its code from the editor first.");
        if (!cache.TryGetValue("CMAKE_HOME_DIRECTORY", out var home) || !(plan == null ? SamePath(home, folder) : SameAbsolutePath(home, folder)))
            throw new EngineException("The project's build cache belongs to another source folder. Configure its build before opening code.");

        var build = Path.GetDirectoryName(cachePath)!;
        var cacheVariables = plan == null
            ? new JsonObject { ["PIXIE_ENGINE_DIR"] = Path.GetFullPath(engine!).Replace('\\', '/') }
            : InstalledVariables(plan, cache, build);

        var settingsBefore = Read(settingsPath);
        using var settings = Parse(settingsBefore ?? "{}"u8.ToArray());
        RequireObject(settings.RootElement);
        var defaults = new JsonObject
        {
            ["C_Cpp.default.configurationProvider"] = "ms-vscode.cmake-tools",
            ["cmake.useCMakePresets"] = "always",
            ["cmake.cmakePath"] = cmake.Replace('\\', '/'),
            ["cmake.configureOnOpen"] = true
        };
        foreach (var key in new[] { "C_Cpp.default.configurationProvider", "cmake.useCMakePresets" })
            if (settings.RootElement.TryGetProperty(key, out var value) && (value.ValueKind != JsonValueKind.String || value.GetString() != defaults[key]!.GetValue<string>()))
                throw new EngineException($"Existing {key} is retained. Select CMake Tools as the C/C++ provider and enable CMake presets to use {PresetName}.");
        if (settings.RootElement.TryGetProperty("cmake.sourceDirectory", out var source) &&
            (source.ValueKind != JsonValueKind.String || source.GetString() is not ("${workspaceFolder}" or "${workspaceFolder}/") && !SamePath(source.GetString()!, folder)))
            throw new EngineException("Existing cmake.sourceDirectory is retained. Configure its custom source directory manually.");
        if (plan != null && settings.RootElement.TryGetProperty("cmake.cmakePath", out var executable) &&
            (executable.ValueKind != JsonValueKind.String || !SameAbsolutePath(executable.GetString()!, cmake)))
            throw new EngineException("Existing cmake.cmakePath is retained. Select the CMake executable used by the installed SDK build.");
        // CMake Tools appends these arguments to the preset, where they can override the owned cache.
        if (plan != null && settings.RootElement.TryGetProperty("cmake.configureArgs", out var arguments) &&
            (arguments.ValueKind != JsonValueKind.Array || arguments.GetArrayLength() != 0))
            throw new EngineException("Existing cmake.configureArgs is retained. Review the extra configure arguments before using the installed SDK preset.");
        foreach (var property in settings.RootElement.EnumerateObject()) defaults.Remove(property.Name);
        var settingsAfter = AddProperties(settingsBefore ?? "{}"u8.ToArray(), defaults);

        var preset = new JsonObject
        {
            ["name"] = PresetName,
            ["displayName"] = "Pixie Editor (local)",
            ["generator"] = generator,
            ["binaryDir"] = plan == null ? "${sourceDir}/build" : PresetPath(build),
            ["cacheVariables"] = cacheVariables
        };
        foreach (var (cacheKey, property) in new[] { ("CMAKE_GENERATOR_PLATFORM", "architecture"), ("CMAKE_GENERATOR_TOOLSET", "toolset") })
            if (cache.TryGetValue(cacheKey, out var value) && value.Length > 0) preset[property] = value;
        var presetsBefore = Read(presetsPath);
        var projectPresetsPath = ProjectCode.CheckedPath(folder, "CMakePresets.json");
        // Let CMake resolve includes/inheritance rather than maintaining another preset graph.
        var nameExists = (Read(projectPresetsPath) != null || presetsBefore != null) && (presetExists?.Invoke() ?? ExistingPreset(folder, cmake));
        byte[] presetsAfter;
        if (presetsBefore == null)
        {
            if (nameExists) throw new EngineException($"An existing CMake preset already defines {PresetName}; select it manually. Configuration was not changed.");
            presetsAfter = JsonSerializer.SerializeToUtf8Bytes(new JsonObject { ["version"] = 6, ["configurePresets"] = new JsonArray(preset) }, Pretty);
        }
        else
        {
            using var existing = Parse(presetsBefore);
            RequireObject(existing.RootElement);
            if (!existing.RootElement.TryGetProperty("version", out var version) || !version.TryGetInt32(out var number) || number < 1)
                throw new EngineException("CMakeUserPresets.json has no valid version; it was not changed.");
            if (FindPreset(existing.RootElement) is { } found)
            {
                if (!Compatible(found, preset, plan != null) || (plan != null && !nameExists))
                    throw new EngineException($"Existing preset {PresetName} has different settings; it was not changed. Select/configure your preset in CMake Tools.");
                presetsAfter = presetsBefore;
            }
            else if (existing.RootElement.TryGetProperty("configurePresets", out var array))
            {
                if (nameExists) throw new EngineException($"An included preset already defines {PresetName}; configuration was not changed.");
                if (array.ValueKind != JsonValueKind.Array) throw new EngineException("Existing configurePresets is not an array; it was not changed.");
                presetsAfter = Insert(presetsBefore, "configurePresets", JsonSerializer.Serialize(preset, Pretty), array.GetArrayLength() > 0);
            }
            else
            {
                if (nameExists) throw new EngineException($"An included preset already defines {PresetName}; configuration was not changed.");
                presetsAfter = AddProperties(presetsBefore, new JsonObject { ["configurePresets"] = new JsonArray(preset) });
            }
        }

        // Preflight both files before writing either. Backups are plain preimages, not another project model.
        var backup = ProjectCode.CheckedPath(folder, ".local/code-editor-backups/" + Guid.NewGuid().ToString("N"));
        foreach (var (path, before, after) in new[] { (presetsPath, presetsBefore, presetsAfter), (settingsPath, settingsBefore, settingsAfter) })
        {
            if (before != null && before.AsSpan().SequenceEqual(after)) continue;
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            using var file = new FileStream(path, before == null ? FileMode.CreateNew : FileMode.Open, FileAccess.ReadWrite, FileShare.None);
            if (before != null)
            {
                using var current = new MemoryStream(); file.CopyTo(current);
                if (!current.ToArray().AsSpan().SequenceEqual(before)) throw new EngineException("IDE settings changed during preparation; no replacement was made.");
                Directory.CreateDirectory(backup);
                File.WriteAllBytes(Path.Combine(backup, Path.GetFileName(path)), before);
            }
            try { file.Position = 0; file.Write(after); file.SetLength(file.Position); file.Flush(true); }
            catch (IOException error) { throw new EngineException($"Could not finish IDE setup: {error.Message}. Existing preimages, when applicable, are in {backup}."); }
        }
    }

    private static JsonObject InstalledVariables(ProjectBuild.BuildPlan plan, Dictionary<string, string> cache, string build)
    {
        plan.Sdk.VerifyUnchanged();
        var owner = ProjectCode.CheckedPath(plan.Cache, "owner.json");
        if (!File.Exists(owner) || !EngineSdk.ReadBytes(owner, 64 * 1024).AsSpan().SequenceEqual(plan.Ownership))
            throw new EngineException("The installed build cache is unowned or belongs to another build. Rebuild this project's code from the editor.");
        if (!cache.TryGetValue("CMAKE_CACHEFILE_DIR", out var directory) || !SameAbsolutePath(directory, build) ||
            !cache.TryGetValue("CMAKE_COMMAND", out var command) || !SameAbsolutePath(command, plan.CMake))
            throw new EngineException("The installed build cache was configured in another directory or with another CMake executable.");

        var variables = new JsonObject();
        foreach (var argument in plan.Configure.Where(argument => argument.StartsWith("-D", StringComparison.Ordinal)))
        {
            var pair = argument[2..].Split('=', 2);
            var key = pair[0];
            var expected = pair[1];
            var path = Path.IsPathFullyQualified(expected);
            if (!cache.TryGetValue(key, out var configured) || !(path ? SameAbsolutePath(configured, expected) : configured == expected))
                throw new EngineException($"The installed build cache differs from the selected SDK build at {key}. Rebuild this project's code from the editor.");
            variables[key] = path ? PresetPath(expected) : expected;
        }
        return variables;
    }

    private static bool ExistingPreset(string folder, string cmake)
    {
        var info = new ProcessStartInfo(cmake, ["--list-presets=configure"]) { WorkingDirectory = folder, UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
        using var process = Process.Start(info) ?? throw new EngineException("Could not inspect existing CMake presets.");
        var output = process.StandardOutput.ReadToEndAsync(); var error = process.StandardError.ReadToEndAsync();
        if (!process.WaitForExit(15_000)) { process.Kill(entireProcessTree: true); throw new EngineException("Inspecting existing CMake presets timed out; configuration was not changed."); }
        if (process.ExitCode != 0) throw new EngineException("CMake could not read existing presets; configuration was not changed. " + error.GetAwaiter().GetResult().Trim());
        return output.GetAwaiter().GetResult().Split('\n').Any(line => line.TrimStart().StartsWith("\"" + PresetName + "\"", StringComparison.Ordinal));
    }

    private static bool SamePath(string left, string right) => Path.GetFullPath(left).TrimEnd('/', '\\').Equals(Path.GetFullPath(right).TrimEnd('/', '\\'), OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal);
    private static bool SameAbsolutePath(string left, string right) => Path.IsPathFullyQualified(left) && SamePath(left, right);
    private static string PresetPath(string path) => path.Replace('\\', '/').Replace("$", "${dollar}");
    private static byte[]? Read(string path)
    {
        if (!File.Exists(path)) return null;
        if (new FileInfo(path).Length > 1024 * 1024) throw new EngineException("IDE configuration exceeds the 1 MiB limit; it was not changed.");
        return File.ReadAllBytes(path);
    }
    private static int Bom(byte[] bytes) => bytes.AsSpan().StartsWith(new byte[] { 239, 187, 191 }) ? 3 : 0;
    private static JsonDocument Parse(byte[] bytes) => JsonDocument.Parse(bytes.AsMemory(Bom(bytes)), JsonOptions);
    private static void RequireObject(JsonElement element)
    {
        if (element.ValueKind != JsonValueKind.Object || element.EnumerateObject().Select(p => p.Name).Distinct().Count() != element.EnumerateObject().Count())
            throw new EngineException("IDE configuration must be an object without duplicate keys; it was not changed.");
    }
    private static JsonElement? FindPreset(JsonElement root)
    {
        RequireObject(root);
        if (!root.TryGetProperty("configurePresets", out var presets)) return null;
        if (presets.ValueKind != JsonValueKind.Array) throw new EngineException("configurePresets must be an array.");
        JsonElement? found = null;
        foreach (var preset in presets.EnumerateArray())
            if (preset.ValueKind == JsonValueKind.Object && preset.TryGetProperty("name", out var name) && name.ValueKind == JsonValueKind.String && name.GetString() == PresetName)
            { if (found != null) throw new EngineException("Duplicate Pixie IDE presets; configuration was not changed."); found = preset; }
        return found;
    }
    private static bool Compatible(JsonElement existing, JsonObject desired, bool installed)
    {
        RequireObject(existing);
        if (installed && existing.EnumerateObject().Any(property => !desired.ContainsKey(property.Name) && property.Name is not ("description" or "vendor")))
            return false;
        foreach (var key in new[] { "generator", "binaryDir", "architecture", "toolset" })
            if (desired.ContainsKey(key) && (!existing.TryGetProperty(key, out var value) || !JsonNode.DeepEquals(JsonNode.Parse(value.GetRawText()), desired[key]))) return false;
        if (!existing.TryGetProperty("cacheVariables", out var variables) || variables.ValueKind != JsonValueKind.Object) return false;
        if (installed) return JsonNode.DeepEquals(JsonNode.Parse(variables.GetRawText()), desired["cacheVariables"]);
        return variables.TryGetProperty("PIXIE_ENGINE_DIR", out var engine) && engine.ValueKind == JsonValueKind.String &&
               SamePath(engine.GetString()!, desired["cacheVariables"]!["PIXIE_ENGINE_DIR"]!.GetValue<string>());
    }
    private static byte[] AddProperties(byte[] bytes, JsonObject additions)
    {
        if (additions.Count == 0) return bytes;
        using var document = Parse(bytes);
        var json = JsonSerializer.Serialize(additions, Pretty);
        return Insert(bytes, null, json[1..^1], document.RootElement.EnumerateObject().Any());
    }
    private static byte[] Insert(byte[] bytes, string? arrayProperty, string content, bool hasExisting)
    {
        var offset = Bom(bytes);
        var reader = new Utf8JsonReader(bytes.AsSpan(offset), new JsonReaderOptions { CommentHandling = JsonCommentHandling.Skip, AllowTrailingCommas = true });
        reader.Read();
        if (arrayProperty != null)
        {
            while (reader.Read())
                if (reader.TokenType == JsonTokenType.PropertyName && reader.CurrentDepth == 1 && reader.ValueTextEquals(arrayProperty)) { reader.Read(); break; }
            if (reader.TokenType != JsonTokenType.StartArray) throw new EngineException("Could not locate configurePresets safely.");
        }
        var at = offset + checked((int)reader.TokenStartIndex) + 1;
        var added = Encoding.UTF8.GetBytes("\n" + content + (hasExisting ? "," : "") + "\n");
        return [.. bytes.AsSpan(0, at), .. added, .. bytes.AsSpan(at)];
    }
}
