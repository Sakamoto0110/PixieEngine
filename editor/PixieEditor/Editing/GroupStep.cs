#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
using PixieEditor.Native;

namespace PixieEditor.Editing;

// A single history entry over the existing field and record steps; no second property writer.
internal sealed class GroupStep(string name, IReadOnlyList<IStep> steps, string? before = null, string? after = null) : IStep
{
    public string Name => name;
    public uint[] Undo(EngineSession session) => Apply(session, undo: true);
    public uint[] Redo(EngineSession session) => Apply(session, undo: false);

    private uint[] Apply(EngineSession session, bool undo)
    {
        if (session.SceneFormatVersion == 2)
        {
            var document = undo ? before : after;
            if (document == null || session.RestoreHistoryDocumentCandidate == null)
                throw new EngineException("This legacy group has no complete candidate history document; no edits were applied.");
            return session.RestoreHistoryDocumentCandidate(document);
        }
        var ids = new HashSet<ulong>();
        foreach (var step in undo ? steps.Reverse() : steps)
            foreach (var entity in undo ? step.Undo(session) : step.Redo(session))
                if (session.SceneId(entity) is { } id) ids.Add(id);
        // Later record steps can invalidate handles returned by earlier field steps.
        return ids.Select(session.Find).OfType<uint>().ToArray();
    }
}
#endif
