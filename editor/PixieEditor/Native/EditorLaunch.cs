using System.Diagnostics;

namespace PixieEditor.Native;

// Launch arguments consume an admitted selection; this helper never resolves an SDK.
internal static class EditorLaunch
{
    private static readonly HashSet<string> SelectionFlags = ["--sdk", "--sdk-id", "--sdk-build-id", "--sdk-manifest-sha256", "--sdk-profile", "--native-configuration"];

    internal static ProcessStartInfo Create(EngineSdk sdk, IEnumerable<string> arguments, int? after = null,
        string? processPath = null, string? assemblyPath = null)
    {
        var executable = processPath ?? Environment.ProcessPath ?? throw new EngineException("The Editor executable path is unavailable.");
        if (!Path.IsPathFullyQualified(executable)) throw new EngineException("Editor launches require an explicit executable path.");
        var action = arguments.ToArray();
        if (action.Any(SelectionFlags.Contains)) throw new EngineException("An Editor action cannot replace its admitted SDK selection.");
        var info = new ProcessStartInfo(executable) { UseShellExecute = false };
        if (Path.GetFileNameWithoutExtension(executable).Equals("dotnet", StringComparison.OrdinalIgnoreCase))
        {
            var assembly = assemblyPath ?? typeof(EditorLaunch).Assembly.Location;
            if (!Path.IsPathFullyQualified(assembly)) throw new EngineException("A dotnet launch requires the explicit Editor assembly path.");
            info.ArgumentList.Add(assembly);
        }
        foreach (var value in action) info.ArgumentList.Add(value);
        foreach (var value in new[] { "--sdk", sdk.Root, "--sdk-id", sdk.Id, "--sdk-build-id", sdk.BuildId,
            "--sdk-manifest-sha256", sdk.ManifestSha256, "--sdk-profile", sdk.Profile, "--native-configuration", sdk.Configuration })
            info.ArgumentList.Add(value);
        if (after is { } process)
        {
            info.ArgumentList.Add("--after");
            info.ArgumentList.Add(process.ToString(System.Globalization.CultureInfo.InvariantCulture));
        }
        return info;
    }

    internal static string SiblingModule(string loadedModule, string name)
    {
        if (!Path.IsPathFullyQualified(loadedModule) || string.IsNullOrWhiteSpace(name) || name is "." or ".." ||
            name.Contains('/') || name.Contains('\\') || name.Contains(':') || name.Any(char.IsControl))
            throw new EngineException("A sibling game needs an explicit loaded-module folder and a simple game name.");
        return Path.Combine(Path.GetDirectoryName(loadedModule)!, OperatingSystem.IsWindows() ? $"{name}-module.dll" : $"lib{name}-module.so");
    }
}
