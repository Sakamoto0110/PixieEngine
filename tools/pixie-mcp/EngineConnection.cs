using System.Diagnostics;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using ModelContextProtocol.Protocol;

namespace PixieMcp;

// An owned local host; no retry after a request has been sent.
internal sealed class EngineConnection : IAsyncDisposable
{
    private readonly NamedPipeClientStream pipe;
    private readonly Process process;
    private readonly HostJob job;
    private readonly SemaphoreSlim gate = new(1, 1);
    private readonly string captureRoot;
    private bool poisoned;
    private long nextId;
    private EngineConnection(Process process, HostJob job, NamedPipeClientStream pipe, string captureRoot)
        => (this.process, this.job, this.pipe, this.captureRoot) = (process, job, pipe, captureRoot);

    public static async Task<EngineConnection> OpenAsync(string executable, string project, string captureDirectory,
        string channel, bool editor, TimeSpan startup, CancellationToken cancellationToken)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("The qualified transport is Windows named pipes.");
        if (captureDirectory.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar).Contains(".."))
            throw new ArgumentException("Capture startup root cannot contain parent traversal.");
        var root = Path.GetFullPath(captureDirectory);
        CheckPath(root);
        Directory.CreateDirectory(root);
        CheckPath(root);
        var info = new ProcessStartInfo(Path.GetFullPath(executable))
        {
            UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardOutput = true, RedirectStandardError = true,
            WorkingDirectory = Path.GetFullPath(project)
        };
        foreach (var argument in new[] { "--project", Path.GetFullPath(project), "--dev-channel", channel,
            "--dev-capture-dir", root, "--dev-hidden", "on" }) info.ArgumentList.Add(argument);
        if (editor) { info.ArgumentList.Add("--recent"); info.ArgumentList.Add("off"); }
        var process = Process.Start(info) ?? throw new IOException("Host process did not start.");
        // The host and its build tools close with this server, even when it does not exit cleanly.
        var job = HostJob.Contain(process);
        // Neither the native host nor its diagnostics may contaminate MCP stdout.
        _ = RelayAsync(process.StandardOutput);
        _ = RelayAsync(process.StandardError);
        var pipe = new NamedPipeClientStream(".", $"pixie-{channel}", PipeDirection.InOut,
            PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
        var connection = new EngineConnection(process, job, pipe, root);
        try
        {
            // The editor opens the channel only after building a project's code, which can take minutes.
            var clock = Stopwatch.StartNew();
            while (true)
            {
                if (process.HasExited) throw new IOException($"Host exited with code {process.ExitCode} before opening its channel.");
                var remaining = startup - clock.Elapsed;
                if (remaining <= TimeSpan.Zero) throw new TimeoutException($"Host did not open its channel within {startup.TotalSeconds:0} seconds.");
                try
                {
                    await pipe.ConnectAsync((int)Math.Min(500, Math.Ceiling(remaining.TotalMilliseconds)), cancellationToken);
                    break;
                }
                catch (TimeoutException) { }
            }
            var hello = await connection.SendAsync("world.hello", new JsonObject(), cancellationToken);
            var value = hello["result"]?["value"];
            if (value?["protocol_version"]?.GetValue<int>() != 1 || value?["pixie_api_version"]?.GetValue<int>() != 18)
                throw new IOException("Host development protocol/API version mismatch.");
            var actualRoot = value?["capture_directory"]?.GetValue<string>();
            if (!string.Equals(Path.GetFullPath(actualRoot ?? ""), root, StringComparison.OrdinalIgnoreCase))
                throw new IOException("Host capture directory does not match the configured directory.");
            return connection;
        }
        catch { await connection.DisposeAsync(); throw; }
    }

    private static async Task RelayAsync(StreamReader reader)
    {
        try { while (await reader.ReadLineAsync() is { } line) await Console.Error.WriteLineAsync(line); }
        catch (IOException) { }
    }

    private async Task<JsonNode> SendAsync(string method, JsonNode parameters, CancellationToken cancellationToken)
    {
        await gate.WaitAsync(cancellationToken);
        bool sent = false;
        try
        {
            if (poisoned) throw new IOException("Connection is unusable after an uncertain response; no automatic retry.");
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeout.CancelAfter(TimeSpan.FromSeconds(30));
            var id = ++nextId;
            var request = new JsonObject { ["jsonrpc"] = "2.0", ["id"] = id, ["method"] = method, ["params"] = parameters.DeepClone() };
            var body = Encoding.UTF8.GetBytes(request.ToJsonString());
            if (body.Length > 65536) throw new ArgumentException("Engine request exceeds 64 KiB.");
            sent = true;
            await pipe.WriteAsync(body, timeout.Token);
            await pipe.WriteAsync(new byte[] { 10 }, timeout.Token);
            await pipe.FlushAsync(timeout.Token);
            // Only one request is outstanding; extra data after its line is a protocol error.
            // Bound bytes before parsing rather than using an unbounded ReadLineAsync.
            using var response = new MemoryStream();
            var chunk = new byte[4096];
            while (true)
            {
                var count = await pipe.ReadAsync(chunk, timeout.Token);
                if (count == 0) throw new IOException("Host disconnected before its response.");
                var newline = Array.IndexOf(chunk, (byte)10, 0, count);
                var size = newline >= 0 ? newline : count;
                if (response.Length + size >= 4 * 1024 * 1024) throw new IOException("Host response exceeds 4 MiB.");
                response.Write(chunk, 0, size);
                if (newline >= 0)
                {
                    if (newline != count - 1) throw new IOException("Unexpected pipelined host response.");
                    break;
                }
            }
            var parsed = JsonNode.Parse(new UTF8Encoding(false, true).GetString(response.ToArray()), documentOptions: new() { MaxDepth = 64 })
                ?? throw new IOException("Empty host response.");
            if (parsed["id"]?.GetValue<long>() != id || parsed["jsonrpc"]?.GetValue<string>() != "2.0") throw new IOException("Host response does not match request.");
            return parsed;
        }
        catch
        {
            if (sent) { poisoned = true; pipe.Dispose(); }
            throw;
        }
        finally { gate.Release(); }
    }

    public async Task<CallToolResult> CallAsync(string method, JsonNode parameters, CancellationToken cancellationToken)
    {
        try
        {
            var response = await SendAsync(method, parameters, cancellationToken);
            var content = new List<ContentBlock> { new TextContentBlock { Text = response.ToJsonString() } };
            var failed = response["error"] is not null;
            if (method == "world.capture" && !failed)
            {
                var path = Path.GetFullPath(response["result"]?["value"]?["path"]?.GetValue<string>() ?? "");
                if (!string.Equals(Path.GetDirectoryName(path), captureRoot, StringComparison.OrdinalIgnoreCase))
                    throw new IOException("Returned capture is outside the configured root.");
                CheckPath(path);
                var file = new FileInfo(path);
                if (file.Length < 8 || file.Length > 16 * 1024 * 1024) throw new IOException("Capture size is outside the image bound.");
                var bytes = await File.ReadAllBytesAsync(path, cancellationToken);
                if (!bytes.AsSpan(0, 8).SequenceEqual(new byte[] { 137, 80, 78, 71, 13, 10, 26, 10 })) throw new IOException("Capture is not PNG.");
                content.Add(ImageContentBlock.FromBytes(bytes, "image/png"));
            }
            return new CallToolResult { IsError = failed, Content = content };
        }
        catch (Exception e) when (e is IOException or ArgumentException or OperationCanceledException or JsonException)
        {
            return new CallToolResult { IsError = true, Content = [new TextContentBlock
            {
                Text = new JsonObject { ["error"] = e.Message, ["possibly_applied"] = true,
                    ["retry_automatically"] = false }.ToJsonString()
            }] };
        }
    }

    private static void CheckPath(string path)
    {
        var part = Path.GetPathRoot(path) ?? throw new IOException("Capture path has no root.");
        foreach (var item in path[part.Length..].Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar))
        {
            if (item.Length == 0) continue;
            part = Path.Combine(part, item);
            if ((File.Exists(part) || Directory.Exists(part)) && (File.GetAttributes(part) & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Capture path contains a reparse point.");
        }
    }

    public async ValueTask DisposeAsync()
    {
        pipe.Dispose();
        if (!process.HasExited)
        {
            process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync();
        }
        process.Dispose();
        job.Dispose();
        gate.Dispose();
    }
}
