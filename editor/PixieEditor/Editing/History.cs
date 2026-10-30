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
    private readonly List<IStep> Done = [];
    private readonly List<IStep> Undone = [];

    public string? UndoName => Done.Count > 0 ? Done[^1].Name : null;
    public string? RedoName => Undone.Count > 0 ? Undone[^1].Name : null;
    public int UndoCount => Done.Count;
    public int RedoCount => Undone.Count;

    public void Record(IStep step)
    {
        Done.Add(step);
        Undone.Clear();
        Console.Error.WriteLine($"[info] Did {step.Name}");
    }

    // The entities to select, or null when there was nothing to undo.
    public uint[]? Undo(EngineSession session)
    {
        if (Done.Count == 0)
            return null;
        var step = Done[^1];
        Done.RemoveAt(Done.Count - 1);
        Undone.Add(step);
        Console.Error.WriteLine($"[info] Undid {step.Name}");
        return step.Undo(session);
    }

    public uint[]? Redo(EngineSession session)
    {
        if (Undone.Count == 0)
            return null;
        var step = Undone[^1];
        Undone.RemoveAt(Undone.Count - 1);
        Done.Add(step);
        Console.Error.WriteLine($"[info] Redid {step.Name}");
        return step.Redo(session);
    }
}

// Turns the inspector's writes into steps: the first write to a field in a gesture remembers the
// value it had, and Close, once no control is held any more, records one step with every field the
// gesture changed. A scrub, a drag on a slider, or typing a value is one step from press to release.
internal sealed class EditRecorder(EngineSession session, History history, Func<bool> recording)
{
    private readonly Dictionary<(uint Entity, int Type, int Field), (ulong Id, PixieValue Before, PixieValue After)> Open = [];
    private readonly Dictionary<uint, (ulong Id, string Before)> Records = [];

    public void WriteRecord(uint entity, RecordAddress address, int field, PixieValue value)
    {
        if (!Records.ContainsKey(entity) && recording() && session.SceneId(entity) is { } id)
            Records[entity] = (id, session.Snapshot([entity]));
        session.SetRecord(entity, address.OwnerType, address.List, address.Index, field, value);
    }

    public void Write(uint entity, int type, int field, PixieValue value)
    {
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

    public static bool Same(PixieValue a, PixieValue b) => a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
}
