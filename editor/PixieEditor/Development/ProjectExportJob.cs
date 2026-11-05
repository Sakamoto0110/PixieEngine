using System.Diagnostics;

namespace PixieEditor.Native;

// The native CLI owns building, manifest creation, copying and validation. The editor only launches it.
internal sealed class ProjectExportJob : IDisposable
{
    private readonly object Gate = new();
    private readonly Queue<string> Output = new();
    private Process? Child;
    private bool Disposed;
    private bool Active;
    private bool Done;
    private int Code;
    public bool Running { get { lock (Gate) return Active; } }
    public bool Finished { get { lock (Gate) return Done; } }
    public bool Failed { get { lock (Gate) return Done && Code != 0; } }
    public string[] Lines { get { lock (Gate) return [.. Output]; } }

    private void Line(string? text)
    {
        if (text == null) return;
        lock (Gate) { Output.Enqueue(text); while (Output.Count > 512) Output.Dequeue(); }
        Console.Error.WriteLine(text);
    }
    internal static ProcessStartInfo CreateLaunch(EngineSdk sdk, string folder, string output,
        Func<string?>? discoverCMake = null, string? temporaryRoot = null)
    {
        var tool = sdk.Exporter ?? throw new EngineException("The selected Engine SDK does not include a paired exporter.");
        sdk.VerifyUnchanged();
        var start = new ProcessStartInfo(tool.Path)
        {
            UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardOutput = true, RedirectStandardError = true,
        };
        var project = Path.GetFullPath(folder);
        start.ArgumentList.Add("--sdk"); start.ArgumentList.Add(sdk.Root);
        if (ProjectBuild.HasCode(project))
        {
            if (sdk.Generation == null || !SdkRequirements.Validate(project, sdk))
                throw new EngineException("Exporting project code needs the selected SDK's installed build support and a compatible portable requirement.");
            start.Environment["PIXIE_EXPORT_CMAKE"] = ProjectBuild.RequireCMake(discoverCMake);
            start.ArgumentList.Add("--work");
            start.ArgumentList.Add(ProjectBuild.Cache(project, sdk, "shipping-export", temporaryRoot));
        }
        start.ArgumentList.Add(project); start.ArgumentList.Add(Path.GetFullPath(output));
        return start;
    }

    public bool Start(EngineSdk sdk, string folder, string output)
    {
        lock (Gate)
        {
            if (Disposed || Active) return false;
            Active = true; Done = false; Code = 0; Output.Clear();
        }
        _ = Task.Run(async () =>
        {
            var result = 1;
            try
            {
                var start = CreateLaunch(sdk, folder, output);
                using var process = new Process { StartInfo = start };
                process.OutputDataReceived += (_, e) => Line(e.Data);
                process.ErrorDataReceived += (_, e) => Line(e.Data);
                lock (Gate)
                {
                    if (Disposed) return;
                    process.Start(); Child = process;
                }
                process.BeginOutputReadLine(); process.BeginErrorReadLine();
                await process.WaitForExitAsync();
                result = process.ExitCode;
                lock (Gate) Child = null;
            }
            catch (Exception error) { Line(error.Message); }
            finally { lock (Gate) { Active = false; Done = true; Code = result; } }
        });
        return true;
    }
    public void Dispose()
    {
        lock (Gate)
        {
            Disposed = true;
            try { if (Child is { HasExited: false }) Child.Kill(entireProcessTree: true); }
            catch (Exception error) when (error is InvalidOperationException or System.ComponentModel.Win32Exception)
            {
                Console.Error.WriteLine($"[info] Export process cleanup: {error.Message}");
            }
        }
    }
}
