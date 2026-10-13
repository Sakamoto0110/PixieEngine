using System.Globalization;
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieInspector;
using PixieInspector.ImGui;
using PixieLib;

namespace PixieEditor.Ui;

// The selected entities' components: one PixieInspector per component type that every selected
// entity has, built from the engine's field descriptors, bound to the first entity and with AddBind to
// the rest, and drawn by its Dear ImGui view under a header with the component's name. An edit
// changes every selected entity; values that differ show as mixed. The inspectors are made again when
// the selection or the set of shared components changes.
internal sealed class InspectorPanel(EngineSession session) : IDisposable
{
    // Set by a scenario's button; the editor rebuilds the scene at the start of the next frame, not
    // while an inspector is drawing.
    public bool RebuildRequested { get; set; }

    private sealed record Section(int Type, string Name, Inspector Inspector, ImGuiInspectorView View);

    private readonly List<Section> Sections = [];
    private uint[] Entities = [];
    private int[] Types = [];

    public void Draw(IReadOnlyList<uint> selected)
    {
        var types = selected.Count == 0
            ? []
            : Enumerable.Range(0, session.TypeCount).Where(t => selected.All(id => session.Has(id, t))).ToArray();
        if (!selected.SequenceEqual(Entities) || !types.SequenceEqual(Types))
            Rebuild([.. selected], types);

        if (ImGui.Begin("Inspector"))
        {
            if (Entities.Length == 0)
            {
                ImGui.TextDisabled("Choose an entity.");
            }
            else
            {
                ImGui.TextUnformatted(Entities.Length == 1
                    ? Describe(Entities[0])
                    : $"{Entities.Length} entities: {string.Join(", ", Entities.Take(4).Select(Describe))}{(Entities.Length > 4 ? ", ..." : "")}");
                if (Entities.Length > 1 && Types.Length == 0)
                    ImGui.TextDisabled("They share no component.");
                ImGui.Separator();
                foreach (var section in Sections)
                {
                    ImGui.PushID(section.Type);
                    if (ImGui.CollapsingHeader(section.Name, ImGuiTreeNodeFlags.DefaultOpen))
                        section.View.Draw();
                    ImGui.PopID();
                }
            }
        }

        ImGui.End();
    }

    private string Describe(uint entity)
    {
        var name = session.EntityName(entity);
        return name.Length > 0 ? $"{name} (#{entity & 0xFFFFF})" : $"#{entity & 0xFFFFF}";
    }

    // Writes field `property` of the selected entities' component `type` through its inspector, as the
    // view does when a value is edited (a point's or vector's axis is the field's name with X or Y).
    // Returns false when there is no such component or field.
    public bool Set(string type, string property, string text)
    {
        var section = Sections.FirstOrDefault(s => s.Name == type);
        if (section == null)
            return false;

        var node = section.Inspector[property];
        if (node is ButtonNode button)
        {
            button.Press();
            return true;
        }

        object value = node.ValueType switch
        {
            var t when t == typeof(bool) => bool.Parse(text),
            var t when t == typeof(int) => int.Parse(text, CultureInfo.InvariantCulture),
            var t when t == typeof(double) => double.Parse(text, CultureInfo.InvariantCulture),
            var t when t == typeof(PxColorRgba) => ParseColor(text),
            _ => text,
        };
        node.SetValue(value);
        return true;
    }

    private static PxColorRgba ParseColor(string text)
    {
        var parts = text.Split(',').Select(p => byte.Parse(p.Trim(), CultureInfo.InvariantCulture)).ToArray();
        return new PxColorRgba(parts[0], parts[1], parts[2], parts.Length > 3 ? parts[3] : (byte)255);
    }

    private void Rebuild(uint[] entities, int[] types)
    {
        Release();
        Entities = entities;
        Types = types;
        foreach (var type in types)
        {
            var handles = entities.Select(id => new ComponentHandle(session, id, type)).ToArray();
            var inspector = InspectorFactory.Build(session, type, handles, () => RebuildRequested = true);
            Sections.Add(new Section(type, session.TypeName(type), inspector, inspector.CreateImGuiView()));
        }
    }

    private void Release()
    {
        foreach (var section in Sections)
        {
            section.View.Dispose();
            section.Inspector.Dispose();
        }

        Sections.Clear();
    }

    public void Dispose() => Release();
}
