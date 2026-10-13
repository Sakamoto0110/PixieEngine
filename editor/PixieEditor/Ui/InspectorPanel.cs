using System.Globalization;
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieInspector;
using PixieInspector.ImGui;

namespace PixieEditor.Ui;

// The selected entity's components: for each one with a proxy, a PixieInspector bound to the proxy and
// drawn by its Dear ImGui view, under a header with the component's name. The inspectors are made
// again when the selection or the entity's set of components changes.
internal sealed class InspectorPanel(EngineSession session) : IDisposable
{
    private sealed record Section(int Type, string Name, Inspector? Inspector, ImGuiInspectorView? View);

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
                    {
                        if (section.View != null)
                            section.View.Draw();
                        else
                            ImGui.TextDisabled("No proxy for this component yet.");
                    }

                    ImGui.PopID();
                }
            }
        }

        ImGui.End();
    }

    // Writes "Property" of the selected entity's component "type" through its inspector, as the view
    // does when a value is edited. Returns false when there is no such component or property.
    public bool Set(string type, string property, string text)
    {
        var section = Sections.FirstOrDefault(s => s.Name == type && s.Inspector != null);
        if (section?.Inspector == null)
            return false;

        var node = section.Inspector[property];
        var valueType = node.ValueType ?? typeof(double);
        object value = valueType.IsEnum
            ? Enum.Parse(valueType, text, ignoreCase: true)
            : Convert.ChangeType(text, valueType, CultureInfo.InvariantCulture);
        node.SetValue(value);
        return true;
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
            var name = session.TypeName(type);
            if (!ProxyCatalog.Has(name))
            {
                Sections.Add(new Section(type, name, null, null));
                continue;
            }

            var inspector = ProxyCatalog.Inspect(name, session, id);
            Sections.Add(new Section(type, name, inspector, inspector.CreateImGuiView()));
        }
    }

    private void Release()
    {
        foreach (var section in Sections)
        {
            section.View?.Dispose();
            section.Inspector?.Dispose();
        }

        Sections.Clear();
    }

    public void Dispose() => Release();
}
