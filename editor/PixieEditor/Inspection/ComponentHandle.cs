using PixieEditor.Native;

namespace PixieEditor.Inspection;

// One component of one entity in the native World: what an inspector binds. Its fields are read and
// written through the C API every time, so the native World stays the only copy of the state.
public sealed class ComponentHandle(EngineSession session, uint entity, int type)
{
    public EngineSession Session { get; } = session;
    public uint Entity { get; } = entity;
    public int Type { get; } = type;

    public PixieValue Get(int field) => Session.Get(Entity, Type, field);

    public void Set(int field, PixieValue value) => Session.Set(Entity, Type, field, value);
}
