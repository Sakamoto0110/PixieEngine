using System.Text.Json;

namespace PixieMcp;

internal sealed partial class EngineConnection
{
    private static (string Player, string Project) OpenTestBuild(string manifest, SelectionContext selection)
    {
        if (!Path.IsPathFullyQualified(manifest) || Path.GetFileName(manifest) != "pixie-test-build.json")
            throw new IOException("Select the absolute pixie-test-build.json path.");
        manifest = Path.GetFullPath(manifest);
        CheckPath(manifest, "Test-build manifest");
        var root = Path.GetDirectoryName(manifest)!;
        using var parsed = SelectionContext.Parse(SelectionContext.ReadMetadata(manifest), 16);
        var bundle = parsed.RootElement;
        if (bundle.GetProperty("format").GetString() != "pixie-test-build" || bundle.GetProperty("version").GetInt32() != 1 ||
            bundle.GetProperty("distributionClass").GetString() != "local-private" ||
            bundle.GetProperty("profile").GetString() != "local-development" ||
            bundle.GetProperty("configuration").GetString() != "Release" || selection.Configuration != "Release" ||
            !bundle.GetProperty("development").GetBoolean() || !bundle.GetProperty("dev_channel").GetBoolean() ||
            bundle.GetProperty("sdkId").GetString() != selection.Id || bundle.GetProperty("sdkBuildId").GetString() != selection.BuildId ||
            bundle.GetProperty("sdkManifestSha256").GetString() != selection.ManifestHash)
            throw new IOException("The test bundle does not match the explicitly selected private Release SDK.");

        var runtimeHashes = selection.RuntimeHashes();
        var player = bundle.GetProperty("player").GetString();
        var project = bundle.GetProperty("project").GetString();
        var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase) { Path.GetFileName(manifest) };
        var playerCount = 0;
        var projectCount = 0;
        var sceneCount = 0;
        foreach (var item in bundle.GetProperty("files").EnumerateArray())
        {
            var name = item.GetProperty("path").GetString() ?? "";
            var parts = name.Split('/');
            if (name.Length > 1024 || parts.Any(part => part is "" or "." or ".." || part.EndsWith('.') || part.EndsWith(' ') ||
                part.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0) || !paths.Add(name) || paths.Count > 65536)
                throw new IOException("Test-build inventory paths must be unique, portable and relative.");
            var path = Path.Combine(root, name.Replace('/', Path.DirectorySeparatorChar));
            CheckPath(path, "Test-build file");
            var hash = item.GetProperty("sha256").GetString();
            if (new FileInfo(path).Length != item.GetProperty("size").GetInt64() || SelectionContext.HashFile(path) != hash)
                throw new IOException($"Test-build file differs from its inventory: {name}");

            var extension = Path.GetExtension(name).ToLowerInvariant();
            switch (item.GetProperty("role").GetString())
            {
                case "player" when parts.Length == 1 && name == player && extension == ".exe":
                    ++playerCount;
                    break;
                case "runtime" when parts.Length == 1 && extension == ".dll":
                    if (!runtimeHashes.Remove(name, out var expected) || hash != expected)
                        throw new IOException($"Test-build runtime differs from the selected SDK: {name}");
                    break;
                case "library" when parts.Length == 1 && extension == ".dll" &&
                    !name.EndsWith("-module.dll", StringComparison.OrdinalIgnoreCase) &&
                    !name.EndsWith("-capi.dll", StringComparison.OrdinalIgnoreCase):
                    break;
                case "project" when parts.Length == 1 && name == project && extension == ".pixie":
                    ++projectCount;
                    break;
                case "scene" when extension == ".scene":
                    ++sceneCount;
                    break;
                case "asset" or "notice" when extension is not (".exe" or ".dll" or ".pixie"):
                    break;
                default:
                    throw new IOException($"Invalid test-build file role or path: {name}");
            }
        }
        if (runtimeHashes.Count != 0 || playerCount != 1 || projectCount != 1 || sceneCount == 0)
            throw new IOException("Test build must contain one player, one project, its scenes and the selected SDK runtime closure.");

        var directories = new Stack<string>();
        directories.Push(root);
        var entries = 0;
        while (directories.TryPop(out var directory))
        {
            foreach (var path in Directory.EnumerateFileSystemEntries(directory))
            {
                if (++entries > 131072) throw new IOException("The test-build inventory exceeds its bound.");
                CheckPath(path, "Test-build tree");
                if (Directory.Exists(path)) directories.Push(path);
                else if (!paths.Remove(Path.GetRelativePath(root, path).Replace('\\', '/')))
                    throw new IOException($"Undeclared test-build file: {path}");
            }
        }
        if (paths.Count != 0) throw new IOException("Test-build inventory is incomplete.");
        return (Path.Combine(root, player!), root);
    }
}
