using System.Collections.Concurrent;
using System.IO.Pipes;
using System.Text;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.Principal;
using Microsoft.Win32.SafeHandles;

namespace PixieEditor.Development;

// One owner-thread command at a time. The worker only frames bytes and waits for a receipt.
internal sealed partial class EditorPipe : IDisposable
{
    internal sealed record Command(string Request, long Client, TaskCompletionSource<string> Reply);
    private readonly NamedPipeServerStream Pipe;
    private readonly CancellationTokenSource Cancellation = new();
    private readonly ConcurrentQueue<Command> Commands = new();
    private readonly Task Worker;
    private bool Disposed;
    public EditorPipe(string name)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("Editor development transport is qualified on Windows only.");
        if (name.Length is < 1 or > 64 || name.Any(c => !char.IsAsciiLetterOrDigit(c) && c != '-' && c != '_'))
            throw new ArgumentException("Invalid local channel name.");
        using var identity = WindowsIdentity.GetCurrent();
        var sid = identity.User?.Value ?? throw new IOException("Cannot identify the channel owner.");
        // The managed constructor does not set PIPE_REJECT_REMOTE_CLIENTS. Create an
        // overlapped, exclusive, owner-only native handle, then give its I/O to .NET.
        if (!Win32.ConvertDescriptor($"O:{sid}D:P(A;;GA;;;{sid})", 1, out var descriptor, out _))
            throw new Win32Exception(Marshal.GetLastPInvokeError());
        try
        {
            var attributes = new Win32.SecurityAttributes { Length = Marshal.SizeOf<Win32.SecurityAttributes>(), Descriptor = descriptor };
            var raw = Win32.CreateNamedPipe($"\\\\.\\pipe\\pixie-{name}", 3u | 0x40000000u | 0x00080000u,
                0x00000008u, 1, 65536, 65536, 0, ref attributes);
            if (raw == new nint(-1)) throw new Win32Exception(Marshal.GetLastPInvokeError());
            var handle = new SafePipeHandle(raw, ownsHandle: true);
            try { Pipe = new NamedPipeServerStream(PipeDirection.InOut, isAsync: true, isConnected: false, handle); }
            catch { handle.Dispose(); throw; }
        }
        finally { Win32.LocalFree(descriptor); }
        Worker = Task.Run(Serve);
    }
    public bool TryTake(out Command? command) => Commands.TryDequeue(out command);
    private async Task Serve()
    {
        long client = 0;
        var token = Cancellation.Token;
        try
        {
            while (!token.IsCancellationRequested)
            {
                await Pipe.WaitForConnectionAsync(token);
                ++client;
                try
                {
                    var buffer = new byte[4096];
                    var offset = 0;
                    var available = 0;
                    while (Pipe.IsConnected && !token.IsCancellationRequested)
                    {
                        using var line = new MemoryStream();
                        bool complete = false;
                        while (!complete)
                        {
                            if (offset == available)
                            {
                                available = await Pipe.ReadAsync(buffer, token);
                                offset = 0;
                                if (available == 0) throw new EndOfStreamException();
                            }
                            var value = buffer[offset++];
                            if (value == 10) { complete = true; continue; }
                            if (value == 0 || line.Length >= 65536) throw new IOException("Invalid or oversized development line.");
                            line.WriteByte(value);
                        }
                        var request = new UTF8Encoding(false, true).GetString(line.ToArray()).TrimEnd('\r');
                        var reply = new TaskCompletionSource<string>(TaskCreationOptions.RunContinuationsAsynchronously);
                        Commands.Enqueue(new Command(request, client, reply));
                        var response = await reply.Task.WaitAsync(token);
                        if (response.Length == 0) continue; // JSON-RPC notification.
                        var encoded = Encoding.UTF8.GetBytes(response);
                        if (encoded.Length >= 4 * 1024 * 1024 || response.Contains('\n')) throw new IOException("Invalid development response.");
                        await Pipe.WriteAsync(encoded, token);
                        await Pipe.WriteAsync(new byte[] { 10 }, token);
                        await Pipe.FlushAsync(token);
                    }
                }
                catch (Exception e) when (e is IOException or DecoderFallbackException) { }
                finally { if (Pipe.IsConnected) Pipe.Disconnect(); }
            }
        }
        catch (Exception e) when (e is OperationCanceledException or ObjectDisposedException or IOException) { }
    }
    public void Dispose()
    {
        if (Disposed) return;
        Disposed = true;
        Cancellation.Cancel();
        Pipe.Dispose();
        while (Commands.TryDequeue(out var command)) command.Reply.TrySetCanceled();
        Worker.GetAwaiter().GetResult();
        Cancellation.Dispose();
    }
    private static partial class Win32
    {
        [StructLayout(LayoutKind.Sequential)]
        internal struct SecurityAttributes { public int Length; public nint Descriptor; public int InheritHandle; }
        [LibraryImport("advapi32.dll", EntryPoint = "ConvertStringSecurityDescriptorToSecurityDescriptorW", SetLastError = true, StringMarshalling = StringMarshalling.Utf16)]
        [return: MarshalAs(UnmanagedType.Bool)]
        internal static partial bool ConvertDescriptor(string text, uint revision, out nint descriptor, out uint size);
        [LibraryImport("kernel32.dll", EntryPoint = "CreateNamedPipeW", SetLastError = true, StringMarshalling = StringMarshalling.Utf16)]
        internal static partial nint CreateNamedPipe(string name, uint access, uint mode, uint instances, uint output, uint input, uint timeout, ref SecurityAttributes attributes);
        [LibraryImport("kernel32.dll")]
        internal static partial nint LocalFree(nint memory);
    }
}
