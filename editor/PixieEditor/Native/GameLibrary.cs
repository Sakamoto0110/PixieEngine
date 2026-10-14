using System.Reflection;
using System.Runtime.InteropServices;

namespace PixieEditor.Native;

// Points the C API's library name at one game's library ("boids-capi.dll", ...). A process hosts one
// game: P/Invoke binds the name once, so another game means another editor process.
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
    }

    // `game` is a path to a library, or a game's name, looked up as "<name>-capi" next to the
    // engine's executables: bin/Release, then bin/Debug, under the repository the editor runs from.
    public static string Find(string game)
    {
        if (File.Exists(game))
            return game;

        var file = OperatingSystem.IsWindows() ? $"{game}-capi.dll" : $"lib{game}-capi.so";
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

    // The games built next to the loaded one: every "<name>-capi" library in its directory, but the
    // engine's own library for projects, which is not a game.
    public static IReadOnlyList<string> Siblings()
    {
        var directory = System.IO.Path.GetDirectoryName(Loaded)!;
        var pattern = OperatingSystem.IsWindows() ? "*-capi.dll" : "lib*-capi.so";
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
        return name[..^"-capi".Length];
    }

    private static nint Resolve(string name, Assembly assembly, DllImportSearchPath? searchPath)
    {
        return name == PixieCapi.Library && Path != null ? NativeLibrary.Load(Path) : 0;
    }
}
