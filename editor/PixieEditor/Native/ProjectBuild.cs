using System.Diagnostics;

namespace PixieEditor.Native;

// A project's code, built with CMake against the engine the editor runs from: the project never
// records where the engine is, so it builds with whichever engine opens it. A build configures
// <project>/build, builds the editor's own configuration, and leaves the game's C API library in
// <project>/bin/<config>/. It runs in the background; its output arrives line by line.
internal sealed class ProjectBuild : IDisposable
{
#if DEBUG
    public const string Configuration = "Debug";
#else
    public const string Configuration = "Release";
#endif

    private readonly string Folder;
    // Written by the build's threads, read by the editor's.
    private readonly object Gate = new();
    private readonly List<string> Output = [];
    private Process? Running;
    private Task? Work;

    public ProjectBuild(string folder) => Folder = folder;

    // The output so far, as a copy.
    public string[] Lines
    {
        get
        {
            lock (Gate)
                return [.. Output];
        }
    }

    private void Add(string line)
    {
        lock (Gate)
            Output.Add(line);
    }

    public bool Finished => Work is { IsCompleted: true };
    public bool Succeeded { get; private set; }
    // What went wrong, in a sentence: a missing tool, or the step that failed.
    public string? Problem { get; private set; }

    // Whether the project has code of its own.
    public static bool HasCode(string folder) => File.Exists(Path.Combine(folder, "CMakeLists.txt"));

    // The game's C API library the last build left, or null.
    public static string? Library(string folder)
    {
        var directory = Path.Combine(folder, "bin", Configuration);
        if (!Directory.Exists(directory))
            return null;
        var pattern = OperatingSystem.IsWindows() ? "*-capi.dll" : "lib*-capi.so";
        var found = Directory.GetFiles(directory, pattern);
        return found.Length == 1 ? found[0] : null;
    }

    // Writes the code templates into the project (cmake/PixieAddCode.cmake); throws with CMake's
    // output when it refuses.
    public static void AddCode(string folder)
    {
        var cmake = FindCMake() ?? throw new EngineException(NoCMake);
        var (code, output) = RunNow(cmake, ["-DPIXIE_PROJECT_DIR=" + folder, "-P", Path.Combine(EngineFolder.Find(), "cmake", "PixieAddCode.cmake")]);
        if (code != 0)
            throw new EngineException($"Could not add code: {output.Trim()}");
    }

    public void Start()
    {
        lock (Gate)
            Output.Clear();
        Succeeded = false;
        Problem = null;
        Work = Task.Run(Run);
    }

    private void Run()
    {
        var cmake = FindCMake();
        if (cmake == null)
        {
            Problem = NoCMake;
            return;
        }

        string engine;
        try
        {
            engine = EngineFolder.Find();
        }
        catch (EngineException e)
        {
            Problem = e.Message;
            return;
        }

        var build = Path.Combine(Folder, "build");
        Add($"> cmake -S \"{Folder}\" -B \"{build}\" -DPIXIE_ENGINE_DIR=\"{engine}\"");
        if (Step(cmake, ["-S", Folder, "-B", build, "-DPIXIE_ENGINE_DIR=" + engine]) != 0)
        {
            Problem = Explain("Configuring the project failed.");
            return;
        }

        Add($"> cmake --build \"{build}\" --config {Configuration} --parallel");
        if (Step(cmake, ["--build", build, "--config", Configuration, "--parallel"]) != 0)
        {
            Problem = Explain("Building the project failed; the errors are in red below.");
            return;
        }

        if (Library(Folder) == null)
        {
            Problem = $"The build left no C API library in bin/{Configuration}.";
            return;
        }

        Add("Built.");
        Succeeded = true;
    }

    // Runs one command, passing its output on; returns its exit code.
    private int Step(string file, string[] arguments)
    {
        var info = new ProcessStartInfo(file, arguments)
        {
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };
        using var process = new Process { StartInfo = info };
        process.OutputDataReceived += (_, e) => { if (e.Data != null) Add(e.Data); };
        process.ErrorDataReceived += (_, e) => { if (e.Data != null) Add(e.Data); };
        process.Start();
        Running = process;
        process.BeginOutputReadLine();
        process.BeginErrorReadLine();
        process.WaitForExit();
        Running = null;
        return process.ExitCode;
    }

    // The failure, with what to install when a tool was missing.
    private string Explain(string failure)
    {
        var text = string.Join('\n', Lines);
        if (text.Contains("No CMAKE_CXX_COMPILER could be found", StringComparison.Ordinal) ||
            text.Contains("could not find any instance of Visual Studio", StringComparison.OrdinalIgnoreCase))
        {
            return "No C++ compiler was found: install Visual Studio with the \"Desktop development with C++\" workload.";
        }

        return failure;
    }

    public void Dispose()
    {
        try
        {
            Running?.Kill(entireProcessTree: true);
        }
        catch (InvalidOperationException)
        {
            // It ended on its own.
        }
    }

    private const string NoCMake =
        "CMake was not found, on PATH or with Visual Studio: install Visual Studio with the \"Desktop development with C++\" workload, which brings it, or CMake from cmake.org.";

    // cmake on PATH, or the one Visual Studio installs.
    public static string? FindCMake()
    {
        var name = OperatingSystem.IsWindows() ? "cmake.exe" : "cmake";
        foreach (var directory in (Environment.GetEnvironmentVariable("PATH") ?? "").Split(Path.PathSeparator))
        {
            var candidate = Path.Combine(directory.Trim(), name);
            if (directory.Length > 0 && File.Exists(candidate))
                return candidate;
        }

        if (!OperatingSystem.IsWindows())
            return null;
        var vswhere = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86),
            "Microsoft Visual Studio", "Installer", "vswhere.exe");
        if (!File.Exists(vswhere))
            return null;
        var (code, output) = RunNow(vswhere, ["-latest", "-prerelease", "-products", "*", "-find",
            @"Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"]);
        var found = output.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries).FirstOrDefault();
        return code == 0 && found != null && File.Exists(found) ? found : null;
    }

    private static (int Code, string Output) RunNow(string file, string[] arguments)
    {
        var info = new ProcessStartInfo(file, arguments)
        {
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true,
        };
        using var process = Process.Start(info)!;
        var error = process.StandardError.ReadToEndAsync();
        var output = process.StandardOutput.ReadToEnd();
        process.WaitForExit();
        return (process.ExitCode, output + error.Result);
    }
}

// The engine the editor runs from: the first folder above the editor that holds
// cmake/PixieProject.cmake.
internal static class EngineFolder
{
    public static string Find()
    {
        for (var directory = new DirectoryInfo(AppContext.BaseDirectory); directory != null; directory = directory.Parent)
        {
            if (File.Exists(Path.Combine(directory.FullName, "cmake", "PixieProject.cmake")))
                return directory.FullName;
        }

        throw new EngineException("The editor does not run from PixieEngine's folder, so it has no engine to build project code with.");
    }
}
