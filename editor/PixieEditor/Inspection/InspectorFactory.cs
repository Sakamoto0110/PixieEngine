using PixieEditor.Native;
using PixieInspector;
using PixieInspector.Options;
using PixieLib;

namespace PixieEditor.Inspection;

// Builds the inspector of one component type from the engine's field descriptors: a manual
// PixieInspector with one field per descriptor field (two for a point or a vector, four for a
// rectangle), each with the node options the descriptor carries. No class per component exists on
// this side; a component added to a game's library shows up without rebuilding the editor.
internal static class InspectorFactory
{
    // The name of the button a scenario's inspector gets.
    public const string RebuildButton = "rebuild";

    // An inspector of component `type`, bound to the first handle and to the rest with AddBind. A
    // scenario type also gets a button that asks for the scene to be rebuilt with its values.
    public static Inspector Build(EngineSession session, int type, IReadOnlyList<ComponentHandle> handles, Action rebuild)
    {
        var inspector = Inspector.Create<ComponentHandle>(TypeBinderMode.Manual);
        var fields = Enumerable.Range(0, session.FieldCount(type))
            .Select(field => (Field: field, Info: session.FieldInfo(type, field)))
            .OrderBy(f => f.Info.Order)
            .ThenBy(f => f.Field);

        var row = 0;
        foreach (var (field, info) in fields)
        {
            if (info.Editor == PixieEditorKind.LayerMask) continue;
            foreach (var node in AddNodes(inspector, session, type, field, info))
            {
                ApplyOptions(node, session, type, field, info);
                node.Order = row++;
            }
        }

        if (session.IsScenario(type))
        {
            var button = inspector.AddButton(RebuildButton, "Rebuild the scene", rebuild);
            button.Label = string.Empty;
            button.Tooltip = "Throws the World away and builds it again from these values.";
            button.Order = row;
        }

        if (handles.Count > 0)
        {
            inspector.Bind(handles[0]);
            if (handles.Count > 1)
                inspector.AddBind([.. handles.Skip(1)]);
        }

        return inspector;
    }

    // The node names of a field: its own name, or one per axis or side for points, vectors, and rects.
    public static string NodeName(string field, string? part = null) => part == null ? field : field + part;

    private static IEnumerable<InspectorNode> AddNodes(Inspector inspector, EngineSession session, int type, int field, PixieFieldInfo info)
    {
        var name = session.FieldName(type, field);
        var label = session.FieldText(type, field, PixieFieldText.Label);
        var readOnly = info.Flags.HasFlag(PixieFieldFlags.ReadOnly);

        switch (info.Kind)
        {
            case PixieFieldKind.Bool:
                yield return Labeled(inspector.AddField<ComponentHandle, bool>(name,
                    h => h.Get(field).X != 0,
                    readOnly ? null : (h, v) => h.Set(field, new PixieValue { X = v ? 1 : 0 })), label);
                break;

            case PixieFieldKind.Int:
                yield return Labeled(inspector.AddField<ComponentHandle, int>(name,
                    h => (int)h.Get(field).X,
                    readOnly ? null : (h, v) => h.Set(field, new PixieValue { X = v })), label);
                break;

            case PixieFieldKind.Float:
            case PixieFieldKind.Double:
                yield return Labeled(inspector.AddField<ComponentHandle, double>(name,
                    h => h.Get(field).X,
                    readOnly ? null : (h, v) => h.Set(field, new PixieValue { X = v })), label);
                break;

            case PixieFieldKind.Enum:
                var names = Enumerable.Range(0, info.EnumCount).Select(v => session.EnumName(type, field, v)).ToArray();
                var choice = inspector.AddField<ComponentHandle, string>(name,
                    h => Name(names, (int)h.Get(field).X),
                    readOnly ? null : (h, v) => h.Set(field, new PixieValue { X = Array.IndexOf(names, v) }));
                choice.Choices = () => names;
                yield return Labeled(choice, label);
                break;

            case PixieFieldKind.Point:
            case PixieFieldKind.Vector:
                yield return Part(inspector, name, label, field, readOnly, "X", v => v.X, (ref PixieValue v, double x) => v.X = x);
                yield return Part(inspector, name, label, field, readOnly, "Y", v => v.Y, (ref PixieValue v, double y) => v.Y = y);
                break;

            case PixieFieldKind.Rect:
                yield return Part(inspector, name, label, field, readOnly, "X", v => v.X, (ref PixieValue v, double x) => v.X = x);
                yield return Part(inspector, name, label, field, readOnly, "Y", v => v.Y, (ref PixieValue v, double y) => v.Y = y);
                yield return Part(inspector, name, label, field, readOnly, "Width", v => v.Z, (ref PixieValue v, double z) => v.Z = z);
                yield return Part(inspector, name, label, field, readOnly, "Height", v => v.W, (ref PixieValue v, double w) => v.W = w);
                break;

            case PixieFieldKind.Color:
                yield return Labeled(inspector.AddField<ComponentHandle, PxColorRgba>(name,
                    h =>
                    {
                        var v = h.Get(field);
                        return new PxColorRgba((byte)v.X, (byte)v.Y, (byte)v.Z, (byte)v.W);
                    },
                    readOnly ? null : (h, c) => h.Set(field, new PixieValue { X = c.R, Y = c.G, Z = c.B, W = c.A })), label);
                break;

            case PixieFieldKind.Entity:
                var references = session.TypeIndex(session.FieldText(type, field, PixieFieldText.References));
                var entity = inspector.AddField<ComponentHandle, string>(name,
                    h => EntityLabel(session, (uint)h.Get(field).X),
                    readOnly ? null : (h, v) =>
                    {
                        if (EntityOf(session, references, v) is uint chosen)
                            h.Set(field, new PixieValue { X = chosen });
                    });
                entity.Choices = () => EntityChoices(session, references);
                yield return Labeled(entity, label);
                break;
        }
    }

    private delegate void Assign(ref PixieValue value, double part);

    // One axis or side of a point, vector, or rect: read it, and write it back with the rest unchanged.
    private static InspectorNode Part(Inspector inspector, string name, string label, int field, bool readOnly,
        string part, Func<PixieValue, double> read, Assign write)
    {
        var node = inspector.AddField<ComponentHandle, double>(NodeName(name, part),
            h => read(h.Get(field)),
            readOnly ? null : (h, x) =>
            {
                var value = h.Get(field);
                write(ref value, x);
                h.Set(field, value);
            });
        return Labeled(node, $"{label} {part.ToLowerInvariant()}");
    }

    private static InspectorNode Labeled(InspectorNode node, string label)
    {
        node.Label = label;
        return node;
    }

    // An Entity field's choices: "(none)", then every live entity that has component `references`
    // (every entity when the type is unknown), as "name #index".
    private const uint NoEntity = uint.MaxValue;
    private const uint IndexMask = 0xFFFFF;
    private const string NoEntityLabel = "(none)";
#if PIXIE_DEVELOPMENT
    // Resolve the stable wire ID immediately before using the existing node/label writer.
    internal static string ChannelReferenceLabel(EngineSession session, ulong? sceneId) => sceneId is { } id
        ? EntityLabel(session, session.Find(id) ?? throw new ArgumentException("Referenced scene entity is gone.")) : NoEntityLabel;
#endif

    private static string EntityLabel(EngineSession session, uint entity)
    {
        if (entity == NoEntity || !session.IsAlive(entity))
            return NoEntityLabel;
        var name = session.EntityName(entity);
        return name.Length > 0 ? $"{name} #{entity & IndexMask}" : $"#{entity & IndexMask}";
    }

    private static IEnumerable<uint> EntityCandidates(EngineSession session, int references) =>
        session.Entities().Where(e => references < 0 || session.Has(e, references));

    private static string[] EntityChoices(EngineSession session, int references) =>
        [NoEntityLabel, .. EntityCandidates(session, references).Select(e => EntityLabel(session, e))];

    // The entity a choice names; null when no live candidate has that label, which leaves the field as it was.
    private static uint? EntityOf(EngineSession session, int references, string label) =>
        label == NoEntityLabel
            ? NoEntity
            : EntityCandidates(session, references).Where(e => EntityLabel(session, e) == label).Select(e => (uint?)e).FirstOrDefault();

    private static string Name(string[] names, int value) =>
        value >= 0 && value < names.Length ? names[value] : value.ToString(System.Globalization.CultureInfo.InvariantCulture);

    private static void ApplyOptions(InspectorNode node, EngineSession session, int type, int field, PixieFieldInfo info)
    {
        var tooltip = session.FieldText(type, field, PixieFieldText.Tooltip);
        if (tooltip.Length > 0)
            node.Tooltip = tooltip;

        var help = session.FieldText(type, field, PixieFieldText.Help);
        if (help.Length > 0)
            node.Help = help;

        if (info.Flags.HasFlag(PixieFieldFlags.ReadOnly))
            node.ReadOnly = true;

        // A color edits with PixieInspector's color editor and its picker; left alone it would only
        // be shown as text.
        if (node.ValueType == typeof(PxColorRgba) && !node.ReadOnly)
            node.Editor = EditorKind.Color;

        var numeric = node.ValueType == typeof(double) || node.ValueType == typeof(int);
        if (!numeric)
            return;

        if (info.Flags.HasFlag(PixieFieldFlags.HasUnit))
            node.Unit = new DisplayUnit(session.FieldText(type, field, PixieFieldText.UnitSuffix), info.UnitFactor, info.UnitOffset);

        if (info.Flags.HasFlag(PixieFieldFlags.HasRange))
            node.Range = new NumericRange(info.RangeMin, info.RangeMax, info.RangeStep);

        if (info.Scrub > 0)
        {
            node.ScrubMultiplier = info.Scrub;
            node.ScrubAxis = info.ScrubAxis == 1 ? ScrubAxis.Vertical : ScrubAxis.Horizontal;
        }

        switch (info.Editor)
        {
            case PixieEditorKind.Number:
                node.Editor = EditorKind.Number;
                break;
            case PixieEditorKind.Slider:
                node.Editor = EditorKind.Slider;
                break;
            case PixieEditorKind.Arc:
                node.Editor = EditorKind.Arc;
                node.ArcSpread = session.FieldText(type, field, PixieFieldText.ArcSpread);
                break;
        }
    }
}
