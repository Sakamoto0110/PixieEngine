using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The World's entities, oldest first, by name or number. A click chooses one, Ctrl + click adds or
// takes out one, and Shift + click chooses the range from the last one clicked. Only the rows in view
// ask the engine for a name, so a World of thousands stays cheap to list. Create entity adds one to
// the scene at the origin, with a transform and a circle so it shows, and chooses it.
internal sealed class EntitiesPanel(EngineSession session, Selection selection, Authoring authoring)
{
    // EnTT's 32-bit entity: the low 20 bits are the index, the rest the version.
    private const uint IndexMask = 0xFFFFF;

    public void Draw()
    {
        if (ImGui.Begin("Entities"))
        {
            if (ImGui.Button("Create entity"))
                authoring.CreateEntity();
            var entities = session.Entities();
            ImGui.SameLine();
            ImGui.TextUnformatted(selection.Items.Count > 1
                ? $"{entities.Length} entities, {selection.Items.Count} selected"
                : $"{entities.Length} entities");
            if (session.TypeIndex("Collider2D") >= 0)
            {
                if (ImGui.SmallButton("Collider")) authoring.CreateMovementEntity(false);
                ImGui.SameLine();
                if (ImGui.SmallButton("Character")) authoring.CreateMovementEntity(true);
                ImGui.SameLine();
                if (ImGui.SmallButton("Physics...")) ImGui.OpenPopup("physics-presets");
                if (ImGui.BeginPopup("physics-presets"))
                {
                    foreach (var (kind, label) in Authoring.PhysicsPresets)
                        if (ImGui.MenuItem(label)) authoring.CreatePhysicsEntity(kind);
                    ImGui.EndPopup();
                }
            }
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
                    if (ImGui.Selectable(label, selection.Contains(id)))
                        Click(entities, id);
                }

                ImGui.Dummy(new System.Numerics.Vector2(1, (entities.Length - last) * line));
            }

            ImGui.EndChild();
        }

        ImGui.End();
    }

    private void Click(uint[] entities, uint id)
    {
        var io = ImGui.GetIO();
        if (io.KeyCtrl)
        {
            selection.Toggle(id);
        }
        else if (io.KeyShift && selection.Anchor is { } anchor && Array.IndexOf(entities, anchor) is var from and >= 0)
        {
            var to = Array.IndexOf(entities, id);
            // The range keeps its start, so a second Shift + click moves only its end.
            selection.Set(entities[Math.Min(from, to)..(Math.Max(from, to) + 1)], anchor);
        }
        else
        {
            selection.Set(id);
        }
    }

    // The first entity with this name, for choosing one from the command line.
    public uint? Find(string name) => session.Entities().Cast<uint?>().FirstOrDefault(id => session.EntityName(id!.Value) == name);
}
