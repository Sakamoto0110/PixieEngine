using System.Text;

namespace PixieEditor.Native;

// A failure reported by the C API, with its last error text.
public sealed class EngineException(string message) : Exception(message);

// A game's session through the C API: the managed face of PixieSession. Every call needs the GL
// context the session was created with current on this thread. A failed call throws an
// EngineException with the engine's reason.
public sealed unsafe class EngineSession : IDisposable
{
    private nint Handle;
    private readonly Dictionary<string, int> TypeIndices = [];
    private readonly Dictionary<(int Type, string Field), int> FieldIndices = [];

    private EngineSession(nint handle)
    {
        Handle = handle;
        for (var t = 0; t < TypeCount; t++)
            TypeIndices[TypeName(t)] = t;
    }

    public static EngineSession Create(int width, int height, ulong seed)
    {
        if (PixieCapi.ApiVersion() != PixieCapi.Version)
            throw new EngineException($"The game's library has C API version {PixieCapi.ApiVersion()}; the editor needs {PixieCapi.Version}.");

        Check(PixieCapi.SessionCreate(width, height, seed, out var handle), "create the session");
        return new EngineSession(handle);
    }

    public void Dispose()
    {
        if (Handle == 0)
            return;

        PixieCapi.SessionDestroy(Handle);
        Handle = 0;
    }

    public string ProjectName => ReadString((b, n) => PixieCapi.ProjectName(Handle, b, n), "read the project name");

    public PixieStats Stats
    {
        get
        {
            Check(PixieCapi.SessionStats(Handle, out var stats), "read the stats");
            return stats;
        }
    }

    public bool Paused
    {
        get => Stats.Paused != 0;
        set => Check(PixieCapi.SessionSetPaused(Handle, value ? 1 : 0), "pause or resume");
    }

    public double TimeScale
    {
        get => Stats.TimeScale;
        set => Check(PixieCapi.SessionSetTimeScale(Handle, value), "set the time scale");
    }

    // The component type holds a scenario: values the scene is built from.
    public bool IsScenario(int type) => (PixieCapi.TypeFlags(Handle, type) & 1u) != 0;

    public void Resize(int width, int height) => Check(PixieCapi.SessionResize(Handle, width, height), "resize");
    public void Advance(double seconds) => Check(PixieCapi.SessionAdvance(Handle, seconds), "advance");
    public void Step() => Check(PixieCapi.SessionStep(Handle), "step");
    public void Reset() => Check(PixieCapi.SessionReset(Handle), "reset");

    public uint Render()
    {
        Check(PixieCapi.SessionRender(Handle, out var texture), "render");
        return texture;
    }

    public void Cursor(double x, double y) => Check(PixieCapi.InputCursor(Handle, x, y), "send the cursor");
    public void MouseButton(int button, bool down) => Check(PixieCapi.InputMouseButton(Handle, button, down ? 1 : 0), "send a button");
    public void Key(int key, bool down) => Check(PixieCapi.InputKey(Handle, key, down ? 1 : 0), "send a key");
    public void Scroll(double dx, double dy) => Check(PixieCapi.InputScroll(Handle, dx, dy), "send the scroll");

    // Live entities, oldest first.
    public uint[] Entities()
    {
        Check(PixieCapi.Entities(Handle, null, 0, out var count), "count the entities");
        var entities = new uint[count];
        fixed (uint* buffer = entities)
        {
            Check(PixieCapi.Entities(Handle, buffer, count, out _), "list the entities");
        }
        return entities;
    }

    public string EntityName(uint entity) => ReadString((b, n) => PixieCapi.EntityName(Handle, entity, b, n), "read a name");
    public bool IsAlive(uint entity) => PixieCapi.EntityAlive(Handle, entity) != 0;

    // Destroys an entity at once; false when it was already gone.
    public bool Destroy(uint entity) => Found(PixieCapi.EntityDestroy(Handle, entity), "destroy an entity");

    // Picking, in view pixels: the entity drawn under a point (widened by `tolerance` pixels), or null.
    public uint? Pick(double x, double y, double tolerance) =>
        Found(PixieCapi.Pick(Handle, x, y, tolerance, out var entity), "pick") ? entity : null;

    // The entities whose center lies in the box between two corners, oldest first.
    public uint[] PickBox(double x0, double y0, double x1, double y1)
    {
        Check(PixieCapi.PickBox(Handle, x0, y0, x1, y1, null, 0, out var count), "count the entities in a box");
        var entities = new uint[count];
        fixed (uint* buffer = entities)
        {
            Check(PixieCapi.PickBox(Handle, x0, y0, x1, y1, buffer, count, out _), "list the entities in a box");
        }
        return entities;
    }

    // Where a pickable entity was drawn by the last render, in view pixels; null for any other entity.
    public (double X, double Y, double Radius)? ViewBounds(uint entity) =>
        Found(PixieCapi.EntityViewBounds(Handle, entity, out var x, out var y, out var radius), "read an entity's bounds")
            ? (x, y, radius)
            : null;

    // Removes the particle nearest a point within `radius` pixels; false when none is that close.
    public bool RemoveParticleAt(double x, double y, double radius) =>
        Found(PixieCapi.ParticleRemoveAt(Handle, x, y, radius), "remove a particle");

    // Asks the game to create something at a point in view pixels; false when it creates nothing.
    public bool SpawnAt(double x, double y)
    {
        Check(PixieCapi.SpawnAt(Handle, x, y, out var created), "spawn");
        return created != 0;
    }

    public int TypeCount => PixieCapi.TypeCount(Handle);
    public string TypeName(int type) => ReadString((b, n) => PixieCapi.TypeName(Handle, type, b, n), "read a type name");
    public int TypeIndex(string name) => TypeIndices.TryGetValue(name, out var index) ? index : -1;
    public bool Has(uint entity, int type) => PixieCapi.EntityHas(Handle, entity, type) != 0;

    public int FieldIndex(int type, string field)
    {
        if (FieldIndices.TryGetValue((type, field), out var index))
            return index;

        var count = PixieCapi.FieldCount(Handle, type);
        for (var f = 0; f < count; f++)
        {
            if (ReadString((b, n) => PixieCapi.FieldName(Handle, type, f, b, n), "read a field name") == field)
            {
                FieldIndices[(type, field)] = f;
                return f;
            }
        }

        throw new EngineException($"Type {TypeName(type)} has no field '{field}'.");
    }

    public int FieldCount(int type) => PixieCapi.FieldCount(Handle, type);

    public string FieldName(int type, int field) =>
        ReadString((b, n) => PixieCapi.FieldName(Handle, type, field, b, n), "read a field name");

    public PixieFieldInfo FieldInfo(int type, int field)
    {
        Check(PixieCapi.FieldInfo(Handle, type, field, out var info), "read a field's metadata");
        return info;
    }

    public string FieldText(int type, int field, PixieFieldText which) =>
        ReadString((b, n) => PixieCapi.FieldText(Handle, type, field, (int)which, b, n), "read a field's text");

    public string EnumName(int type, int field, int value) =>
        ReadString((b, n) => PixieCapi.FieldEnumName(Handle, type, field, value, b, n), "read an enum name");

    public PixieValue Get(uint entity, int type, int field)
    {
        Check(PixieCapi.FieldGet(Handle, entity, type, field, out var value), "read a field");
        return value;
    }

    public void Set(uint entity, int type, int field, PixieValue value)
    {
        Check(PixieCapi.FieldSet(Handle, entity, type, field, in value), "write a field");
    }

    private static string LastError()
    {
        var buffer = stackalloc byte[512];
        var length = PixieCapi.LastError(buffer, 512);
        return Encoding.UTF8.GetString(buffer, Math.Min(length, 511));
    }

    // Ok is true, not found is false, and anything else throws.
    private static bool Found(PixieResult result, string what)
    {
        if (result == PixieResult.ErrorNotFound)
            return false;
        Check(result, what);
        return true;
    }

    private static void Check(PixieResult result, string what)
    {
        if (result != PixieResult.Ok)
            throw new EngineException($"Could not {what}: {result}, {LastError()}");
    }

    private delegate PixieResult StringReader(byte* buffer, int capacity);

    private static string ReadString(StringReader read, string what)
    {
        var buffer = stackalloc byte[256];
        Check(read(buffer, 256), what);
        return Encoding.UTF8.GetString(buffer, new ReadOnlySpan<byte>(buffer, 256).IndexOf((byte)0));
    }
}
