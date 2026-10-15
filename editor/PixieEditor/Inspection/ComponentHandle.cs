using PixieEditor.Native;

namespace PixieEditor.Inspection;

// One component of one entity in the native World: what an inspector binds. Its fields are read and
// written through the C API every time, so the native World stays the only copy of the state. Writes
// go through the editor's recorder, which makes them steps of the history.
internal sealed class ComponentHandle(EngineSession session, uint entity, int type, Editing.EditRecorder? recorder = null)
{
    public EngineSession Session { get; } = session;
    public uint Entity { get; } = entity;
    public int Type { get; } = type;

    public PixieValue Get(int field) => Session.Get(Entity, Type, field);

    public void Set(int field, PixieValue value)
    {
        if (recorder != null)
            recorder.Write(Entity, Type, field, value);
        else
            Session.Set(Entity, Type, field, value);
    }
}
