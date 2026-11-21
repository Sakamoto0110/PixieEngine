using PixieEditor.Native;

namespace PixieEditor.Editing;

// One serial admission boundary shared by GUI, scripted edits and development dispatch.
// It owns no World and launches no work. Read-only observations remain separate.
internal sealed class SessionEditGate
{
    private enum Phase { Ready, Preparing, Recovery }
    private Phase Current;
    private readonly int OwnerThread = Environment.CurrentManagedThreadId;
    internal Func<bool>? GestureActive { get; set; }
    internal Func<bool>? RetryRecovery { get; set; }
    internal bool IsBlocked => Current != Phase.Ready;
    internal bool RecoveryRequired => Current == Phase.Recovery;
    internal bool BindingRecovery { get; private set; }

    internal void RequireMutation()
    {
        if (Environment.CurrentManagedThreadId != OwnerThread)
            throw new EngineException("Editor mutations require the session owner thread.");
        if (IsBlocked) throw new EngineException("The World edit is committed or preparing; finish Inspector recovery before editing.");
    }

    internal void RequireCandidateAdmission(bool localGesture)
    {
        RequireMutation();
        if (localGesture || GestureActive?.Invoke() == true)
            throw new EngineException("Finish the active Inspector or viewport gesture before candidate editing.");
    }

    internal Lease BeginCandidate(bool localGesture)
    {
        RequireCandidateAdmission(localGesture);
        var lease = new Lease(this); // Allocate before changing state or publishing anything.
        Current = Phase.Preparing;
        return lease;
    }

    internal void RequirePreparation()
    {
        if (Current != Phase.Preparing || Environment.CurrentManagedThreadId != OwnerThread)
            throw new EngineException("Candidate publication requires its serial preparation lease.");
    }
    internal void Published() => Current = Phase.Recovery;
    internal void Recovered() => Current = Phase.Ready;
    internal void BeginBindingRecovery() => BindingRecovery = true;
    internal void EndBindingRecovery() => BindingRecovery = false;

    internal static bool IsReadOnlyWorldCommand(string method, System.Text.Json.Nodes.JsonObject? parameters = null) =>
        method is "world.hello" or "world.stats" or "world.types" or "world.type" or "world.entities" or
            "world.entity" or "world.get" or "world.pick" or "world.pick_box" or "world.view_to_world" or "world.world_to_view" ||
        method == "world.lighting_debug" && parameters?.ContainsKey("flags") != true;

    internal sealed class Lease(SessionEditGate owner) : IDisposable
    {
        public void Dispose() { if (owner.Current == Phase.Preparing) owner.Current = Phase.Ready; }
    }
}
