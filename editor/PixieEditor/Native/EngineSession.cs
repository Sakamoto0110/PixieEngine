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
        CheckVersion();
        Check(PixieCapi.SessionCreate(width, height, seed, out var handle), "create the session");
        return new EngineSession(handle);
    }

    // A session for the project in `folder`, which starts in edit mode.
    public static EngineSession OpenProject(int width, int height, string folder)
    {
        CheckVersion();
        Check(PixieCapi.SessionOpenProject(width, height, folder, out var handle), "open the project");
        return new EngineSession(handle) { ProjectFolder = folder };
    }

    // Makes a new project in `folder`, which must be empty or not exist yet; needs no session.
    public static void CreateProject(string folder)
    {
        CheckVersion();
        Check(PixieCapi.ProjectCreate(folder), "create the project");
    }

    private static void CheckVersion()
    {
        if (PixieCapi.ApiVersion() != PixieCapi.Version)
            throw new EngineException($"The game's library has C API version {PixieCapi.ApiVersion()}; the editor needs {PixieCapi.Version}.");
    }

    // The folder of the open project, or null for a game opened without one.
    public string? ProjectFolder { get; private init; }

    public PixieMode Mode => (PixieMode)PixieCapi.SessionMode(Handle);
    public bool Editing => Mode == PixieMode.Edit;
    public bool Unsaved => PixieCapi.ProjectUnsaved(Handle) != 0;

    public void Play() => Check(PixieCapi.SessionPlay(Handle), "play");
    public void Stop() => Check(PixieCapi.SessionStop(Handle), "stop");
    public void Save() => Check(PixieCapi.ProjectSave(Handle), "save the scene");

    // A new entity of the scene, named `name`.
    public uint CreateEntity(string name)
    {
        Check(PixieCapi.EntityCreate(Handle, name, out var entity), "create an entity");
        return entity;
    }

    public void SetName(uint entity, string name) => Check(PixieCapi.EntitySetName(Handle, entity, name), "rename an entity");

    // The entity's scene ID, which survives the World being built again; null for an entity that is
    // gone or not in the scene.
    public ulong? SceneId(uint entity) =>
        Found(PixieCapi.EntitySceneId(Handle, entity, out var id), "read a scene id") ? id : null;

    // The live entity with a scene ID, or null.
    public uint? Find(ulong id) => Found(PixieCapi.EntityFind(Handle, id, out var entity), "find an entity") ? entity : null;

    // Scene entities as text, to bring them back or copy them with Restore.
    public string Snapshot(IReadOnlyList<uint> entities)
    {
        var chosen = entities.ToArray();
        fixed (uint* list = chosen)
        {
            Check(PixieCapi.EntitiesSnapshot(Handle, list, chosen.Length, null, 0, out var length), "measure a snapshot");
            var buffer = new byte[length + 1];
            fixed (byte* text = buffer)
            {
                Check(PixieCapi.EntitiesSnapshot(Handle, list, chosen.Length, text, buffer.Length, out _), "take a snapshot");
            }

            return Encoding.UTF8.GetString(buffer, 0, length);
        }
    }

    // Adds a snapshot's entities back: with their scene IDs, or as copies with new ones. Returns them.
    public uint[] Restore(string snapshot, bool newIds)
    {
        using var document = System.Text.Json.JsonDocument.Parse(snapshot);
        var entities = new uint[document.RootElement.GetProperty("entities").GetArrayLength()];
        fixed (uint* list = entities)
        {
            Check(PixieCapi.EntitiesRestore(Handle, snapshot, newIds ? 1 : 0, list, entities.Length, out var count), "restore entities");
            return entities[..Math.Min(count, entities.Length)];
        }
    }

    public void AddComponent(uint entity, int type) => Check(PixieCapi.ComponentAdd(Handle, entity, type), $"add {TypeName(type)}");
    public void RemoveComponent(uint entity, int type) => Check(PixieCapi.ComponentRemove(Handle, entity, type), $"remove {TypeName(type)}");

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

    // The camera: a point in view pixels in world units, and back.
    public (double X, double Y) ViewToWorld(double x, double y)
    {
        Check(PixieCapi.ViewToWorld(Handle, x, y, out var worldX, out var worldY), "turn a view point into world units");
        return (worldX, worldY);
    }

    public (double X, double Y) WorldToView(double x, double y)
    {
        Check(PixieCapi.WorldToView(Handle, x, y, out var viewX, out var viewY), "turn a world point into the view");
        return (viewX, viewY);
    }

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

    // A text field, such as a sprite's image path.
    public string GetText(uint entity, int type, int field)
    {
        Check(PixieCapi.FieldGetText(Handle, entity, type, field, null, 0, out var length), "measure a text field");
        var buffer = new byte[length + 1];
        fixed (byte* text = buffer)
        {
            Check(PixieCapi.FieldGetText(Handle, entity, type, field, text, buffer.Length, out _), "read a text field");
        }

        return Encoding.UTF8.GetString(buffer, 0, length);
    }

    public void SetText(uint entity, int type, int field, string text) =>
        Check(PixieCapi.FieldSetText(Handle, entity, type, field, text), "write a text field");

    // The four corners of a sprite's rectangle as last drawn, in view pixels (the image's top-left,
    // top-right, bottom-right, bottom-left); null for an entity without a sprite.
    public (double X, double Y)[]? ViewQuad(uint entity)
    {
        var corners = stackalloc double[8];
        if (!Found(PixieCapi.EntityViewQuad(Handle, entity, corners), "read a sprite's rectangle"))
            return null;
        return [(corners[0], corners[1]), (corners[2], corners[3]), (corners[4], corners[5]), (corners[6], corners[7])];
    }

    // An image as a sprite names it: the session's texture of it and its size, or the reason it cannot
    // be read.
    public (uint Texture, int Width, int Height)? Image(string path, out string? problem)
    {
        problem = null;
        var result = PixieCapi.Image(Handle, path, out var texture, out var width, out var height);
        if (result == PixieResult.ErrorIo)
        {
            problem = LastError();
            return null;
        }

        Check(result, "read an image");
        return (texture, width, height);
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
