using PixieEditor.Native;
using PixieEditor.Inspection;

namespace PixieEditor.Editing;

// One change to the scene that can be undone and redone. It names entities by scene ID, since their
// handles change whenever the World is built again (Play, Stop, Rebuild, or restoring them).
internal interface IStep
{
    // What it did, for the Edit menu: "move of 3 entities".
    string Name { get; }

    // Each returns the entities to select afterwards.
    uint[] Undo(EngineSession session);
    uint[] Redo(EngineSession session);
}

// Fields written from one value to another: an edit in the inspector, or a move in the viewport.
internal sealed class FieldStep(string name, IReadOnlyList<FieldStep.Change> changes) : IStep
{
    public sealed record Change(ulong Entity, int Type, int Field, PixieValue Before, PixieValue After);

    public string Name => name;

    public uint[] Undo(EngineSession session) => Apply(session, after: false);
    public uint[] Redo(EngineSession session) => Apply(session, after: true);

    private uint[] Apply(EngineSession session, bool after)
    {
        var touched = new List<uint>();
        foreach (var change in changes)
        {
            if (session.Find(change.Entity) is not { } entity)
                continue;
            session.Set(entity, change.Type, change.Field, after ? change.After : change.Before);
            if (!touched.Contains(entity))
                touched.Add(entity);
        }

        return [.. touched];
    }
}

// Entities replaced as a whole, from one snapshot to another: creating them (no snapshot before),
// deleting them (none after), duplicating, adding or removing components, and renaming.
internal sealed class ReplaceStep(string name, IReadOnlyList<ulong> entities, string? before, string? after) : IStep
{
    public string Name => name;

    public uint[] Undo(EngineSession session) => Apply(session, before);
    public uint[] Redo(EngineSession session) => Apply(session, after);

    private uint[] Apply(EngineSession session, string? state)
    {
        if (session.SceneFormatVersion == 2)
            return session.RestoreHistoryFragmentCandidate?.Invoke(state ?? "", entities.ToArray())
                ?? throw new EngineException("Version2 replacement history requires candidate publication.");
        if (state != null) session.ValidateRestore(state, newIds: false, replacing: entities);
        foreach (var id in entities)
        {
            if (session.Find(id) is { } entity)
                session.Destroy(entity);
        }

        return state == null ? [] : session.Restore(state, newIds: false);
    }
}

// The steps of this session, newest last. A new step clears what was undone. It lives in memory only,
// and outlasts Play, Stop, and Save, which do not change what the scene holds.
internal sealed class History
{
    internal SessionEditGate? Gate { get; set; }
    internal long Revision { get; private set; }
    private readonly List<IStep> Done = [];
    private readonly List<IStep> Undone = [];

    public string? UndoName => Done.Count > 0 ? Done[^1].Name : null;
    public string? RedoName => Undone.Count > 0 ? Undone[^1].Name : null;
    public int UndoCount => Done.Count;
    public int RedoCount => Undone.Count;
    internal IStep? NextUndo => Done.Count == 0 ? null : Done[^1];
    internal IStep? NextRedo => Undone.Count == 0 ? null : Undone[^1];

    // Capacity is reserved before a candidate can commit. The serial publisher invokes no host
    // callbacks between this reservation and RecordPrepared; the existing UI is not exposed yet.
    internal void ReserveRecord() => Done.EnsureCapacity(Done.Count + 1);
    internal void RecordPrepared(IStep step)
    {
        Done.Add(step);
        Undone.Clear();
        ++Revision;
    }

    public void Record(IStep step)
    {
        Gate?.RequireMutation();
        Done.Add(step);
        Undone.Clear();
        ++Revision;
        Console.Error.WriteLine($"[info] Did {step.Name}");
    }

    // The entities to select, or null when there was nothing to undo.
    public uint[]? Undo(EngineSession session) { Gate?.RequireMutation(); return UndoCore(session, true); }
    internal uint[]? UndoPrepared(EngineSession session) => UndoCore(session, false);
    private uint[]? UndoCore(EngineSession session, bool log)
    {
        if (Done.Count == 0)
            return null;
        var step = Done[^1];
        Undone.EnsureCapacity(Undone.Count + 1);
        var selected = step.Undo(session);
        Undone.Add(step);
        Done.RemoveAt(Done.Count - 1);
        ++Revision;
        if (log) Console.Error.WriteLine($"[info] Undid {step.Name}");
        return selected;
    }

    public uint[]? Redo(EngineSession session) { Gate?.RequireMutation(); return RedoCore(session, true); }
    internal uint[]? RedoPrepared(EngineSession session) => RedoCore(session, false);
    private uint[]? RedoCore(EngineSession session, bool log)
    {
        if (Undone.Count == 0)
            return null;
        var step = Undone[^1];
        Done.EnsureCapacity(Done.Count + 1);
        var selected = step.Redo(session);
        Done.Add(step);
        Undone.RemoveAt(Undone.Count - 1);
        ++Revision;
        if (log) Console.Error.WriteLine($"[info] Redid {step.Name}");
        return selected;
    }
}

// Turns the inspector's writes into steps: the first write to a field in a gesture remembers the
// value it had, and Close, once no control is held any more, records one step with every field the
// gesture changed. A scrub, a drag on a slider, or typing a value is one step from press to release.
internal sealed class EditRecorder(EngineSession session, History history, Func<bool> recording)
{
    private readonly Dictionary<(uint Entity, int Type, int Field), (ulong Id, PixieValue Before, PixieValue After)> Open = [];
    private readonly Dictionary<uint, (ulong Id, string Before)> Records = [];
    internal SceneDocumentEditor? SceneEditor { get; set; }
    private SceneDocumentEditor.Observation? WholeChange;
    private void ObserveWholeChange()
    {
        if (WholeChange == null && recording() && session.SceneFormatVersion == 2)
            WholeChange = SceneEditor?.ObserveChange()
                ?? throw new EngineException("Version2 property history requires the candidate coordinator.");
    }
    internal bool HasPendingEdits => WholeChange != null || Open.Count != 0 || Records.Count != 0
#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
        || Grouping
#endif
        ;

#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
    private bool Grouping;
    private string? GroupBefore;
    private readonly Dictionary<uint, (ulong Id, string Before)> RecordStarts = [];

    public void BeginInteraction(IReadOnlyList<uint> recordEntities)
    {
        session.Edits.RequireMutation();
        Close();
        GroupBefore = recording() && session.SceneFormatVersion == 1 ? session.ProjectSnapshot() : null;
        if (recording())
            foreach (var entity in recordEntities.Distinct())
                if (session.SceneId(entity) is { } id)
                    RecordStarts[entity] = (id, session.Snapshot([entity]));
        Grouping = true;
    }

    private void CloseInteraction()
    {
        if (CloseWholeChange()) return;
        var records = new List<IStep>();
        foreach (var (entity, change) in Records)
        {
            if (!session.IsAlive(entity)) continue;
            var after = session.Snapshot([entity]);
            if (change.Before != after)
                records.Add(new ReplaceStep("edit of shape properties", [change.Id], change.Before, after));
        }
        var changes = Open.Where(c => !Same(c.Value.Before, c.Value.After))
            .Select(c => new FieldStep.Change(c.Value.Id, c.Key.Type, c.Key.Field, c.Value.Before, c.Value.After)).ToArray();
        var steps = new List<IStep>();
        if (changes.Length > 0) steps.Add(new FieldStep("edit of field properties", changes));
        // Redo fields before records, undo in reverse: restoring a record replaces its entity.
        // Record snapshots start before the entire gesture, including preceding Shape writes.
        steps.AddRange(records);
        if (steps.Count > 0)
            history.Record(GroupBefore != null
                ? new GroupStep(steps.Count == 1 ? steps[0].Name : "edit of chained properties", steps,
                    GroupBefore, session.ProjectSnapshot())
                : steps.Count == 1 ? steps[0] : new GroupStep("edit of chained properties", steps));
        Grouping = false;
        GroupBefore = null;
        RecordStarts.Clear(); Records.Clear(); Open.Clear();
    }
#endif

    public void WriteRecord(uint entity, RecordAddress address, int field, PixieValue value)
    {
        session.Edits.RequireMutation();
        ObserveWholeChange();
        if (!Records.ContainsKey(entity) && recording() && session.SceneId(entity) is { } id)
        {
#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
            Records[entity] = RecordStarts.TryGetValue(entity, out var start) ? start : (id, session.Snapshot([entity]));
#else
            Records[entity] = (id, session.Snapshot([entity]));
#endif
        }
        session.SetRecord(entity, address.OwnerType, address.List, address.Index, field, value);
    }

    public void Write(uint entity, int type, int field, PixieValue value)
    {
        session.Edits.RequireMutation();
        ObserveWholeChange();
        var key = (entity, type, field);
        var existing = Open.TryGetValue(key, out var change);
        var sceneId = existing ? change.Id : recording() ? session.SceneId(entity) : null;
        var before = existing ? change.Before : sceneId != null ? session.Get(entity, type, field) : default;
        var succeeded = false;
        try { session.Set(entity, type, field, value); succeeded = true; }
        finally
        {
            if (sceneId is { } id)
            {
                try
                {
                    // Record observed storage, never a requested value before the setter ran.
                    // A throwing setter may still have changed storage; preserve that effect
                    // when it can be read, without replacing the original native failure.
                    var actual = session.Get(entity, type, field);
                    if (existing || !Same(before, actual)) Open[key] = (id, before, actual);
                }
                catch (EngineException) when (!succeeded) { }
            }
        }
    }

    public void Close()
    {
        if (HasPendingEdits) session.Edits.RequireMutation();
        if (CloseWholeChange()) return;
#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
        if (Grouping) { CloseInteraction(); return; }
#endif
        foreach (var (entity, change) in Records)
        {
            if (!session.IsAlive(entity)) continue;
            var after = session.Snapshot([entity]);
            if (change.Before != after)
                history.Record(new ReplaceStep("edit of shape properties", [change.Id], change.Before, after));
        }
        Records.Clear();
        if (Open.Count == 0)
            return;

        var changes = Open
            .Where(c => !Same(c.Value.Before, c.Value.After))
            .Select(c => new FieldStep.Change(c.Value.Id, c.Key.Type, c.Key.Field, c.Value.Before, c.Value.After))
            .ToList();
        Open.Clear();
        if (changes.Count == 0)
            return;

        var fields = changes.Select(c => (c.Type, c.Field)).Distinct().ToList();
        var name = fields.Count == 1
            ? $"edit of {session.TypeName(fields[0].Type)}.{session.FieldName(fields[0].Type, fields[0].Field)}"
            : $"edit of {fields.Count} fields";
        history.Record(new FieldStep(name, changes));
    }

    private bool CloseWholeChange()
    {
        if (WholeChange == null) return false;
        SceneEditor!.RecordExistingChange(WholeChange);
        WholeChange = null;
        Records.Clear(); Open.Clear();
#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
        Grouping = false; GroupBefore = null; RecordStarts.Clear();
#endif
        return true;
    }

    public static bool Same(PixieValue a, PixieValue b) => a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
}
