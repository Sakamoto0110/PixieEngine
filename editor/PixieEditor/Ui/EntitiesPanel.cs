using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The World's entities, oldest first, by name or number; choosing one shows it in the inspector.
// Only the rows in view ask the engine for a name, so a World of thousands stays cheap to list.
internal sealed class EntitiesPanel(EngineSession session)
{
    // EnTT's 32-bit entity: the low 20 bits are the index, the rest the version.
    private const uint IndexMask = 0xFFFFF;

    public uint? Selected { get; set; }

    public void Draw()
    {
        if (ImGui.Begin("Entities"))
        {
            var entities = session.Entities();
            ImGui.TextUnformatted($"{entities.Length} entities");
            ImGui.Separator();

            if (ImGui.BeginChild("##list"))
            {
                var line = ImGui.GetTextLineHeightWithSpacing();
                var first = Math.Clamp((int)(ImGui.GetScrollY() / line), 0, entities.Length);
                var last = Math.Min(entities.Length, first + (int)(ImGui.GetWindowHeight() / line) + 2);

                ImGui.Dummy(new System.Numerics.Vector2(1, first * line));
                for (var i = first; i < last; i++)
                {
                    var id = entities[i];
                    var name = session.EntityName(id);
                    var label = name.Length > 0 ? $"{name}  #{id & IndexMask}##{id}" : $"#{id & IndexMask}##{id}";
                    if (ImGui.Selectable(label, Selected == id))
                        Selected = id;
                }

                ImGui.Dummy(new System.Numerics.Vector2(1, (entities.Length - last) * line));
            }

            ImGui.EndChild();
        }

        ImGui.End();
    }

    // The first entity with this name, for choosing one from the command line.
    public uint? Find(string name) => session.Entities().Cast<uint?>().FirstOrDefault(id => session.EntityName(id!.Value) == name);
}
