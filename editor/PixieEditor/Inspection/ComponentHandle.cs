using PixieEditor.Native;

namespace PixieEditor.Inspection;

// One component of one entity in the native World: what an inspector binds. Its fields are read and
// written through the C API every time, so the native World stays the only copy of the state. Writes
// go through the editor's recorder, which makes them steps of the history.
internal readonly record struct RecordAddress(int OwnerType, int List, int Index);

internal sealed class ComponentHandle(EngineSession session, uint entity, int type, Editing.EditRecorder? recorder = null, RecordAddress? record = null)
{
    public EngineSession Session { get; } = session;
    public uint Entity { get; } = entity;
    public int Type { get; } = type;

    public PixieValue Get(int field) => record is { } r
        ? Session.GetRecord(Entity, r.OwnerType, r.List, r.Index, field) : Session.Get(Entity, Type, field);

    public void Set(int field, PixieValue value)
    {
        if (record is { } r)
        {
            if (recorder != null) recorder.WriteRecord(Entity, r, field, value);
            else Session.SetRecord(Entity, r.OwnerType, r.List, r.Index, field, value);
            return;
        }
        if (recorder != null)
            recorder.Write(Entity, Type, field, value);
        else
            Session.Set(Entity, Type, field, value);
    }
}
