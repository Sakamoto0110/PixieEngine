using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace PixieMcp;

// A Windows job that closes the host, and every process it started, when this server's handle closes:
// on a normal exit, and also when the server is killed or crashes. Without one, a hidden host would
// outlive its client.
internal sealed partial class HostJob : IDisposable
{
    private readonly SafeFileHandle? handle;
    private HostJob(SafeFileHandle? handle) => this.handle = handle;

    public static HostJob Contain(Process process)
    {
        var handle = CreateJobObject(0, null);
        if (handle.IsInvalid) return Unavailable(handle, "the job could not be created");
        var limits = new ExtendedLimitInformation { Basic = new BasicLimitInformation { LimitFlags = KillOnJobClose } };
        if (!SetInformationJobObject(handle, ExtendedLimitInformationClass, ref limits, Marshal.SizeOf<ExtendedLimitInformation>()))
            return Unavailable(handle, "the job's limits could not be set");
        if (!AssignProcessToJobObject(handle, process.Handle))
            return Unavailable(handle, "the host could not join the job");
        return new HostJob(handle);
    }

    // The host still runs and closes with the server's normal exit; only a crash would leave it behind.
    private static HostJob Unavailable(SafeFileHandle handle, string reason)
    {
        Console.Error.WriteLine($"[warning] The host is not tied to this server ({reason}, error {Marshal.GetLastPInvokeError()}).");
        handle.Dispose();
        return new HostJob(null);
    }

    public void Dispose() => handle?.Dispose();

    private const uint KillOnJobClose = 0x2000;
    private const int ExtendedLimitInformationClass = 9;

    [StructLayout(LayoutKind.Sequential)]
    private struct BasicLimitInformation
    {
        public long PerProcessUserTimeLimit;
        public long PerJobUserTimeLimit;
        public uint LimitFlags;
        public nuint MinimumWorkingSetSize;
        public nuint MaximumWorkingSetSize;
        public uint ActiveProcessLimit;
        public nuint Affinity;
        public uint PriorityClass;
        public uint SchedulingClass;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct IoCounters
    {
        public ulong ReadOperationCount, WriteOperationCount, OtherOperationCount;
        public ulong ReadTransferCount, WriteTransferCount, OtherTransferCount;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct ExtendedLimitInformation
    {
        public BasicLimitInformation Basic;
        public IoCounters Io;
        public nuint ProcessMemoryLimit;
        public nuint JobMemoryLimit;
        public nuint PeakProcessMemoryUsed;
        public nuint PeakJobMemoryUsed;
    }

    [LibraryImport("kernel32.dll", EntryPoint = "CreateJobObjectW", SetLastError = true, StringMarshalling = StringMarshalling.Utf16)]
    private static partial SafeFileHandle CreateJobObject(nint attributes, string? name);

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool SetInformationJobObject(SafeFileHandle job, int informationClass, ref ExtendedLimitInformation information, int length);

    [LibraryImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static partial bool AssignProcessToJobObject(SafeFileHandle job, nint process);
}
