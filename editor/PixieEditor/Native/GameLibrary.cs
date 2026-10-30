using System.Reflection;
using System.Runtime.InteropServices;

namespace PixieEditor.Native;

// Resolves the C API to the shared engine and selects a development game's module.
// A process hosts one game; another game opens in another editor process.
internal static class GameLibrary
{
    private static string? Path;

    // The engine's library for a project without code.
    public const string SceneGame = "pixie-scene";

    public static string Loaded => Path ?? throw new InvalidOperationException("No game library is loaded.");
    public static bool IsLoaded => Path != null;

    public static void Use(string path)
    {
        if (Path != null)
            throw new InvalidOperationException("The editor hosts one game per process.");

        Path = System.IO.Path.GetFullPath(path);
        NativeLibrary.SetDllImportResolver(typeof(GameLibrary).Assembly, Resolve);
        var engineName = OperatingSystem.IsWindows() ? "pixie.dll" : "libpixie.so";
        if (!System.IO.Path.GetFileName(Path).Equals(engineName, StringComparison.OrdinalIgnoreCase))
        {
            var result = PixieCapi.ModuleLoad(Path);
            EngineSession.Check(result, "load the development game module");
        }
    }

    // `game` is a path to a library, or a game's name, looked up as "<name>-module" next to the
    // engine's executables: bin/Release, then bin/Debug, under the repository the editor runs from.
    public static string Find(string game)
    {
        if (File.Exists(game))
            return game;

        var file = game == SceneGame
            ? OperatingSystem.IsWindows() ? "pixie.dll" : "libpixie.so"
            : OperatingSystem.IsWindows() ? $"{game}-module.dll" : $"lib{game}-module.so";
        for (var directory = new DirectoryInfo(AppContext.BaseDirectory); directory != null; directory = directory.Parent)
        {
            foreach (var config in new[] { "Release", "Debug" })
            {
                var candidate = System.IO.Path.Combine(directory.FullName, "bin", config, file);
                if (File.Exists(candidate))
                    return candidate;
            }
        }

        throw new FileNotFoundException($"No library for the game '{game}' ({file}); build the engine first.");
    }

    // The games built next to the loaded one: every "<name>-module" library in its directory, but the
    // engine's own library for projects, which is not a game.
    public static IReadOnlyList<string> Siblings()
    {
        var directory = System.IO.Path.GetDirectoryName(Loaded)!;
        var pattern = OperatingSystem.IsWindows() ? "*-module.dll" : "lib*-module.so";
        return Directory.GetFiles(directory, pattern)
            .Select(GameName)
            .Where(name => name != SceneGame)
            .Order()
            .ToList();
    }

    private static string GameName(string file)
    {
        var name = System.IO.Path.GetFileNameWithoutExtension(file);
        if (!OperatingSystem.IsWindows() && name.StartsWith("lib", StringComparison.Ordinal))
            name = name[3..];
        return name[..^"-module".Length];
    }

    private static nint Resolve(string name, Assembly assembly, DllImportSearchPath? searchPath)
    {
        if (name != PixieCapi.Library || Path == null) return 0;
        var engine = System.IO.Path.Combine(System.IO.Path.GetDirectoryName(Path)!,
            OperatingSystem.IsWindows() ? "pixie.dll" : "libpixie.so");
        return NativeLibrary.Load(engine);
    }
}
