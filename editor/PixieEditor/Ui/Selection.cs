using PixieEditor.Native;

namespace PixieEditor.Ui;

// The entities chosen in the editor, in the order they were chosen, shared by the viewport, the
// entity list, and the inspector. The first is the one the inspector names; the anchor is where a
// Shift + click range in the list starts.
internal sealed class Selection
{
    private readonly List<uint> Chosen = [];

    public IReadOnlyList<uint> Items => Chosen;
    public uint? Anchor { get; private set; }

    public bool Contains(uint entity) => Chosen.Contains(entity);

    // Only this entity.
    public void Set(uint entity) => Set([entity]);

    // Only these entities, in this order; the anchor stays at `anchor`, or moves to the first.
    public void Set(IEnumerable<uint> entities, uint? anchor = null)
    {
        Chosen.Clear();
        Add(entities);
        Anchor = anchor ?? (Chosen.Count > 0 ? Chosen[0] : null);
    }

    // These entities as well, after the ones already chosen.
    public void Add(IEnumerable<uint> entities)
    {
        foreach (var entity in entities)
        {
            if (!Chosen.Contains(entity))
                Chosen.Add(entity);
        }
    }

    // Adds the entity, or takes it out when it was chosen.
    public void Toggle(uint entity)
    {
        if (!Chosen.Remove(entity))
            Chosen.Add(entity);
        Anchor = entity;
    }

    public void Clear()
    {
        Chosen.Clear();
        Anchor = null;
    }

    // Drops the entities that no longer exist, such as one deleted in the viewport.
    public void Prune(EngineSession session)
    {
        Chosen.RemoveAll(entity => !session.IsAlive(entity));
        if (Anchor is { } anchor && !session.IsAlive(anchor))
            Anchor = null;
    }
}
