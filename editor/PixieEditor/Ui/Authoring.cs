using PixieEditor.Editing;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// Changing the scene by hand: creating, deleting, duplicating, renaming, changing components, and
// moving entities, from the entity list, the viewport, the inspector, and the shortcuts. In a
// project's edit mode, each change is one step of the history; while playing, or in a game opened
// without a project, the same changes happen and nothing is recorded, since nothing of them is kept.
internal sealed class Authoring(EngineSession session, Selection selection, History history)
{
    // A shape's default size is one unit: too small to see or click.
    private const double ShapeSize = 24;
    // How far a duplicate lands from what it copies, in world units on each axis.
    private const double DuplicateOffset = 16;

    public History History => history;

    public bool Recording => session.ProjectFolder != null && session.Editing;

    // A scene entity named "entity N", with the first N no entity has, with a transform at `at` (world
    // units; the origin when null) and a circle, chosen as the selection.
    public uint? CreateEntity((double X, double Y)? at = null)
    {
        return Guard(() =>
        {
            var names = session.Entities().Select(session.EntityName).ToHashSet();
            var number = Enumerable.Range(1, names.Count + 1).First(n => !names.Contains($"entity {n}"));
            var entity = session.CreateEntity($"entity {number}");
            if (session.TypeIndex("Transform2D") is var transform and >= 0)
            {
                session.AddComponent(entity, transform);
                if (at is { } point)
                    session.Set(entity, transform, session.FieldIndex(transform, "position"), new PixieValue { X = point.X, Y = point.Y });
            }

            if (session.TypeIndex("Shape") is var shape and >= 0)
            {
                session.AddComponent(entity, shape);
                session.Set(entity, shape, session.FieldIndex(shape, "size"), new PixieValue { X = ShapeSize, Y = ShapeSize });
            }

            selection.Set(entity);
            Record($"creation of entity {number}", [entity], before: null, after: true);
            return entity;
        });
    }

    public void Delete(IReadOnlyList<uint> entities)
    {
        if (entities.Count == 0)
            return;
        Guard(() =>
        {
            var name = entities.Count == 1 ? $"deletion of {Describe(entities[0])}" : $"deletion of {entities.Count} entities";
            var (ids, before) = Take(entities);
            foreach (var entity in entities)
                session.Destroy(entity);
            selection.Prune(session);
            if (Recording && ids.Length > 0)
                history.Record(new ReplaceStep(name, ids, before, null));
            return 0;
        });
    }

    // Copies the selected scene entities a little to the side and selects the copies; entities the
    // game generates are not in the scene and are not copied.
    public void Duplicate(IReadOnlyList<uint> entities)
    {
        Guard(() =>
        {
            var inScene = entities.Where(e => session.SceneId(e) != null).ToArray();
            if (inScene.Length == 0)
            {
                Status.Show("Nothing to duplicate: only entities of the scene can be.", error: true);
                return 0;
            }

            var copies = session.Restore(session.Snapshot(inScene), newIds: true);
            if (session.TypeIndex("Transform2D") is var transform and >= 0)
            {
                var position = session.FieldIndex(transform, "position");
                foreach (var copy in copies.Where(c => session.Has(c, transform)))
                {
                    var at = session.Get(copy, transform, position);
                    session.Set(copy, transform, position, at with { X = at.X + DuplicateOffset, Y = at.Y + DuplicateOffset });
                }
            }

            selection.Set(copies);
            Record(copies.Length == 1 ? $"duplicate of {Describe(inScene[0])}" : $"duplicate of {copies.Length} entities",
                copies, before: null, after: true);
            return 0;
        });
    }

    public void Rename(uint entity, string name)
    {
        if (session.EntityName(entity) == name)
            return;
        Change([entity], $"rename to \"{name}\"", e => session.SetName(e, name));
    }

    // Changes the entities one by one, as one step: adding or removing a component.
    public void Change(IReadOnlyList<uint> entities, string name, Action<uint> change)
    {
        Guard(() =>
        {
            var (ids, before) = Take(entities);
            foreach (var entity in entities)
                change(entity);
            if (Recording && ids.Length > 0)
                history.Record(new ReplaceStep(name, ids, before, session.Snapshot(entities)));
            return 0;
        });
    }

    // Starts moving the entities that have a transform; `anchor` is the one under the cursor, which
    // snapping aligns.
    public Move? BeginMove(IReadOnlyList<uint> entities, uint anchor)
    {
        if (session.TypeIndex("Transform2D") is not (var transform and >= 0))
            return null;
        var position = session.FieldIndex(transform, "position");
        var moved = entities.Where(e => session.Has(e, transform)).ToArray();
        if (moved.Length == 0)
            return null;
        var before = moved.Select(e => session.Get(e, transform, position)).ToArray();
        var anchorAt = session.Has(anchor, transform) ? session.Get(anchor, transform, position) : before[0];
        return new Move(session, transform, position, moved, before, anchorAt);
    }

    // Ends a move: one step, when something moved and the project is in edit mode.
    public void EndMove(Move move)
    {
        var changes = new List<FieldStep.Change>();
        for (var i = 0; i < move.Entities.Length; i++)
        {
            if (!session.IsAlive(move.Entities[i]) || session.SceneId(move.Entities[i]) is not { } id)
                continue;
            var after = session.Get(move.Entities[i], move.Type, move.Field);
            if (!EditRecorder.Same(move.Before[i], after))
                changes.Add(new FieldStep.Change(id, move.Type, move.Field, move.Before[i], after));
        }

        if (Recording && changes.Count > 0)
            history.Record(new FieldStep(changes.Count == 1 ? $"move of {Describe(move.Entities[0])}" : $"move of {changes.Count} entities", changes));
    }

    public void Undo()
    {
        if (!session.Editing && session.ProjectFolder != null)
        {
            Status.Show("Undo waits for Stop.", error: true);
            return;
        }

        Guard(() =>
        {
            if (history.Undo(session) is { } chosen)
                selection.Set(chosen);
            return 0;
        });
    }

    public void Redo()
    {
        if (!session.Editing && session.ProjectFolder != null)
        {
            Status.Show("Redo waits for Stop.", error: true);
            return;
        }

        Guard(() =>
        {
            if (history.Redo(session) is { } chosen)
                selection.Set(chosen);
            return 0;
        });
    }

    public string Describe(uint entity)
    {
        var name = session.IsAlive(entity) ? session.EntityName(entity) : "";
        return name.Length > 0 ? name : $"#{entity & 0xFFFFF}";
    }

    // The scene IDs of the entities that are in the scene, and a snapshot of them (null when none).
    private (ulong[] Ids, string? Snapshot) Take(IReadOnlyList<uint> entities)
    {
        var inScene = entities.Where(e => session.SceneId(e) != null).ToArray();
        return inScene.Length == 0 ? ([], null) : (inScene.Select(e => session.SceneId(e)!.Value).ToArray(), session.Snapshot(inScene));
    }

    // Records entities that did not exist before, as they are now.
    private void Record(string name, IReadOnlyList<uint> entities, string? before, bool after)
    {
        if (!Recording)
            return;
        var (ids, snapshot) = Take(entities);
        if (ids.Length > 0)
            history.Record(new ReplaceStep(name, ids, before, after ? snapshot : null));
    }

    private static T? Guard<T>(Func<T> change)
    {
        try
        {
            return change();
        }
        catch (EngineException e)
        {
            Status.Show(e.Message, error: true);
            return default;
        }
    }
}

// A move in progress: the entities, where they started, and the one the cursor holds.
internal sealed class Move(EngineSession session, int typeIndex, int fieldIndex, uint[] entities, PixieValue[] before, PixieValue anchor)
{
    public uint[] Entities => entities;
    public PixieValue[] Before => before;
    public int Type => typeIndex;
    public int Field => fieldIndex;
    public PixieValue Anchor => anchor;

    // Places every entity at where it started, moved by (dx, dy) world units.
    public void By(double dx, double dy)
    {
        for (var i = 0; i < entities.Length; i++)
        {
            if (session.IsAlive(entities[i]))
                session.Set(entities[i], typeIndex, fieldIndex, before[i] with { X = before[i].X + dx, Y = before[i].Y + dy });
        }
    }
}
