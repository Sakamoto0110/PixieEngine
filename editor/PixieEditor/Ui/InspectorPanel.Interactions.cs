#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieInspector;
using PixieInspector.Options;
using PixieInspector.ImGui;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private bool InteractionSubscribed;
    private ImGuiEditIcons? InteractionIcons;
    private bool IconsLoaded;
    private string? InteractionProblem;
    private sealed class CustomControlState;
    private Inspector? CustomControls;
    private readonly Dictionary<string, InspectorNode> CustomControlNodes = [];

    // Custom list/text/mask editors retain their native handlers. The surrogate owns UI state only.
    private bool DrawCustomEditControls(int type, int field, RecordAddress? record = null)
    {
        var key = $"{type}:{field}:{record?.OwnerType}:{record?.List}:{record?.Index}";
        if (!CustomControlNodes.TryGetValue(key, out var node))
        {
            if (CustomControls == null)
            {
                CustomControls = Inspector.Create<CustomControlState>(TypeBinderMode.Manual);
                CustomControls.Bind(new CustomControlState());
            }
            node = CustomControls.AddField<CustomControlState, bool>(key, _ => false, (_, _) => { });
            node.Label = session.FieldText(type, field, Native.PixieFieldText.Label);
            node.ReadOnly = session.FieldInfo(type, field).Flags.HasFlag(Native.PixieFieldFlags.ReadOnly);
            CustomControlNodes[key] = node;
        }
        Sections[0].View.DrawEditControls(node);
        return node.IsEditLocked || node.ReadOnly;
    }

    private void ReleaseCustomEditControls()
    {
        CustomControls?.Dispose();
        CustomControls = null;
        CustomControlNodes.Clear();
    }

    internal bool IsInteracting => Sections.Any(s => s.View.IsEditing) || RecordSections.Values.Any(s => s.View.IsEditing);

    private unsafe void ConfigureInteraction(ImGuiInspectorView view)
    {
        if (!InteractionSubscribed)
        {
            session.BindingsReplacing += BeforeBindingsReplace;
            InteractionSubscribed = true;
        }
        if (!IconsLoaded)
        {
            IconsLoaded = true;
            var folder = Path.Combine(AppContext.BaseDirectory, "assets", "inspector");
            var locked = session.Image(Path.Combine(folder, "lock.png"), out _);
            var chain = session.Image(Path.Combine(folder, "chain.png"), out _);
            if (locked is { } l && chain is { } c)
                InteractionIcons = new(new ImTextureRef(null, new ImTextureID(l.Texture)), new ImTextureRef(null, new ImTextureID(c.Texture)));
        }
        view.EditIcons = InteractionIcons;
        view.InteractionStarted += (_, args) => recorder.BeginInteraction(args.Gesture.Members
            .SelectMany(node => node.Inspector.Instances).OfType<ComponentHandle>()
            .Where(handle => handle.Record != null).Select(handle => handle.Entity).Distinct().ToArray());
        view.InteractionFinished += (_, args) =>
        {
            recorder.Close();
            InteractionProblem = args.Error?.Message ?? (args.Result is { Accepted: false } ? "Some property edits were refused. Actual changes remain undoable." : null);
        };
    }

    private void BeforeBindingsReplace()
    {
        // Finish against the old World. Nothing may replay a rollback into its replacement.
        foreach (var section in Sections) section.View.EndInteraction();
        foreach (var section in RecordSections.Values) section.View.EndInteraction();
        recorder.Close();
    }

    private void ConfigureDimensionChains()
    {
        var shape = Sections.FirstOrDefault(s => s.Name == "Shape");
        var collider = Sections.FirstOrDefault(s => s.Name == "Collider2D");
        if (shape == null) return;
        // Collider metadata converts halfSize to full displayed size (factor 2).
        // Core gestures capture and apply VIEW values, including each selected bind.
        var dimensions = new List<InspectorNode> { shape.Inspector["sizeX"], shape.Inspector["sizeY"] };
        if (collider != null)
            dimensions.AddRange([collider.Inspector["halfSizeX"], collider.Inspector["halfSizeY"]]);
        var compound = Sections.FirstOrDefault(s => s.Name == "CompoundCollider2D");
        if (compound != null && Entities.Length == 1)
        {
            var list = session.FieldIndex(compound.Type, "shapes");
            var (_, count) = session.RecordInfo(Entities[0], compound.Type, list);
            // Declare the complete graph once per binding, rather than changing active chains
            // when a collapsed record is opened. Record editing retains its single-selection rule.
            for (var i = 0; i < count; ++i)
            {
                var record = RecordInspector(compound.Type, list, i);
                dimensions.AddRange([record.Inspector["halfSizeX"], record.Inspector["halfSizeY"]]);
            }
        }
        if (dimensions.Count > 2) InspectorFactory.Connect(dimensions);
    }
}
#endif
