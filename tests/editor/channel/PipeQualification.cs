using System.Diagnostics;
using System.IO.Pipes;
using System.Text;
using PixieEditor.Development;

internal static class PipeQualification
{
    public static async Task Run(Action<bool, string> check)
    {
        var name = "editor-test-" + Guid.NewGuid().ToString("N");
        using var server = new EditorPipe(name);
        bool exclusive = false;
        try { using var duplicate = new EditorPipe(name); }
        catch (System.ComponentModel.Win32Exception) { exclusive = true; }
        check(exclusive, "editor-pipe-exclusive-first-instance");
        NamedPipeClientStream Client() => new(".", "pixie-" + name, PipeDirection.InOut, PipeOptions.Asynchronous);
        async Task<EditorPipe.Command> Take()
        {
            var clock = Stopwatch.StartNew();
            while (clock.Elapsed < TimeSpan.FromSeconds(5))
            {
                if (server.TryTake(out var command) && command != null) return command;
                await Task.Delay(5);
            }
            throw new TimeoutException("Editor owner queue did not receive a command.");
        }
        long firstClient;
        using (var client = Client())
        {
            await client.ConnectAsync(5000);
            using var second = Client();
            bool refused = false;
            try { await second.ConnectAsync(100); } catch (TimeoutException) { refused = true; }
            check(refused, "editor-pipe-second-client-refused");
            using var writer = new StreamWriter(client, new UTF8Encoding(false), leaveOpen: true) { AutoFlush = true };
            using var reader = new StreamReader(client, leaveOpen: true);
            await writer.WriteLineAsync("first");
            var command = await Take(); firstClient = command.Client;
            command.Reply.SetResult("receipt");
            check(await reader.ReadLineAsync().WaitAsync(TimeSpan.FromSeconds(5)) == "receipt", "editor-pipe-owner-receipt-roundtrip");
        }
        // Reconnection is accepted only after the previous stream reaches EOF.
        await Task.Delay(50);
        using (var client = Client())
        {
            await client.ConnectAsync(5000);
            await client.WriteAsync(Encoding.UTF8.GetBytes(new string('x', 65537) + "\n"));
            var buffer = new byte[1];
            bool disconnected;
            try { disconnected = await client.ReadAsync(buffer).AsTask().WaitAsync(TimeSpan.FromSeconds(5)) == 0; }
            catch (IOException) { disconnected = true; }
            check(disconnected && !server.TryTake(out _), "editor-pipe-oversized-line-disconnects-before-owner-dispatch");
        }
        await Task.Delay(50);
        using (var client = Client())
        {
            await client.ConnectAsync(5000);
            await client.WriteAsync(Encoding.UTF8.GetBytes("pending\n"));
            var command = await Take();
            check(command.Client > firstClient, "editor-pipe-reconnect-has-new-client-epoch");
            var clock = Stopwatch.StartNew();
            server.Dispose();
            check(clock.Elapsed < TimeSpan.FromSeconds(2), "editor-pipe-shutdown-cancels-inflight-reply-wait");
        }
    }
}
