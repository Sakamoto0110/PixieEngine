using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;

namespace PixieEditor.Native;

// Resolves the C API to the shared engine and selects a development game's module.
// A process hosts one game; another game opens in another editor process.
internal static class GameLibrary
{
    private static string? Path;
    private static bool ResolverInstalled;
    private static bool LoadSucceeded;
    private static nint EngineHandle;
    internal static EngineSdk? Selection { get; private set; }

    internal static void SelectSdk(EngineSdk sdk)
    {
        if (Path != null && Selection is { } selected &&
            (!selected.Root.Equals(sdk.Root, OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal) ||
                selected.BuildId != sdk.BuildId || selected.ManifestSha256 != sdk.ManifestSha256))
            throw new EngineException("Restart the editor to change the SDK after a library load attempt.");
        Selection = sdk;
    }

    // The engine's library for a project without code.
    public const string SceneGame = "pixie-scene";

    public static string Loaded => LoadSucceeded ? Path! : throw new InvalidOperationException("No game library is loaded.");
    public static bool IsLoaded => LoadSucceeded;

    public static void Use(string path)
    {
        if (LoadSucceeded)
            throw new InvalidOperationException("The editor hosts one game per process.");
        var sdk = Selection ?? throw new EngineException("Select an installed Engine SDK before loading a game.");

        var requested = System.IO.Path.GetFullPath(path);
        if (Path != null && !Path.Equals(requested, OperatingSystem.IsWindows()
                ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal))
            throw new EngineException("Restart the editor to select another library after a failed load.");
        Path = requested;
        if (!ResolverInstalled)
        {
            NativeLibrary.SetDllImportResolver(typeof(PixieCapi).Assembly, Resolve);
            ResolverInstalled = true;
        }
        sdk.VerifyUnchanged();
        if (EngineHandle == 0)
        {
            try { EngineHandle = NativeLibrary.Load(sdk.RuntimePath); }
            catch (Exception error) when (error is DllNotFoundException or BadImageFormatException)
            { throw new EngineException($"Cannot load the selected Engine SDK runtime: {error.Message}"); }
        }
        ValidateNativeIdentity(sdk);
        if (!Path.Equals(sdk.RuntimePath, StringComparison.OrdinalIgnoreCase))
        {
            if ((sdk.CapabilityBits & 1) == 0) throw new EngineException("The selected SDK cannot load game modules.");
            var result = PixieCapi.ModuleLoad(Path);
            EngineSession.Check(result, "load the development game module");
        }
        LoadSucceeded = true;
    }

    // Installed mode never discovers an Engine checkout or substitutes a module-adjacent runtime.
    public static string Find(string game)
    {
        if (File.Exists(game))
            return System.IO.Path.GetFullPath(game);
        if (game == SceneGame && Selection is { } sdk) return sdk.RuntimePath;
        throw new EngineException("Select an Engine SDK and supply an explicit game-module path.");
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
        return name == PixieCapi.Library ? EngineHandle : 0;
    }

    private static unsafe void ValidateNativeIdentity(EngineSdk sdk)
    {
        try
        {
            var length = PixieCapi.SdkIdentity(null, 0);
            if (length is <= 0 or > 65535) throw new EngineException("The selected runtime returned an invalid SDK identity size.");
            var bytes = new byte[length + 1];
            fixed (byte* buffer = bytes)
                if (PixieCapi.SdkIdentity(buffer, bytes.Length) != length || bytes[length] != 0 || Array.IndexOf(bytes, (byte)0) != length)
                    throw new EngineException("The selected runtime returned an inconsistent SDK identity.");
            _ = new UTF8Encoding(false, true).GetString(bytes, 0, length);
            using var document = EngineSdk.Parse(bytes[..length]);
            sdk.ValidateRuntime(document.RootElement, PixieCapi.Capabilities());
        }
        catch (Exception error) when (error is EntryPointNotFoundException or System.Text.Json.JsonException or DecoderFallbackException
            or KeyNotFoundException or InvalidOperationException or FormatException)
        { throw new EngineException($"The selected runtime has no supported SDK identity: {error.Message}"); }
    }
}
