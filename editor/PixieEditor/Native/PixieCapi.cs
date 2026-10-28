using System.Runtime.InteropServices;

namespace PixieEditor.Native;

// The shared engine C API (capi/include/pixie_capi.h), through source-generated P/Invoke.
// GameLibrary resolves its location and selects a development module before session creation.

public enum PixieResult
{
    Ok = 0,
    ErrorArgument = 1,
    ErrorNotFound = 2,
    ErrorGl = 3,
    ErrorBuffer = 4,
    ErrorInternal = 5,
    ErrorIo = 6,
}

public enum PixieMode
{
    Play = 0,
    Edit = 1,
}

public enum PixieFieldKind
{
    Bool = 0,
    Int = 1,
    Float = 2,
    Double = 3,
    Enum = 4,
    Point = 5,
    Vector = 6,
    Rect = 7,
    Color = 8,
    Entity = 9,
    Text = 10,
    PointList = 11,
    RecordList = 12,
}

public enum PixieEditorKind
{
    Auto = 0,
    Number = 1,
    Slider = 2,
    Toggle = 3,
    Choice = 4,
    Color = 5,
    Arc = 6,
    LayerMask = 7,
    LayerNames = 8,
}

[Flags]
public enum PixieFieldFlags : uint
{
    None = 0,
    ReadOnly = 1,
    HasRange = 2,
    HasUnit = 4,
}

public enum PixieFieldText
{
    Label = 0,
    Tooltip = 1,
    Help = 2,
    UnitSuffix = 3,
    ArcSpread = 4,
    References = 5,
    Asset = 6,
}

// How a tool shows and edits a field (pixie_capi.h, PixieFieldInfo; 72 bytes).
[StructLayout(LayoutKind.Sequential)]
public struct PixieFieldInfo
{
    public PixieFieldKind Kind;
    public PixieEditorKind Editor;
    public PixieFieldFlags Flags;
    public int Order;
    public int ScrubAxis;
    public int EnumCount;
    public double RangeMin;
    public double RangeMax;
    public double RangeStep;
    public double Scrub;
    public double UnitFactor;
    public double UnitOffset;
}

[StructLayout(LayoutKind.Sequential)]
public struct PixieValue
{
    public double X;
    public double Y;
    public double Z;
    public double W;
}

[StructLayout(LayoutKind.Sequential)]
public struct PixieStats
{
    public ulong Steps;
    public double Elapsed;
    public double LastStepMilliseconds;
    public int Entities;
    public int Paused;
    public double TimeScale;
}

[StructLayout(LayoutKind.Sequential)]
public struct PixieBodyMass2D
{
    public double Mass;
    public double Area;
    public double LocalCenterX;
    public double LocalCenterY;
    public double Inertia;
}

[StructLayout(LayoutKind.Sequential)]
public struct PixieColliderHandles2D
{
    public double CenterX, CenterY, ResizeX, ResizeY, RotateX, RotateY, HeightX, HeightY;
    public uint CanRotate, CanHeight;
}

internal static unsafe partial class PixieCapi
{
    internal const string Library = "pixie_capi";

    // The C API version this editor was written against.
    internal const int Version = 17;

    [LibraryImport(Library, EntryPoint = "pixie_module_load", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult ModuleLoad(string? path);

    [LibraryImport(Library, EntryPoint = "pixie_physics_debug_get")]
    internal static partial PixieResult PhysicsDebugGet(nint session, out uint flags);
    [LibraryImport(Library, EntryPoint = "pixie_physics_debug_set")]
    internal static partial PixieResult PhysicsDebugSet(nint session, uint flags);

    [LibraryImport(Library, EntryPoint = "pixie_collider_handles_2d")]
    internal static partial PixieResult ColliderHandles2D(nint session, uint entity, int shape, out PixieColliderHandles2D handles);

    [LibraryImport(Library, EntryPoint = "pixie_collider_drag_2d")]
    internal static partial PixieResult ColliderDrag2D(nint session, uint entity, int shape, int kind, double x, double y, int snap);

    [LibraryImport(Library, EntryPoint = "pixie_body_mass_2d")]
    internal static partial PixieResult BodyMass2D(nint session, uint entity, out PixieBodyMass2D mass);

    [LibraryImport(Library, EntryPoint = "pixie_api_version")]
    internal static partial int ApiVersion();

    [LibraryImport(Library, EntryPoint = "pixie_last_error")]
    internal static partial int LastError(byte* buffer, int capacity);

    [LibraryImport(Library, EntryPoint = "pixie_movement_status")]
    internal static partial PixieResult MovementStatus(nint session, byte* buffer, int capacity, out int required);

    [LibraryImport(Library, EntryPoint = "pixie_collision_layer_name")]
    internal static partial PixieResult CollisionLayerName(nint session, int slot, byte* buffer, int capacity, out int required);

    [LibraryImport(Library, EntryPoint = "pixie_session_create")]
    internal static partial PixieResult SessionCreate(int width, int height, ulong seed, out nint session);

    [LibraryImport(Library, EntryPoint = "pixie_session_destroy")]
    internal static partial void SessionDestroy(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_project_name")]
    internal static partial PixieResult ProjectName(nint session, byte* buffer, int capacity);

    [LibraryImport(Library, EntryPoint = "pixie_session_resize")]
    internal static partial PixieResult SessionResize(nint session, int width, int height);

    [LibraryImport(Library, EntryPoint = "pixie_session_advance")]
    internal static partial PixieResult SessionAdvance(nint session, double seconds);

    [LibraryImport(Library, EntryPoint = "pixie_session_step")]
    internal static partial PixieResult SessionStep(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_session_set_paused")]
    internal static partial PixieResult SessionSetPaused(nint session, int paused);

    [LibraryImport(Library, EntryPoint = "pixie_session_set_time_scale")]
    internal static partial PixieResult SessionSetTimeScale(nint session, double scale);

    [LibraryImport(Library, EntryPoint = "pixie_type_flags")]
    internal static partial uint TypeFlags(nint session, int type);

    [LibraryImport(Library, EntryPoint = "pixie_session_reset")]
    internal static partial PixieResult SessionReset(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_session_render")]
    internal static partial PixieResult SessionRender(nint session, out uint texture);

    [LibraryImport(Library, EntryPoint = "pixie_session_stats")]
    internal static partial PixieResult SessionStats(nint session, out PixieStats stats);

    [LibraryImport(Library, EntryPoint = "pixie_input_cursor")]
    internal static partial PixieResult InputCursor(nint session, double x, double y);

    [LibraryImport(Library, EntryPoint = "pixie_input_mouse_button")]
    internal static partial PixieResult InputMouseButton(nint session, int button, int down);

    [LibraryImport(Library, EntryPoint = "pixie_input_key")]
    internal static partial PixieResult InputKey(nint session, int key, int down);

    [LibraryImport(Library, EntryPoint = "pixie_input_scroll")]
    internal static partial PixieResult InputScroll(nint session, double dx, double dy);

    [LibraryImport(Library, EntryPoint = "pixie_entities")]
    internal static partial PixieResult Entities(nint session, uint* entities, int capacity, out int count);

    [LibraryImport(Library, EntryPoint = "pixie_entity_name")]
    internal static partial PixieResult EntityName(nint session, uint entity, byte* buffer, int capacity);

    [LibraryImport(Library, EntryPoint = "pixie_entity_alive")]
    internal static partial int EntityAlive(nint session, uint entity);

    [LibraryImport(Library, EntryPoint = "pixie_type_count")]
    internal static partial int TypeCount(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_type_name")]
    internal static partial PixieResult TypeName(nint session, int type, byte* buffer, int capacity);

    [LibraryImport(Library, EntryPoint = "pixie_entity_has")]
    internal static partial int EntityHas(nint session, uint entity, int type);

    [LibraryImport(Library, EntryPoint = "pixie_field_count")]
    internal static partial int FieldCount(nint session, int type);

    [LibraryImport(Library, EntryPoint = "pixie_field_name")]
    internal static partial PixieResult FieldName(nint session, int type, int field, byte* buffer, int capacity);

    [LibraryImport(Library, EntryPoint = "pixie_field_kind")]
    internal static partial PixieResult FieldKind(nint session, int type, int field, out int kind);

    [LibraryImport(Library, EntryPoint = "pixie_field_info")]
    internal static partial PixieResult FieldInfo(nint session, int type, int field, out PixieFieldInfo info);

    [LibraryImport(Library, EntryPoint = "pixie_field_text")]
    internal static partial PixieResult FieldText(nint session, int type, int field, int which, byte* buffer, int capacity);

    [LibraryImport(Library, EntryPoint = "pixie_field_enum_name")]
    internal static partial PixieResult FieldEnumName(nint session, int type, int field, int value, byte* buffer, int capacity);

    [LibraryImport(Library, EntryPoint = "pixie_field_get")]
    internal static partial PixieResult FieldGet(nint session, uint entity, int type, int field, out PixieValue value);

    [LibraryImport(Library, EntryPoint = "pixie_field_set")]
    internal static partial PixieResult FieldSet(nint session, uint entity, int type, int field, in PixieValue value);

    [LibraryImport(Library, EntryPoint = "pixie_entity_destroy")]
    internal static partial PixieResult EntityDestroy(nint session, uint entity);

    [LibraryImport(Library, EntryPoint = "pixie_pick")]
    internal static partial PixieResult Pick(nint session, double x, double y, double tolerance, out uint entity);

    [LibraryImport(Library, EntryPoint = "pixie_pick_box")]
    internal static partial PixieResult PickBox(nint session, double x0, double y0, double x1, double y1, uint* entities, int capacity, out int count);

    [LibraryImport(Library, EntryPoint = "pixie_entity_view_bounds")]
    internal static partial PixieResult EntityViewBounds(nint session, uint entity, out double x, out double y, out double radius);

    [LibraryImport(Library, EntryPoint = "pixie_particle_remove_at")]
    internal static partial PixieResult ParticleRemoveAt(nint session, double x, double y, double radius);

    [LibraryImport(Library, EntryPoint = "pixie_spawn_at")]
    internal static partial PixieResult SpawnAt(nint session, double x, double y, out int created);

    [LibraryImport(Library, EntryPoint = "pixie_view_to_world")]
    internal static partial PixieResult ViewToWorld(nint session, double x, double y, out double worldX, out double worldY);

    [LibraryImport(Library, EntryPoint = "pixie_world_to_view")]
    internal static partial PixieResult WorldToView(nint session, double worldX, double worldY, out double x, out double y);

    [LibraryImport(Library, EntryPoint = "pixie_project_create", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult ProjectCreate(string folder);

    [LibraryImport(Library, EntryPoint = "pixie_session_open_project", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult SessionOpenProject(int width, int height, string folder, out nint session);

    [LibraryImport(Library, EntryPoint = "pixie_project_save")]
    internal static partial PixieResult ProjectSave(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_project_unsaved")]
    internal static partial int ProjectUnsaved(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_session_mode")]
    internal static partial int SessionMode(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_session_play")]
    internal static partial PixieResult SessionPlay(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_session_stop")]
    internal static partial PixieResult SessionStop(nint session);

    [LibraryImport(Library, EntryPoint = "pixie_entity_create", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult EntityCreate(nint session, string? name, out uint entity);

    [LibraryImport(Library, EntryPoint = "pixie_entity_set_name", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult EntitySetName(nint session, uint entity, string? name);

    [LibraryImport(Library, EntryPoint = "pixie_entity_scene_id")]
    internal static partial PixieResult EntitySceneId(nint session, uint entity, out ulong id);

    [LibraryImport(Library, EntryPoint = "pixie_entity_find")]
    internal static partial PixieResult EntityFind(nint session, ulong id, out uint entity);

    [LibraryImport(Library, EntryPoint = "pixie_entities_snapshot")]
    internal static partial PixieResult EntitiesSnapshot(nint session, uint* entities, int count, byte* buffer, int capacity, out int length);

    [LibraryImport(Library, EntryPoint = "pixie_entities_restore", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult EntitiesRestore(nint session, string snapshot, int ids, uint* entities, int capacity, out int count);

    [LibraryImport(Library, EntryPoint = "pixie_record_info")]
    internal static partial PixieResult RecordInfo(nint session, uint entity, int type, int list, out int itemType, out int count);

    [LibraryImport(Library, EntryPoint = "pixie_record_edit")]
    internal static partial PixieResult RecordEdit(nint session, uint entity, int type, int list, int index, int remove);

    [LibraryImport(Library, EntryPoint = "pixie_record_get")]
    internal static partial PixieResult RecordGet(nint session, uint entity, int type, int list, int index, int field, out PixieValue value);

    [LibraryImport(Library, EntryPoint = "pixie_record_set")]
    internal static partial PixieResult RecordSet(nint session, uint entity, int type, int list, int index, int field, in PixieValue value);

    [LibraryImport(Library, EntryPoint = "pixie_record_get_points")]
    internal static partial PixieResult RecordGetPoints(nint session, uint entity, int type, int list, int index, int field, double* xy, int capacity, out int count);

    [LibraryImport(Library, EntryPoint = "pixie_record_set_points")]
    internal static partial PixieResult RecordSetPoints(nint session, uint entity, int type, int list, int index, int field, double* xy, int count);

    [LibraryImport(Library, EntryPoint = "pixie_record_get_text")]
    internal static partial PixieResult RecordGetText(nint session, uint entity, int type, int list, int index, int field, byte* text, int capacity, out int required);

    [LibraryImport(Library, EntryPoint = "pixie_record_set_text", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult RecordSetText(nint session, uint entity, int type, int list, int index, int field, string text);

    [LibraryImport(Library, EntryPoint = "pixie_component_add")]
    internal static partial PixieResult ComponentAdd(nint session, uint entity, int type);

    [LibraryImport(Library, EntryPoint = "pixie_component_add_status")]
    internal static partial PixieResult ComponentAddStatus(nint session, uint entity, int type, byte* buffer, int capacity, out int required);

    [LibraryImport(Library, EntryPoint = "pixie_component_remove")]
    internal static partial PixieResult ComponentRemove(nint session, uint entity, int type);

    [LibraryImport(Library, EntryPoint = "pixie_entity_view_quad")]
    internal static partial PixieResult EntityViewQuad(nint session, uint entity, double* corners);

    [LibraryImport(Library, EntryPoint = "pixie_field_get_text")]
    internal static partial PixieResult FieldGetText(nint session, uint entity, int type, int field, byte* buffer, int capacity, out int length);

    [LibraryImport(Library, EntryPoint = "pixie_field_set_text", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult FieldSetText(nint session, uint entity, int type, int field, string text);

    [LibraryImport(Library, EntryPoint = "pixie_entity_view_path")]
    internal static partial PixieResult EntityViewPath(nint session, uint entity, double* xy, int capacity, out int count);

    [LibraryImport(Library, EntryPoint = "pixie_field_get_points")]
    internal static partial PixieResult FieldGetPoints(nint session, uint entity, int type, int field, double* xy, int capacity, out int count);

    [LibraryImport(Library, EntryPoint = "pixie_field_set_points")]
    internal static partial PixieResult FieldSetPoints(nint session, uint entity, int type, int field, double* xy, int count);

    [LibraryImport(Library, EntryPoint = "pixie_image", StringMarshalling = StringMarshalling.Utf8)]
    internal static partial PixieResult Image(nint session, string path, out uint texture, out int width, out int height);
}
