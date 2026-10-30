using PixieEditor.Native;

namespace PixieEditor.Development;

// Uses the normal session step and paused rendering path; never advances wall time.
internal sealed class DeterministicCapture
{
    private readonly EngineSession Session;
    private readonly ulong Target;
    public ulong Completed { get; private set; }
    public bool Finished => Completed == Target;

    public DeterministicCapture(EngineSession session, ulong target)
    {
        Session = session;
        Target = target;
        if (session.Editing) session.Play();
        session.Paused = true; // The native Alpha() path returns exactly one while paused.
        session.Cursor(-1, -1);
    }

    public void Advance()
    {
        if (Finished) return;
        Session.Step();
        ++Completed;
    }
}
