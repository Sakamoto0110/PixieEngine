using System.Runtime.InteropServices;

namespace PixieEditor.Native;

// The C API of a game's library (capi/include/pixie_capi.h), through source-generated P/Invoke. The
// library name is resolved by GameLibrary to the game the editor was opened with.

public enum PixieResult
{
    Ok = 0,
    ErrorArgument = 1,
    ErrorNotFound = 2,
    ErrorGl = 3,
    ErrorBuffer = 4,
    ErrorInternal = 5,
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
}

internal static unsafe partial class PixieCapi
{
    internal const string Library = "pixie_capi";

    // The C API version this editor was written against.
    internal const int Version = 2;

    [LibraryImport(Library, EntryPoint = "pixie_api_version")]
    internal static partial int ApiVersion();

    [LibraryImport(Library, EntryPoint = "pixie_last_error")]
    internal static partial int LastError(byte* buffer, int capacity);

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
}
