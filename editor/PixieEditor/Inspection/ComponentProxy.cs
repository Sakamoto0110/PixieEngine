using PixieEditor.Native;
using PixieLib;

namespace PixieEditor.Inspection;

// The base of the hand-written proxies: a C# object whose properties read and write one component of
// one entity through the C API, so PixieInspector, which reads the properties on every frame, always
// shows the live values. The native World stays the only copy of the state.
//
// Helpers here are protected, so PixieInspector, which reflects public properties, sees only the
// proxy's own.
public abstract class ComponentProxy
{
    private readonly EngineSession Session;
    private readonly uint Entity;
    private readonly int Type;

    private protected ComponentProxy(EngineSession session, uint entity, string typeName)
    {
        Session = session;
        Entity = entity;
        Type = session.TypeIndex(typeName);
        if (Type < 0)
            throw new EngineException($"The game has no component type '{typeName}'.");
    }

    private PixieValue Read(string field) => Session.Get(Entity, Type, Session.FieldIndex(Type, field));

    private void Write(string field, PixieValue value) => Session.Set(Entity, Type, Session.FieldIndex(Type, field), value);

    protected double GetNumber(string field) => Read(field).X;

    protected void SetNumber(string field, double value) => Write(field, new PixieValue { X = value });

    protected bool GetBool(string field) => Read(field).X != 0.0;

    protected void SetBool(string field, bool value) => SetNumber(field, value ? 1.0 : 0.0);

    protected T GetEnum<T>(string field) where T : struct, Enum => (T)Enum.ToObject(typeof(T), (int)Read(field).X);

    protected void SetEnum<T>(string field, T value) where T : struct, Enum => SetNumber(field, Convert.ToInt32(value));

    // One axis of a point or vector field; the other keeps its value.
    protected double GetX(string field) => Read(field).X;

    protected double GetY(string field) => Read(field).Y;

    protected void SetX(string field, double x)
    {
        var value = Read(field);
        value.X = x;
        Write(field, value);
    }

    protected void SetY(string field, double y)
    {
        var value = Read(field);
        value.Y = y;
        Write(field, value);
    }

    protected PxColorRgba GetColor(string field)
    {
        var v = Read(field);
        return new PxColorRgba((byte)v.X, (byte)v.Y, (byte)v.Z, (byte)v.W);
    }

    protected void SetColor(string field, PxColorRgba color) =>
        Write(field, new PixieValue { X = color.R, Y = color.G, Z = color.B, W = color.A });
}
