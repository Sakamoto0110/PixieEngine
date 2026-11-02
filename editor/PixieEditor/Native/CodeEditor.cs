using System.ComponentModel;
using System.Diagnostics;

namespace PixieEditor.Native;

internal static class CodeEditor
{
    internal static ProcessStartInfo StartInfo(string executable, string folder, string source)
    {
        var path = ProjectCode.ResolveSource(folder, source);
        var info = new ProcessStartInfo(executable) { UseShellExecute = false, CreateNoWindow = true };
        info.ArgumentList.Add("--reuse-window");
        info.ArgumentList.Add(Path.GetFullPath(folder));
        info.ArgumentList.Add("--goto");
        info.ArgumentList.Add(path);
        return info;
    }

    internal static string? Open(string folder, string source)
    {
        var executable = Find();
        if (executable == null)
            throw new EngineException("VS Code was not found. Install Visual Studio Code and enable its PATH command, or install it in the default user/system location.");
        var start = StartInfo(executable, folder, source);
        string? notice = null;
        try
        {
            var cmake = ProjectBuild.FindCMake() ?? throw new EngineException("CMake was not found. Install the Visual Studio C++ workload or select CMake in VS Code.");
            CodeWorkspace.Prepare(folder, EngineFolder.Find(), cmake);
        }
        catch (EngineException error) { notice = "VS Code opened; IDE setup needs attention: " + error.Message; }
        try { using var process = Process.Start(start); }
        catch (Exception error) when (error is Win32Exception or InvalidOperationException or IOException or UnauthorizedAccessException or ArgumentException or NotSupportedException)
        { throw new EngineException($"Could not open VS Code: {error.Message}. Check its installation and PATH."); }
        return notice;
    }

    private static string? Find()
    {
        var candidates = new List<string>();
        if (OperatingSystem.IsWindows())
        {
            candidates.Add(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs", "Microsoft VS Code", "Code.exe"));
            candidates.Add(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Microsoft VS Code", "Code.exe"));
        }
        else if (OperatingSystem.IsMacOS()) candidates.Add("/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code");
        foreach (var directory in (Environment.GetEnvironmentVariable("PATH") ?? "").Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries))
        {
            var path = directory.Trim('"');
            candidates.Add(Path.Combine(path, OperatingSystem.IsWindows() ? "Code.exe" : "code"));
            // VS Code's Windows PATH entry is its bin/ wrapper directory. Launch its real executable
            // directly, avoiding shell parsing of user project or source names.
            if (OperatingSystem.IsWindows() && File.Exists(Path.Combine(path, "code.cmd")))
                candidates.Add(Path.GetFullPath(Path.Combine(path, "..", "Code.exe")));
        }
        return candidates.FirstOrDefault(File.Exists);
    }
}
