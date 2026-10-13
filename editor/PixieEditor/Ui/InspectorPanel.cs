using System.Globalization;
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieInspector;
using PixieInspector.ImGui;
using PixieLib;

namespace PixieEditor.Ui;

// The selected entity's components: one PixieInspector per component, built from the engine's field
// descriptors and drawn by its Dear ImGui view under a header with the component's name. The
// inspectors are made again when the selection or the entity's set of components changes.
internal sealed class InspectorPanel(EngineSession session) : IDisposable
{
    private sealed record Section(int Type, string Name, Inspector Inspector, ImGuiInspectorView View);

    private readonly List<Section> Sections = [];
    private uint? Entity;
    private int[] Types = [];

    public void Draw(uint? selected)
    {
        if (selected is { } candidate && !session.IsAlive(candidate))
            selected = null;

        var types = selected is { } id ? Enumerable.Range(0, session.TypeCount).Where(t => session.Has(id, t)).ToArray() : [];
        if (selected != Entity || !types.SequenceEqual(Types))
            Rebuild(selected, types);

        if (ImGui.Begin("Inspector"))
        {
            if (Entity is not { } entity)
            {
                ImGui.TextDisabled("Choose an entity.");
            }
            else
            {
                var name = session.EntityName(entity);
                ImGui.TextUnformatted(name.Length > 0 ? $"{name} (#{entity & 0xFFFFF})" : $"#{entity & 0xFFFFF}");
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

    // Writes field `property` of the selected entity's component `type` through its inspector, as the
    // view does when a value is edited (a point's or vector's axis is the field's name with X or Y).
    // Returns false when there is no such component or field.
    public bool Set(string type, string property, string text)
    {
        var section = Sections.FirstOrDefault(s => s.Name == type);
        if (section == null)
            return false;

        var node = section.Inspector[property];
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

    private void Rebuild(uint? entity, int[] types)
    {
        Release();
        Entity = entity;
        Types = types;
        if (entity is not { } id)
            return;

        foreach (var type in types)
        {
            var inspector = InspectorFactory.Build(session, type, [new ComponentHandle(session, id, type)]);
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
