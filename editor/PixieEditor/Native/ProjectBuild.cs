using System.Diagnostics;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace PixieEditor.Native;

// Builds the declared module in an owned consumer cache, against the admitted SDK.
internal sealed class ProjectBuild : IDisposable
{
#if DEBUG
    internal const string Configuration = "Debug";
#else
    internal const string Configuration = "Release";
#endif
    internal sealed record BuildPlan(string Folder, EngineSdk Sdk, string CMake, string Cache, string Target, string Library,
        string Owner, byte[] Ownership, string[] Configure, string[] Build);
    private readonly BuildPlan? Plan;
    private readonly string Folder;
    // Written by the build's threads, read by the editor's.
    private readonly object Gate = new();
    private readonly List<string> Output = [];
    private Process? Running;
    private Task? Work;
    private bool Disposed;

    public ProjectBuild(BuildPlan plan) { Plan = plan; Folder = plan.Folder; }
    // Explicit source fixtures still use the separate source build entry.
    public ProjectBuild(string sourceFolder) => Folder = sourceFolder;
    public string? BuiltLibrary => Succeeded ? Plan?.Library : null;

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
        Console.Error.WriteLine(line);
    }

    public bool Finished => Work is { IsCompleted: true };
    public bool Succeeded { get; private set; }
    // What went wrong, in a sentence: a missing tool, or the step that failed.
    public string? Problem { get; private set; }

    // Whether the project has code of its own.
    public static bool HasCode(string folder) => File.Exists(Path.Combine(folder, "CMakeLists.txt"));

    internal static EngineSdk.ProjectSupport RequireSupport(string folder, EngineSdk sdk)
    {
        var support = sdk.Generation ?? throw new EngineException("The selected SDK does not include installed project code support.");
        sdk.VerifyUnchanged();
        if (!SdkRequirements.Validate(folder, sdk))
            throw new EngineException("Record the selected SDK requirement for this project before building or adding code.");
        return support;
    }

    // Writes the code templates into the project (cmake/PixieAddCode.cmake); throws with CMake's
    // output when it refuses.
    public static void AddCode(string folder)
    {
        if (GameLibrary.Selection is { } sdk) { AddCode(folder, sdk); return; }
        var cmake = RequireCMake();
        var (code, output) = RunNow(cmake, ["-DPIXIE_PROJECT_DIR=" + folder, "-DPIXIE_ENGINE_MODE=SOURCE", "-P",
            Path.Combine(EngineFolder.Find(), "cmake", "PixieAddCode.cmake")]);
        if (code != 0)
            throw new EngineException($"Could not add code: {output.Trim()}");
    }

    internal static void AddCode(string folder, EngineSdk sdk)
    {
        var support = RequireSupport(folder, sdk);
        var (code, output) = RunNow(RequireCMake(), ["-DPIXIE_PROJECT_DIR=" + Path.GetFullPath(folder),
            "-DPIXIE_ENGINE_MODE=PACKAGE", "-DPIXIE_PROJECT_BUILD_ROLE=authoring", "-DPIXIE_SDK_ROOT=" + sdk.Root,
            "-DPIXIE_SDK_PROFILE=" + sdk.Profile, "-DPIXIE_PROJECT_ADOPT_SDK=OFF", "-DPIXIE_GENERATED_CANDIDATE_AUTHORING=OFF", "-P", support.Generator]);
        if (code != 0) throw new EngineException($"Could not add code with the selected SDK: {output.Trim()}");
    }

    internal static string ReadTarget(string folder, EngineSdk.ProjectSupport support)
    {
        var path = ProjectCode.CheckedPath(folder, ".pixie-code/manifest.json");
        using var document = EngineSdk.Parse(EngineSdk.ReadBytes(path, 4 * 1024 * 1024));
        var registry = document.RootElement;
        EngineSdk.Schema(registry);
        if (EngineSdk.Text(registry, "generatorFamily") != "sdk-v1" || registry.GetProperty("components").ValueKind != JsonValueKind.Array)
            throw new EngineException("This project does not have the installed sdk-v1 registry. Its existing code and CMake were preserved.");
        var id = EngineSdk.Text(registry, "projectId");
        if (!Regex.IsMatch(id, @"\A[A-Za-z][A-Za-z0-9_]*\z", RegexOptions.CultureInvariant))
            throw new EngineException("The SDK project registry does not name a valid module target.");
        foreach (var (relative, hash) in new[] { ("src/PixieComponents.generated.hpp", "headerHash"),
            ("src/PixieComponents.generated.cpp", "sourceHash"), (".pixie-code/Components.cmake", "cmakeHash") })
        {
            var generated = ProjectCode.CheckedPath(folder, relative);
            if (!File.Exists(generated) || !EngineSdk.HashFile(generated).Equals(EngineSdk.Text(registry, hash), StringComparison.OrdinalIgnoreCase))
                throw new EngineException($"The project registry no longer owns {relative}. Its existing files were preserved.");
        }
        var cmake = File.ReadAllText(ProjectCode.CheckedPath(folder, "CMakeLists.txt")).Replace("\r\n", "\n");
        var template = File.ReadAllText(support.CMakeTemplate).Replace("\r\n", "\n").Replace("@PIXIE_ID@", id);
        if (cmake != template.Replace("@PIXIE_GENERATED_CANDIDATE_OPTION@", "") &&
            cmake != template.Replace("@PIXIE_GENERATED_CANDIDATE_OPTION@", "CANDIDATE_AUTHORING"))
            throw new EngineException("The project's CMake does not match its installed sdk-v1 template. Its custom wiring was preserved.");
        return id + "-module";
    }

    private static string CacheKey(string folder, EngineSdk sdk, string role) => EngineSdk.HashBytes(JsonSerializer.SerializeToUtf8Bytes(new[] {
        Path.TrimEndingDirectorySeparator(Path.GetFullPath(folder)), sdk.Root, sdk.ManifestSha256, sdk.BuildId, role, sdk.Configuration }));

    internal static string Cache(string folder, EngineSdk sdk, string role, string? temporaryRoot = null)
    {
        var directory = role switch { "authoring" => "code", "shipping-export" => "export", _ => throw new ArgumentException("Unknown build role.", nameof(role)) };
        // Visual Studio's tracking files still have a path limit. The directory
        // is a short locator; each build checks the full ownership before reuse.
        return ProjectCode.CheckedPath(temporaryRoot ?? Path.GetTempPath(), $"PixieEditor/{directory}/{CacheKey(folder, sdk, role)[..24]}");
    }

    private static void CheckCache(BuildPlan plan)
    {
        _ = ProjectCode.CheckedPath(plan.Cache, "owner.json");
        if (File.Exists(plan.Owner))
        {
            if (!EngineSdk.ReadBytes(plan.Owner, 64 * 1024).AsSpan().SequenceEqual(plan.Ownership))
                throw new EngineException("The project build cache belongs to another project, SDK or toolchain. Its files were preserved.");
        }
        else if (Directory.Exists(plan.Cache) && Directory.EnumerateFileSystemEntries(plan.Cache).Any(p => Path.GetFileName(p) != ".build.lock"))
            throw new EngineException("The project build cache is unowned. Its existing files were preserved.");
    }

    internal static BuildPlan Prepare(string folder, EngineSdk sdk, Func<string?>? discovery = null, string? temporaryRoot = null)
    {
        try
        {
            folder = Path.TrimEndingDirectorySeparator(Path.GetFullPath(folder));
            var support = RequireSupport(folder, sdk);
            var target = ReadTarget(folder, support);
            var cmake = RequireCMake(discovery);
            var cache = Cache(folder, sdk, "authoring", temporaryRoot);
            var build = ProjectCode.CheckedPath(cache, "build");
            var bin = ProjectCode.CheckedPath(cache, "bin");
            var module = ProjectCode.CheckedPath(cache, $"bin/{sdk.Configuration}/{target}.dll");
            var owner = ProjectCode.CheckedPath(cache, "owner.json");
            var ownership = JsonSerializer.SerializeToUtf8Bytes(new { schemaVersion = 1, cacheKey = CacheKey(folder, sdk, "authoring"), project = folder, sdkRoot = sdk.Root,
                manifestSha256 = sdk.ManifestSha256, sdkBuildId = sdk.BuildId, role = "authoring", configuration = sdk.Configuration, cmake });
            var plan = new BuildPlan(folder, sdk, cmake, cache, target, module, owner, ownership,
                ["-S", folder, "-B", build, "-DPIXIE_ENGINE_MODE=PACKAGE", "-DPIXIE_ENGINE_DIR=", "-DPIXIE_SDK_ROOT=" + sdk.Root,
                 "-DPIXIE_SDK_PROFILE=" + sdk.Profile, "-DPIXIE_PROJECT_BUILD_ROLE=authoring", "-DPIXIE_PROJECT_ADOPT_SDK=OFF",
                 "-DPIXIE_OUTPUT_ROOT=" + bin, "-DPixieEngine_DIR=" + Path.Combine(sdk.Root, "lib", "cmake", "PixieEngine"),
                 "-DCMAKE_CONFIGURATION_TYPES=" + sdk.Configuration, "-DCMAKE_BUILD_TYPE=" + sdk.Configuration,
                 "-DCMAKE_SYSTEM_VERSION=" + EngineSdk.Text(sdk.Identity, "windowsSdkVersion")],
                ["--build", build, "--config", sdk.Configuration, "--target", target]);
            CheckCache(plan);
            return plan;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or InvalidOperationException or ArgumentException)
        { throw new EngineException($"Could not prepare this installed project build: {error.Message}"); }
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
        try
        {
            if (Plan == null) { RunSource(); return; }
            var support = RequireSupport(Plan.Folder, Plan.Sdk);
            if (ReadTarget(Plan.Folder, support) != Plan.Target) throw new EngineException("The project module target changed before building.");
            CheckCache(Plan);
            Directory.CreateDirectory(Plan.Cache);
            using var lease = new FileStream(ProjectCode.CheckedPath(Plan.Cache, ".build.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
            CheckCache(Plan);
            if (!File.Exists(Plan.Owner))
            {
                using var ownership = new FileStream(Plan.Owner, FileMode.CreateNew, FileAccess.Write, FileShare.None);
                ownership.Write(Plan.Ownership);
            }
            Add($"Configuring the project with {Plan.Sdk.Summary}.");
            if (Step(Plan.CMake, Plan.Configure) != 0) { Problem = Explain("Configuring the project failed."); return; }
            Add($"Building {Plan.Target} ({Plan.Sdk.Configuration}).");
            if (Step(Plan.CMake, Plan.Build) != 0) { Problem = Explain("Building the project failed; the errors are in red below."); return; }
            if (!File.Exists(ProjectCode.CheckedPath(Plan.Cache, $"bin/{Plan.Sdk.Configuration}/{Plan.Target}.dll")))
                throw new EngineException("The build did not produce the declared SDK game module.");
            Add("Built.");
            Succeeded = true;
        }
        catch (Exception error) when (error is EngineException or IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or
                                      InvalidOperationException or System.ComponentModel.Win32Exception or ArgumentException)
        { Problem = error.Message; }
    }

    private void RunSource()
    {
        var engine = EngineFolder.Find(); // Refuses whenever an installed SDK is selected.
        var cmake = RequireCMake();
        var build = Path.Combine(Folder, "build");
        if (Step(cmake, ["-S", Folder, "-B", build, "-DPIXIE_ENGINE_MODE=SOURCE", "-DPIXIE_ENGINE_DIR=" + engine]) != 0)
        { Problem = Explain("Configuring the explicit source project failed."); return; }
        if (Step(cmake, ["--build", build, "--config", Configuration, "--parallel"]) != 0)
        { Problem = Explain("Building the explicit source project failed."); return; }
        var output = Path.Combine(Folder, "bin", Configuration);
        var pattern = OperatingSystem.IsWindows() ? "*-module.dll" : "lib*-module.so";
        if (!Directory.Exists(output) || Directory.GetFiles(output, pattern).Length != 1)
        { Problem = "The explicit source build did not produce one game module."; return; }
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
        lock (Gate)
        {
            if (Disposed) return 1;
            process.Start(); Running = process;
        }
        try
        {
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();
            process.WaitForExit();
            return process.ExitCode;
        }
        finally { lock (Gate) Running = null; }
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
        lock (Gate)
        {
            Disposed = true;
            try { Running?.Kill(entireProcessTree: true); }
            catch (Exception error) when (error is InvalidOperationException or System.ComponentModel.Win32Exception)
            { Console.Error.WriteLine($"[info] Build process cleanup: {error.Message}"); }
        }
    }

    private const string NoCMake =
        "CMake was not found, on PATH or with Visual Studio: install Visual Studio with the \"Desktop development with C++\" workload, which brings it, or CMake from cmake.org.";

    internal static string RequireCMake(Func<string?>? discovery = null)
    {
        var path = (discovery ?? FindCMake)();
        if (path == null || !Path.IsPathFullyQualified(path) || !File.Exists(path) ||
            !Path.GetFileName(path).Equals(OperatingSystem.IsWindows() ? "cmake.exe" : "cmake", StringComparison.OrdinalIgnoreCase))
            throw new EngineException(NoCMake);
        return Path.GetFullPath(path);
    }

    // cmake on PATH, or the one Visual Studio installs.
    public static string? FindCMake()
    {
        var name = OperatingSystem.IsWindows() ? "cmake.exe" : "cmake";
        foreach (var directory in (Environment.GetEnvironmentVariable("PATH") ?? "").Split(Path.PathSeparator))
        {
            var candidate = Path.Combine(directory.Trim(), name);
            if (Path.IsPathFullyQualified(candidate) && File.Exists(candidate))
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
        if (GameLibrary.Selection != null)
            throw new EngineException("This code operation requires the selected SDK's installed build/generation support. The project and its existing build files were left unchanged; an Engine source checkout cannot be used as a fallback.");
        for (var directory = new DirectoryInfo(AppContext.BaseDirectory); directory != null; directory = directory.Parent)
        {
            if (File.Exists(Path.Combine(directory.FullName, "cmake", "PixieProject.cmake")))
                return directory.FullName;
        }

        throw new EngineException("The editor does not run from PixieEngine's folder, so it has no engine to build project code with.");
    }
}
