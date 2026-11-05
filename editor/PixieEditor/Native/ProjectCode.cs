using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;

namespace PixieEditor.Native;

// Owns generated registration only. User component implementations are written once, never regenerated.
internal static class ProjectCode
{
    private const string Header = "src/PixieComponents.generated.hpp";
    private const string Registry = "src/PixieComponents.generated.cpp";
    private const string Sources = ".pixie-code/Components.cmake";
    private const string Manifest = ".pixie-code/manifest.json";
    private const string CmakeBlock = "# BEGIN PIXIE COMPONENT SOURCES (managed)\npixie_project_components()\n# END PIXIE COMPONENT SOURCES (managed)";
    private static readonly HashSet<string> Keywords = new(("alignas alignof and and_eq asm atomic_cancel atomic_commit atomic_noexcept auto bitand bitor bool break case catch char char8_t char16_t char32_t class compl concept const consteval constexpr constinit const_cast continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected public reflexpr register reinterpret_cast requires return short signed sizeof static static_assert static_cast struct switch synchronized template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq module import").Split(' '), StringComparer.Ordinal);

    internal static bool ValidName(string? name) => name is not null && name.Length is > 0 and <= 64
        && Regex.IsMatch(name, "^[A-Za-z][A-Za-z0-9_]*$", RegexOptions.CultureInvariant)
        && !name.Contains("__", StringComparison.Ordinal) && !Keywords.Contains(name);

    internal static string CheckedPath(string folder, string relative)
    {
        var root = Path.GetFullPath(folder);
        if (string.IsNullOrWhiteSpace(relative) || relative.Any(char.IsControl))
            throw new EngineException("A code path must be a nonempty project-relative path without control characters.");
        if (Path.IsPathRooted(relative) || relative.Split('/', '\\').Any(p => p is ".." or "."))
            throw new EngineException("Code paths must be explicit paths inside this project.");
        var full = Path.GetFullPath(Path.Combine(root, relative));
        if (!full.StartsWith(root.TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar,
                OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal))
            throw new EngineException("The code path leaves the project folder.");
        var current = root;
        foreach (var part in Path.GetRelativePath(root, full).Split(Path.DirectorySeparatorChar))
        {
            current = Path.Combine(current, part);
            if (current != full && File.Exists(current)) throw new EngineException($"A code directory is an existing file: {relative}");
            if ((File.Exists(current) || Directory.Exists(current)) && File.GetAttributes(current).HasFlag(FileAttributes.ReparsePoint))
                throw new EngineException($"Code paths cannot pass through linked files or folders: {relative}");
        }
        return full;
    }

    internal static string ResolveSource(string folder, string relative)
    {
        if (string.IsNullOrWhiteSpace(relative)) throw new EngineException("This component has no project-code source metadata.");
        var source = CheckedPath(folder, relative);
        if (!new[] { ".cpp", ".cc", ".cxx", ".hpp", ".h" }.Contains(Path.GetExtension(source), StringComparer.OrdinalIgnoreCase)
            || !File.Exists(source))
            throw new EngineException($"The component's declared source is missing or is not a C++ file: {relative}");
        return source;
    }

    private static string Hash(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes));
    private static string Normalized(string text) => text.Replace("\r\n", "\n");
    private static string Templates => Path.Combine(EngineFolder.Find(), "templates", "project-code");

    private static (JsonObject Metadata, byte[] Bytes) ReadMetadata(string folder)
    {
        var path = CheckedPath(folder, Manifest);
        if (!File.Exists(path) || new FileInfo(path).Length > 4 * 1024 * 1024)
            throw new EngineException("Component registry metadata is missing or too large.");
        var bytes = File.ReadAllBytes(path);
        JsonObject? metadata;
        try { metadata = JsonNode.Parse(bytes) as JsonObject; }
        catch (JsonException error) { throw new EngineException($"Invalid component registry JSON: {error.Message}"); }
        bool Text(string key, out string value)
        {
            value = "";
            return metadata?[key] is JsonValue item && item.TryGetValue<string>(out value!);
        }
        if (metadata == null || metadata["schemaVersion"] is not JsonValue version || !version.TryGetValue<int>(out var schema) || schema != 1
            || !Text("projectId", out var id) || !Regex.IsMatch(id, "^[A-Za-z][A-Za-z0-9_]*$")
            || metadata["components"] is not JsonArray names || names.Count > 4096
            || names.Any(node => node is not JsonValue item || !item.TryGetValue<string>(out var name) || !ValidName(name))
            || !Text("headerHash", out var header) || !Regex.IsMatch(header, "^[A-Fa-f0-9]{64}$")
            || !Text("sourceHash", out var source) || !Regex.IsMatch(source, "^[A-Fa-f0-9]{64}$")
            || !Text("cmakeHash", out var cmake) || !Regex.IsMatch(cmake, "^[A-Fa-f0-9]{64}$"))
            throw new EngineException("Unsupported or malformed component registry schema. No component code was written.");
        return (metadata, bytes);
    }

    private static (string Id, string Name) ProjectIdentity(string folder)
    {
        var files = Directory.GetFiles(folder, "*.pixie");
        if (files.Length != 1) throw new EngineException("The project must contain exactly one .pixie file.");
        using var project = JsonDocument.Parse(File.ReadAllText(files[0]));
        var name = project.RootElement.TryGetProperty("name", out var value) && value.ValueKind == JsonValueKind.String
            ? value.GetString()! : Path.GetFileNameWithoutExtension(files[0]);
        var id = string.Concat(Regex.Matches(name, "[A-Za-z0-9]+").Select(m => char.ToUpperInvariant(m.Value[0]) + m.Value[1..]));
        if (id.Length == 0 || char.IsDigit(id[0])) id = "Game" + id;
        return (id, name);
    }

    // New projects use Add code's exact templates. Older untouched templates can be adopted;
    // unfamiliar registration layouts require explicit wiring, never filename-based guessing.
    private static void EnsureRegistry(string folder)
    {
        if (File.Exists(CheckedPath(folder, Manifest))) return;
        if (!ProjectBuild.HasCode(folder)) { ProjectBuild.AddCode(folder); return; }
        var (id, name) = ProjectIdentity(folder);
        string Render(string file) => File.ReadAllText(Path.Combine(Templates, file))
            .Replace("@PIXIE_ID@", id).Replace("@PIXIE_NAME_LITERAL@", name.Replace("\\", "\\\\").Replace("\"", "\\\""));
        var legacy = new Dictionary<string, string>
        {
            ["CMakeLists.txt"] = Render("legacy/CMakeLists.txt.in"),
            [$"src/{id}Project.hpp"] = Render("legacy/Project.hpp.in"),
            [$"src/{id}Project.cpp"] = Render("legacy/Project.cpp.in")
        };
        var legacyHashes = new Dictionary<string, string>();
        var matches = true;
        foreach (var pair in legacy)
        {
            var path = CheckedPath(folder, pair.Key);
            if (!File.Exists(path)) { matches = false; continue; }
            var bytes = File.ReadAllBytes(path);
            legacyHashes[pair.Key] = Hash(bytes);
            if (Normalized(Encoding.UTF8.GetString(bytes)) != Normalized(pair.Value)) matches = false;
        }
        if (!matches)
            throw new EngineException("This custom project has no generated-component registry. Preserve its code and wire pixie::generated::DescribeComponents plus pixie_project_components explicitly, then run cmake/PixieInitComponents.cmake as described in docs/entity-input-and-code-components.md. No existing file was changed.");
        var changes = new Dictionary<string, string>
        {
            ["CMakeLists.txt"] = Render("CMakeLists.txt.in"),
            [$"src/{id}Project.cpp"] = Render("Project.cpp.in"),
            [Header] = Render("Components.hpp.in"), [Registry] = Render("Components.cpp.in"), [Sources] = Render("Components.cmake.in")
        };
        foreach (var path in new[] { Header, Registry, Sources, Manifest })
            if (File.Exists(CheckedPath(folder, path))) throw new EngineException($"Refusing to replace an unowned generated-code file: {path}");
        var metadata = new JsonObject { ["schemaVersion"] = 1, ["projectId"] = id, ["components"] = new JsonArray() };
        UpdateHashes(metadata, changes);
        changes[Manifest] = metadata.ToJsonString(new() { WriteIndented = true }) + "\n";
        WriteChanges(folder, changes, legacyHashes);
    }

    private static (string Header, string Source) ReadInstalledComponentTemplates(EngineSdk.ProjectSupport support)
    {
        try
        {
            using var versions = EngineSdk.Parse(EngineSdk.ReadBytes(support.TemplateVersions, 1024 * 1024));
            var inventory = versions.RootElement;
            EngineSdk.Schema(inventory);
            var family = inventory.GetProperty("families").GetProperty("sdk-v1");
            if (EngineSdk.Text(inventory, "hashPolicy") != "utf8-lf" || family.GetProperty("registrySchema").GetInt32() != 1)
                throw new EngineException("The selected SDK has an unsupported component template inventory.");
            var templates = family.GetProperty("templates").EnumerateArray().Select(x => x.GetString()).ToArray();
            string ReadTemplate(string name, string path)
            {
                var text = Normalized(new UTF8Encoding(false, true).GetString(EngineSdk.ReadBytes(path, 1024 * 1024)));
                if (!templates.Contains(name, StringComparer.Ordinal) ||
                    EngineSdk.HashBytes(Encoding.UTF8.GetBytes(text)) != EngineSdk.Text(inventory.GetProperty("sha256"), name))
                    throw new EngineException($"The selected SDK's sdk-v1 component template is missing or changed: {name}.");
                return text;
            }
            return (ReadTemplate("Component.hpp.in", support.ComponentHeader), ReadTemplate("Component.cpp.in", support.ComponentSource));
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or
                                      InvalidOperationException or ArgumentException)
        { throw new EngineException($"Could not read the installed component templates: {error.Message}"); }
    }

    internal static string CreateComponent(string folder, string name) => CreateComponent(folder, name, GameLibrary.Selection);

    internal static string CreateComponent(string folder, string name, EngineSdk? sdk)
    {
        if (!ValidName(name)) throw new EngineException("Use a C++ identifier of at most 64 letters/digits/underscores, starting with a letter, without reserved keywords or double underscores.");
        var id = File.Exists(CheckedPath(folder, Manifest)) ? ReadMetadata(folder).Metadata["projectId"]!.GetValue<string>() : ProjectIdentity(folder).Id;
        if (name == "Spinner" || name == id + "Project" || new[] { "pixie", "std", "entt", "pxPoint", "pxVec2", "pxVec3", "pxVec4", "pxRect", "pxSize", "pxColorRgba" }.Contains(name))
            throw new EngineException("This name is reserved by the project templates or the engine's public C++ symbols. Choose another component name; no file was changed.");
        // Check these before Add code, so a collision cannot create an unrelated base project.
        var componentHeader = $"src/components/{name}.hpp";
        var componentSource = $"src/components/{name}.cpp";
        foreach (var relative in new[] { componentHeader, componentSource })
            if (File.Exists(CheckedPath(folder, relative)) || Directory.Exists(CheckedPath(folder, relative)))
                throw new EngineException($"Refusing to overwrite existing component code: {relative}");
        CheckedPath(folder, Header); CheckedPath(folder, Sources);
        (string Header, string Source)? installed = null;
        if (sdk == null) EnsureRegistry(folder);
        else
        {
            var support = ProjectBuild.RequireSupport(folder, sdk);
            installed = ReadInstalledComponentTemplates(support);
            if (!File.Exists(CheckedPath(folder, Manifest)))
            {
                if (ProjectBuild.HasCode(folder))
                    throw new EngineException("This installed SDK project has no owned sdk-v1 registry. Its existing code and CMake were preserved.");
                ProjectBuild.AddCode(folder, sdk);
            }
            try { _ = ProjectBuild.ReadTarget(folder, support); }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or InvalidOperationException or ArgumentException)
            { throw new EngineException($"Could not verify the installed project registry: {error.Message}"); }
        }
        var (metadata, manifestBytes) = ReadMetadata(folder);
        var components = (JsonArray)metadata["components"]!;
        var names = components.Select(node => node!.GetValue<string>()).ToList();
        if (names.Count > 4096 || names.Any(n => !ValidName(n)) || names.Distinct().Count() != names.Count || names.Contains(name))
            throw new EngineException("Component registry has invalid/duplicate names, or this component already exists.");
        foreach (var (relative, key) in new[] { (Header, "headerHash"), (Registry, "sourceHash"), (Sources, "cmakeHash") })
            if (!File.Exists(CheckedPath(folder, relative)) || !string.Equals(Hash(File.ReadAllBytes(CheckedPath(folder, relative))), metadata[key]?.GetValue<string>(), StringComparison.OrdinalIgnoreCase))
                throw new EngineException($"Generated registry was modified: {relative}. Preserve those edits before regenerating; user component files remain editable.");
        var cmake = Normalized(File.ReadAllText(CheckedPath(folder, "CMakeLists.txt")));
        if (cmake.Split(CmakeBlock, StringSplitOptions.None).Length != 2)
            throw new EngineException("The project's managed component CMake block is missing, duplicated or changed. Restore its explicit wiring before adding components.");
        var expected = new Dictionary<string, string>
        {
            [Registry] = metadata["sourceHash"]!.GetValue<string>(),
            [Sources] = metadata["cmakeHash"]!.GetValue<string>(),
            [Manifest] = Hash(manifestBytes)
        };
        names.Add(name);
        var source = new StringBuilder("// Generated registry. Edit component descriptions in their own C++ sources.\n#include \"PixieComponents.generated.hpp\"\n#include \"pixie/runtime/ComponentTypes.hpp\"\n");
        foreach (var component in names) source.AppendLine($"#include \"components/{component}.hpp\"");
        source.AppendLine("namespace pixie::generated {\nvoid DescribeComponents(ComponentTypes& types) {");
        foreach (var component in names) source.AppendLine($"    ::{component}::Describe(types);");
        source.AppendLine("}\n}");
        var sources = new StringBuilder("# Generated source registry; user component implementations remain editable.\ntarget_sources(${PIXIE_COMPONENT_TARGET} PRIVATE\n    \"${CMAKE_CURRENT_SOURCE_DIR}/src/PixieComponents.generated.cpp\"\n");
        foreach (var component in names) sources.AppendLine($"    \"${{CMAKE_CURRENT_SOURCE_DIR}}/src/components/{component}.cpp\"");
        sources.AppendLine(")");
        var changes = new Dictionary<string, string>
        {
            [componentHeader] = (installed?.Header ?? File.ReadAllText(Path.Combine(Templates, "Component.hpp.in"))).Replace("@PIXIE_COMPONENT@", name),
            [componentSource] = (installed?.Source ?? File.ReadAllText(Path.Combine(Templates, "Component.cpp.in"))).Replace("@PIXIE_COMPONENT@", name),
            [Registry] = Normalized(source.ToString()), [Sources] = Normalized(sources.ToString())
        };
        components.Add(name); UpdateHashes(metadata, changes);
        changes[Manifest] = metadata.ToJsonString(new() { WriteIndented = true }) + "\n";
        WriteChanges(folder, changes, expected);
        return componentSource;
    }

    private static void UpdateHashes(JsonObject metadata, Dictionary<string, string> changes)
    {
        foreach (var (path, key) in new[] { (Header, "headerHash"), (Registry, "sourceHash"), (Sources, "cmakeHash") })
            if (changes.TryGetValue(path, out var text)) metadata[key] = Hash(Encoding.UTF8.GetBytes(text));
    }

    private static void WriteChanges(string folder, Dictionary<string, string> changes, IReadOnlyDictionary<string, string> expected)
    {
        // A durable preimage exists before replacing owned files. New source destinations use
        // CreateNew; an unexpected collision never truncates an unknown file.
        foreach (var relative in changes.Keys)
        {
            var target = CheckedPath(folder, relative);
            if (expected.TryGetValue(relative, out var hash))
            {
                if (!File.Exists(target) || !string.Equals(Hash(File.ReadAllBytes(target)), hash, StringComparison.OrdinalIgnoreCase))
                    throw new EngineException($"Code changed during generation: {relative}. No generated output was committed.");
            }
            else if (File.Exists(target) || Directory.Exists(target))
                throw new EngineException($"Refusing to overwrite an unowned code destination: {relative}");
        }
        var backup = CheckedPath(folder, $".pixie-code/backups/{Guid.NewGuid():N}");
        Directory.CreateDirectory(backup);
        foreach (var (relative, _) in changes)
        {
            var target = CheckedPath(folder, relative);
            if (File.Exists(target))
            {
                var saved = Path.Combine(backup, relative); Directory.CreateDirectory(Path.GetDirectoryName(saved)!);
                File.Copy(target, saved, overwrite: false);
            }
        }
        try
        {
            foreach (var (relative, text) in changes)
            {
                var target = CheckedPath(folder, relative);
                Directory.CreateDirectory(Path.GetDirectoryName(target)!);
                var existed = expected.TryGetValue(relative, out var hash);
                using var file = new FileStream(target, existed ? FileMode.Open : FileMode.CreateNew, FileAccess.ReadWrite, FileShare.None);
                if (existed)
                {
                    if (!string.Equals(Convert.ToHexString(SHA256.HashData(file)), hash, StringComparison.OrdinalIgnoreCase))
                        throw new IOException($"Code changed before replacement: {relative}");
                    file.Position = 0; file.SetLength(0);
                }
                var bytes = Encoding.UTF8.GetBytes(text); file.Write(bytes); file.Flush(flushToDisk: true);
            }
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            throw new EngineException($"Component generation did not finish. Existing-file preimages are preserved at {backup}; inspect the partial outputs before retrying. {error.Message}");
        }
    }
}
