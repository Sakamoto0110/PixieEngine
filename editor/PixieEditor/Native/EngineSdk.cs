using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace PixieEditor.Native;

// One validated installed selection. Package provenance and C++ compatibility are separate axes.
internal sealed class EngineSdk
{
    internal static readonly IReadOnlyList<string> NativeMatchFields = Array.AsReadOnly<string>([
        "schemaVersion", "sdkId", "sdkBuildId", "cApiVersion", "moduleIdentityContract",
        "platform", "architecture", "compilerId", "compilerVersion", "msvcFullVersion",
        "stlVersion", "stlUpdate", "iteratorDebugLevel", "cppStandard", "configuration", "crt",
        "cpuFeatures", "profile", "distributionClass", "exceptions", "rtti", "windowsSdkVersion",
        "dependencyInventorySha256"]);
    internal sealed record Payload(string Path, long Size, string Sha256, string Role);
    internal sealed record ExportTool(string Path, string ShippingBuildId);
    internal sealed record ProjectSupport(string Generator, string CMakeTemplate, string TemplateVersions, string ComponentHeader, string ComponentSource);
    public string Root { get; }
    public string ManifestSha256 { get; }
    public string Id { get; }
    public string BuildId { get; }
    public string ProductVersion { get; }
    public string DistributionClass { get; }
    public string Profile { get; }
    public string Configuration { get; }
    public string ProfileSha256 { get; }
    public string Origin { get; }
    public string RuntimePath { get; }
    public string RuntimeSha256 { get; }
    public JsonElement Identity { get; }
    public ulong CapabilityBits { get; }
    internal ExportTool? Exporter { get; }
    internal ProjectSupport? Generation { get; }
    public string Summary => $"PixieEngine {ProductVersion} · {Profile} · {Configuration}";

    private EngineSdk(string root, string manifestHash, JsonElement manifest, JsonElement profile,
        string profileHash, string origin, ExportTool? exporter, ProjectSupport? generation)
    {
        Root = root; ManifestSha256 = manifestHash; Id = Text(manifest, "sdkId");
        BuildId = Text(profile, "sdkBuildId"); ProductVersion = Text(manifest, "productVersion");
        DistributionClass = Text(manifest, "distributionClass"); Profile = Text(profile, "profile");
        Configuration = Text(profile, "configuration"); ProfileSha256 = profileHash; Origin = origin;
        Identity = profile.Clone();
        var runtime = profile.GetProperty("runtime");
        RuntimePath = CheckedPath(root, Text(runtime, "path")); RuntimeSha256 = Text(runtime, "sha256");
        CapabilityBits = Bits(profile.GetProperty("capabilities"));
        Exporter = exporter;
        Generation = generation;
    }

    // Explicit launch/environment input never falls through to a remembered UI preference.
    internal static (string Root, string Origin)? Override(string? launch, string? environment) =>
        launch != null ? (launch, "launch") : environment != null ? (environment, "environment") : null;

    internal static EngineSdk Load(string root, string profile = "authoring", string configuration = "Release",
        string origin = "ui", string? expectedId = null, string? expectedBuildId = null, string? expectedManifestSha256 = null)
    {
        try { return LoadCore(root, profile, configuration, origin, expectedId, expectedBuildId, expectedManifestSha256); }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException
            or KeyNotFoundException or InvalidOperationException or ArgumentException or OverflowException or FormatException)
        { throw new EngineException($"Cannot select this Engine SDK: {error.Message}"); }
    }

    private static EngineSdk LoadCore(string folder, string selectedProfile, string configuration, string origin,
        string? expectedId, string? expectedBuildId, string? expectedManifestSha256)
    {
        if (string.IsNullOrWhiteSpace(folder)) throw new EngineException("Choose an installed Engine SDK folder.");
        var root = Path.TrimEndingDirectorySeparator(Path.GetFullPath(folder));
        if (!Directory.Exists(root)) throw new EngineException($"The Engine SDK folder does not exist: {root}");
        NoLink(root);
        var manifestPath = CheckedPath(root, "sdk.json");
        var manifestBytes = ReadBytes(manifestPath, 16 * 1024 * 1024);
        var manifestHash = HashBytes(manifestBytes);
        if (expectedManifestSha256 != null && manifestHash != expectedManifestSha256)
            throw new EngineException("The selected SDK manifest changed across the Editor restart.");
        using var document = Parse(manifestBytes);
        var manifest = document.RootElement;
        Schema(manifest); HashText(manifest, "sdkId");
        if (!Regex.IsMatch(Text(manifest, "productVersion"), @"^\d+\.\d+\.\d+\.\d+$"))
            throw new EngineException("The SDK has no valid Engine product version.");
        var distribution = Text(manifest, "distributionClass");
        if (distribution is not ("public" or "local-private")) throw new EngineException("Unknown SDK distribution class.");
        if (expectedId != null && Text(manifest, "sdkId") != expectedId)
            throw new EngineException("The selected SDK identity changed. Choose the expected SDK explicitly.");
        var files = new Dictionary<string, Payload>(StringComparer.OrdinalIgnoreCase);
        foreach (var item in manifest.GetProperty("files").EnumerateArray())
        {
            var relative = Text(item, "path"); var path = CheckedPath(root, relative);
            HashText(item, "sha256");
            var size = item.GetProperty("size").GetInt64();
            if (size < 0 || relative.Equals("sdk.json", StringComparison.OrdinalIgnoreCase) ||
                !files.TryAdd(relative, new(relative, size, Text(item, "sha256"), Text(item, "role"))))
                throw new EngineException("The SDK inventory has a duplicate, recursive or invalid file entry.");
            if (distribution == "public" && IsPrivatePayload(relative))
                throw new EngineException($"A public SDK contains private automation payload: {relative}");
            if (!File.Exists(path) || new FileInfo(path).Length != size || HashFile(path) != Text(item, "sha256"))
                throw new EngineException($"The SDK payload is missing or differs from its inventory: {relative}");
        }
        var actual = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var directories = new Stack<string>(); directories.Push(root);
        while (directories.TryPop(out var directory))
            foreach (var path in Directory.EnumerateFileSystemEntries(directory))
            {
                NoLink(path);
                if (Directory.Exists(path)) { directories.Push(path); continue; }
                var relative = Path.GetRelativePath(root, path).Replace('\\', '/');
                if (!actual.Add(relative)) throw new EngineException("The SDK has colliding payload paths.");
                if (relative != "sdk.json" && !files.ContainsKey(relative))
                    throw new EngineException($"The SDK contains an undeclared payload: {relative}");
            }
        JsonElement? chosen = null, shipping = null; string? chosenHash = null;
        var tuples = new HashSet<string>(StringComparer.Ordinal);
        foreach (var entry in manifest.GetProperty("profiles").EnumerateArray())
        {
            var profile = Text(entry, "profile"); var config = Text(entry, "configuration");
            if (config is not ("Debug" or "Release") ||
                (distribution == "public" ? profile is not ("authoring" or "shipping") : profile != "local-development") ||
                profile == "shipping" && config != "Release" || !tuples.Add(profile + "/" + config))
                throw new EngineException("The SDK catalog contains an unavailable, duplicate or private/public mixed tuple.");
            var relative = $"profiles/{profile}/{config}/profile.json";
            HashText(entry, "sdkBuildId"); HashText(entry, "manifestSha256");
            if (Text(entry, "manifest") != relative || !files.TryGetValue(relative, out var inventory) ||
                inventory.Sha256 != Text(entry, "manifestSha256"))
                throw new EngineException("The SDK profile manifest disagrees with its catalog/inventory.");
            using var profileDocument = Parse(ReadBytes(CheckedPath(root, relative), 1024 * 1024));
            var identity = profileDocument.RootElement;
            ValidateIdentity(identity);
            if (Text(identity, "sdkId") != Text(manifest, "sdkId") || Text(identity, "sdkBuildId") != Text(entry, "sdkBuildId") ||
                Text(identity, "profile") != profile || Text(identity, "configuration") != config ||
                Text(identity, "distributionClass") != distribution || Text(identity, "productVersion") != Text(manifest, "productVersion"))
                throw new EngineException("The SDK/profile identities are mixed.");
            var bits = Bits(identity.GetProperty("capabilities"));
            if (bits != (profile == "shipping" ? 4UL : profile == "authoring" ? 5UL : 7UL))
                throw new EngineException("The profile's native capabilities do not match its admitted role.");
            CheckReference(identity.GetProperty("runtime"), root, files, ".dll");
            CheckReference(identity.GetProperty("player"), root, files, ".exe");
            if (profile == "shipping") shipping = identity.Clone();
            if (profile == selectedProfile && config == configuration) { chosen = identity.Clone(); chosenHash = inventory.Sha256; }
        }
        if (chosen == null) throw new EngineException($"This SDK has no {selectedProfile}/{configuration} authoring host. Choose an available SDK/profile.");
        if (selectedProfile == "shipping") throw new EngineException("A shipping runtime cannot host the Editor. Choose an authoring SDK.");
#if !PIXIE_DEVELOPMENT
        if (distribution != "public") throw new EngineException("The public Editor cannot activate a private automation SDK.");
#endif
        if (expectedBuildId != null && Text(chosen.Value, "sdkBuildId") != expectedBuildId)
            throw new EngineException("The selected SDK build changed across the Editor restart.");
        return new(root, manifestHash, manifest, chosen.Value, chosenHash!, origin,
            ReadExporter(root, manifest, shipping, files), ReadProjectSupport(root, manifest, files));
    }

    private static ProjectSupport? ReadProjectSupport(string root, JsonElement manifest, IReadOnlyDictionary<string, Payload> files)
    {
        if (!manifest.TryGetProperty("features", out var features) ||
            !features.TryGetProperty("projectGeneration", out var advertised) || !advertised.GetBoolean()) return null;
        const string generator = "cmake/PixieAddCode.cmake";
        const string template = "templates/project-code/sdk/CMakeLists.txt.in";
        const string versions = "templates/project-code/versions.json";
        const string componentHeader = "templates/project-code/Component.hpp.in";
        const string componentSource = "templates/project-code/Component.cpp.in";
        foreach (var path in new[] { generator, template, "cmake/PixieProject.cmake", "cmake/PixieGame.cmake", "cmake/PixieTargetDefaults.cmake",
            "cmake/PixieComponentRegistry.cmake", "lib/cmake/PixieEngine/PixieEngineConfig.cmake", "support/player/PlayerMain.cpp",
            "support/module/GameModuleEntry.cpp", versions, componentHeader, componentSource, "templates/project-code/Project.hpp.in",
            "templates/project-code/Project.cpp.in", "templates/project-code/Components.hpp.in", "templates/project-code/Components.cpp.in",
            "templates/project-code/Components.cmake.in" })
            if (!files.TryGetValue(path, out var payload) || payload.Path != path)
                throw new EngineException($"The selected SDK advertises project generation but omits its declared support: {path}.");
        return new(CheckedPath(root, generator), CheckedPath(root, template), CheckedPath(root, versions),
            CheckedPath(root, componentHeader), CheckedPath(root, componentSource));
    }

    private static ExportTool? ReadExporter(string root, JsonElement manifest, JsonElement? shipping,
        IReadOnlyDictionary<string, Payload> files)
    {
        const string toolDirectory = "tools/export/Release/";
        const string shippingDirectory = "profiles/shipping/Release/bin/";
        var advertised = manifest.TryGetProperty("features", out var features) &&
            features.TryGetProperty("pairedExport", out var flag) && flag.GetBoolean();
        JsonElement entry = default;
        var present = manifest.TryGetProperty("tools", out var tools) && tools.TryGetProperty("exporter", out entry);
        if (!advertised && !present) return null;
        if (!advertised || !present || shipping is not { } paired || Text(manifest, "distributionClass") != "public")
            throw new EngineException("The selected SDK has an inconsistent public paired exporter declaration.");
        Schema(entry);
        var buildId = Text(paired, "sdkBuildId");
        if (Text(entry, "sdkId") != Text(manifest, "sdkId") || Text(entry, "sdkBuildId") != buildId ||
            Text(entry, "profile") != "shipping" || Text(entry, "configuration") != "Release" ||
            entry.GetProperty("descriptorSchema").GetInt32() != 1 || entry.GetProperty("buildProductSchema").GetInt32() != 2 ||
            entry.GetProperty("exportManifestSchema").GetInt32() != 2)
            throw new EngineException("The paired exporter does not match this SDK's shipping build or export schemas.");
        _ = Text(entry, "toolVersion");

        string RequirePayload(JsonElement reference, string expectedPath)
        {
            var path = Text(reference, "path"); HashText(reference, "sha256");
            var digest = Text(reference, "sha256");
            if (path != expectedPath || !files.TryGetValue(path, out var file) || file.Path != path || file.Sha256 != digest)
                throw new EngineException("A paired exporter payload disagrees with the selected SDK inventory.");
            return digest;
        }

        const string executable = toolDirectory + "pixie-export.exe";
        _ = RequirePayload(entry, executable);
        if (RequirePayload(entry.GetProperty("runtime"), toolDirectory + "pixie.dll") !=
            RequirePayload(paired.GetProperty("runtime"), shippingDirectory + "pixie.dll") ||
            paired.GetProperty("externalRuntimeDependencies").GetArrayLength() != 0)
            throw new EngineException("The exporter does not use the contained shipping runtime.");
        var expected = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var dependency in paired.GetProperty("nativeDependencies").EnumerateArray())
        {
            var name = Path.GetFileName(Text(dependency, "path"));
            if (!name.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) || name.Equals("pixie.dll", StringComparison.OrdinalIgnoreCase) ||
                !expected.TryAdd(name, RequirePayload(dependency, shippingDirectory + name)))
                throw new EngineException("The shipping SDK has a duplicate or invalid native dependency.");
        }
        foreach (var dependency in entry.GetProperty("dependencies").EnumerateArray())
        {
            var name = Path.GetFileName(Text(dependency, "path"));
            var crt = name.StartsWith("msvcp", StringComparison.OrdinalIgnoreCase) || name.StartsWith("vcruntime", StringComparison.OrdinalIgnoreCase) ||
                name.StartsWith("concrt", StringComparison.OrdinalIgnoreCase);
            if (!expected.Remove(name, out var digest) || digest != RequirePayload(dependency, toolDirectory + name) ||
                Text(dependency, "kind") != (crt ? "crt" : "native"))
                throw new EngineException("The paired exporter dependency differs from the shipping closure.");
        }
        if (expected.Count != 0) throw new EngineException("The paired exporter omits shipping native dependencies.");
        return new(CheckedPath(root, executable), buildId);
    }

    internal void ValidateRuntime(JsonElement runtime, ulong bits)
    {
        ValidateIdentity(runtime);
        foreach (var field in NativeMatchFields)
            if (!JsonElement.DeepEquals(Identity.GetProperty(field), runtime.GetProperty(field)))
                throw new EngineException($"The loaded runtime differs from the selected SDK: {field}.");
        if (bits != CapabilityBits || Bits(runtime.GetProperty("capabilities")) != bits)
            throw new EngineException("The loaded runtime capabilities differ from the selected SDK.");
    }

    internal void VerifyUnchanged()
    {
        var current = Load(Root, Profile, Configuration, Origin, Id, BuildId);
        if (current.ManifestSha256 != ManifestSha256 || current.ProfileSha256 != ProfileSha256)
            throw new EngineException("The selected SDK changed after selection. Restart and select it again.");
    }

    private static void CheckReference(JsonElement reference, string root, Dictionary<string, Payload> files, string extension)
    {
        var relative = Text(reference, "path"); HashText(reference, "sha256");
        if (!files.TryGetValue(relative, out var file) || file.Sha256 != Text(reference, "sha256") ||
            !Path.GetExtension(CheckedPath(root, relative)).Equals(extension, StringComparison.OrdinalIgnoreCase))
            throw new EngineException("A profile runtime/player does not match the declared inventory.");
    }

    internal static void ValidateIdentity(JsonElement identity)
    {
        Schema(identity); HashText(identity, "sdkId"); HashText(identity, "sdkBuildId"); HashText(identity, "dependencyInventorySha256");
        foreach (var field in NativeMatchFields) _ = identity.GetProperty(field);
        foreach (var field in new[] { "cApiVersion", "moduleIdentityContract", "msvcFullVersion", "stlVersion", "stlUpdate", "iteratorDebugLevel", "cppStandard" })
            if (identity.GetProperty(field).GetInt64() < 0) throw new EngineException($"Invalid identity number: {field}");
        if (identity.GetProperty("cApiVersion").GetInt32() != PixieCapi.Version || NativeApiVersion.IdentitySchema != 1 || identity.GetProperty("moduleIdentityContract").GetInt32() != 1 ||
            identity.GetProperty("cppStandard").GetInt32() != 20 || Text(identity, "platform") != "windows" ||
            Text(identity, "architecture") != "x64" || Text(identity, "compilerId") != "MSVC")
            throw new EngineException("This Editor does not support the selected native API/schema/platform/toolchain.");
        _ = identity.GetProperty("exceptions").GetBoolean(); _ = identity.GetProperty("rtti").GetBoolean();
        foreach (var field in new[] { "compilerVersion", "windowsSdkVersion", "profile", "distributionClass" }) _ = Text(identity, field);
        var config = Text(identity, "configuration");
        if (config is not ("Debug" or "Release") || Text(identity, "crt") != (config == "Debug" ? "MDd" : "MD"))
            throw new EngineException("Invalid native configuration/CRT identity.");
        if (!identity.GetProperty("cpuFeatures").EnumerateArray().Select(x => x.GetString()).SequenceEqual(new[] { "avx2", "fma", "f16c" }))
            throw new EngineException("Unsupported or noncanonical native CPU features.");
    }

    internal static ulong Bits(JsonElement capabilities) =>
        (capabilities.GetProperty("moduleLoading").GetBoolean() ? 1UL : 0) |
        (capabilities.GetProperty("nativeAutomation").GetBoolean() ? 2UL : 0) |
        (capabilities.GetProperty("candidateAuthoring").GetBoolean() ? 4UL : 0);

    internal static string Text(JsonElement value, string name) => value.GetProperty(name).GetString() is { Length: > 0 } text
        ? text : throw new EngineException($"Missing SDK text: {name}.");
    internal static void Schema(JsonElement value)
    {
        if (value.GetProperty("schemaVersion").GetInt32() != 1) throw new EngineException("Unsupported SDK metadata schema.");
    }
    internal static void HashText(JsonElement value, string name)
    {
        if (!Regex.IsMatch(Text(value, name), "^[0-9a-f]{64}$")) throw new EngineException($"Invalid SDK digest: {name}.");
    }
    internal static string HashBytes(byte[] bytes) => Convert.ToHexStringLower(SHA256.HashData(bytes));
    internal static string HashFile(string path) { using var file = File.OpenRead(path); return Convert.ToHexStringLower(SHA256.HashData(file)); }
    internal static byte[] ReadBytes(string path, int maximum)
    {
        if (!File.Exists(path) || new FileInfo(path).Length > maximum) throw new EngineException($"Missing or oversized SDK metadata: {path}");
        return File.ReadAllBytes(path);
    }
    internal static JsonDocument Parse(byte[] bytes)
    {
        var document = JsonDocument.Parse(bytes, new JsonDocumentOptions { MaxDepth = 32 });
        try { Unique(document.RootElement); return document; } catch { document.Dispose(); throw; }
    }
    private static void Unique(JsonElement value)
    {
        if (value.ValueKind == JsonValueKind.Object)
        {
            var names = new HashSet<string>(StringComparer.Ordinal);
            foreach (var property in value.EnumerateObject())
            { if (!names.Add(property.Name)) throw new EngineException("Duplicate SDK metadata property."); Unique(property.Value); }
        }
        else if (value.ValueKind == JsonValueKind.Array) foreach (var item in value.EnumerateArray()) Unique(item);
    }
    internal static string CheckedPath(string root, string relative)
    {
        if (string.IsNullOrWhiteSpace(relative) || relative.Contains('\\') || relative.Contains(':') || relative.Any(char.IsControl) ||
            Path.IsPathRooted(relative) || relative.Split('/').Any(p => p is "" or "." or ".." || p.EndsWith('.') || p.EndsWith(' ')))
            throw new EngineException("SDK paths must be normalized relative paths inside the selected SDK.");
        var current = root;
        foreach (var part in relative.Split('/')) { current = Path.Combine(current, part); if (File.Exists(current) || Directory.Exists(current)) NoLink(current); }
        return current;
    }
    private static void NoLink(string path)
    { if (File.GetAttributes(path).HasFlag(FileAttributes.ReparsePoint)) throw new EngineException("SDK paths cannot traverse linked files or directories."); }
    private static bool IsPrivatePayload(string path) => path.Split('/').Any(x => x.Equals("devchannel", StringComparison.OrdinalIgnoreCase) ||
        x.Equals("local-development", StringComparison.OrdinalIgnoreCase)) || new[] { "pixie-mcp", "ModelContextProtocol", "LocalCommandChannel.hpp", "ChannelTests", "NativeWriteProbe" }
        .Any(x => path.Contains(x, StringComparison.OrdinalIgnoreCase));
}
