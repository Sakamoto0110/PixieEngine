using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieInspector.ImGui;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private bool BoundToWorld;
    private bool WorldBindingsSubscribed;
    private void BeforeWorldBindingsReplace() => recorder.Close();
    private sealed record WorldRow(int Type, string Name, uint? Owner, int Count, bool ReadOnly);
    private WorldRow[] WorldRows = [];
    private static readonly string[] EnvironmentFields =
        ["enabled", "ambient", "ambientIntensity", "fog", "fogColor", "fogDensity", "fogTop", "fogBottom"];

    private static bool WorldFieldAllowed(string type, string field) => type == "PhysicsSettings2D" ||
        (type == "LightingSettings2D" && EnvironmentFields.Contains(field));
    private static bool WorldPropertyAllowed(string type, string property) =>
        WorldFieldAllowed(type, property.TrimEnd('X', 'Y'));

    public void Draw(Selection selection)
    {
        if (session.Edits.IsBlocked && !session.Edits.BindingRecovery) return;
        if (selection.Scope is { } scope) { DrawScope(scope); return; }
        if (!selection.IsWorld) { Draw(selection.Items); return; }
        BindWorld();
        DrawWorld();
    }

    private void BindWorld()
    {
        if (!WorldBindingsSubscribed)
        {
            session.BindingsReplacing += BeforeWorldBindingsReplace;
            WorldBindingsSubscribed = true;
        }
        var entities = session.Entities();
        var rows = new[] { "PhysicsSettings2D", "LightingSettings2D" }.Select(name =>
        {
            var type = session.TypeIndex(name);
            var owners = type < 0 ? [] : entities.Where(entity => session.Has(entity, type)).ToArray();
            uint? owner = owners.Length == 1 ? owners[0] : null;
            return new WorldRow(type, name, owner, owners.Length,
                !session.Editing || owner == null || session.SceneId(owner.Value) == null);
        }).ToArray();
        if (!BoundToWorld || BoundRevision != session.BindingRevision || !rows.SequenceEqual(WorldRows))
        {
            recorder.Close(); Release();
            NameActive = false;
            BoundScope = null; ScopeNameActive = false;
            Typing.Clear(); PointTyping.Clear(); LayerTyping.Clear();
            Entities = []; Types = []; WorldRows = rows; BoundToWorld = true;
            BoundRevision = session.BindingRevision;
            foreach (var row in rows.Where(row => row.Owner != null))
            {
                var handles = new[] { new ComponentHandle(session, row.Owner!.Value, row.Type, recorder) };
                var inspector = InspectorFactory.Build(session, row.Type, handles, () => RebuildRequested = true,
                    field => WorldFieldAllowed(row.Name, session.FieldName(row.Type, field)), row.ReadOnly);
                var view = inspector.CreateImGuiView();
#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
                ConfigureInteraction(view);
#endif
                Sections.Add(new Section(row.Type, row.Name, inspector, view, []));
            }
        }
    }

    private void DrawWorld()
    {
        if (ImGui.Begin("Inspector"))
        {
            ImGui.TextUnformatted("World properties");
            ImGui.TextWrapped("World defaults. Inspecting this target does not select the viewport Environment.");
            ImGui.Separator();
            DrawPresentation(0);
            foreach (var row in WorldRows.Where(row => row.Name != "LightingSettings2D" || session.PresentationSnapshot()["canonical"]?.GetValue<bool>() != true))
            {
                ImGui.PushID(row.Name);
                var title = row.Name == "LightingSettings2D" ? "Environment" : "Physics";
                if (ImGui.CollapsingHeader(title, ImGuiTreeNodeFlags.DefaultOpen))
                {
                    if (row.Count == 0)
                        ImGui.TextWrapped(row.Name == "LightingSettings2D"
                            ? "Absent. Legacy rendering is unlit; no lighting settings are created."
                            : "Absent. Engine physics defaults apply; no settings are created.");
                    else if (row.Count > 1)
                        ImGui.TextWrapped($"{row.Count} conflicting legacy authorities. Reads remain compatible; resolve the conflict before editing.");
                    else
                    {
                        if (row.ReadOnly) ImGui.TextWrapped("Read-only: editing requires Edit mode and a persistent settings owner.");
                        if (row.Name == "LightingSettings2D")
                            ImGui.TextWrapped("World ambient light and fog. Other lighting fields remain in the existing component editor.");
                        ImGui.BeginDisabled(row.ReadOnly);
                        Sections.First(section => section.Type == row.Type).View.Draw();
                        ImGui.EndDisabled();
                    }
                }
                ImGui.PopID();
            }
        }
        ImGui.End();
    }
}
