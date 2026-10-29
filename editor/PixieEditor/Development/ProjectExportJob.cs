using System.Diagnostics;
using PixieEditor.Native;

namespace PixieEditor.Development;

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
    public bool Start(string folder, string output)
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
                var engine = EngineFolder.Find();
                var file = OperatingSystem.IsWindows() ? "pixie-export.exe" : "pixie-export";
                var tool = new[] { "Release", "Debug" }.Select(c => Path.Combine(engine, "bin", c, file))
                    .FirstOrDefault(File.Exists) ?? throw new EngineException("Build pixie-export before exporting a game.");
                var start = new ProcessStartInfo(tool)
                {
                    UseShellExecute = false, CreateNoWindow = true,
                    RedirectStandardOutput = true, RedirectStandardError = true,
                };
                start.ArgumentList.Add(Path.GetFullPath(folder));
                start.ArgumentList.Add(Path.GetFullPath(output));
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
