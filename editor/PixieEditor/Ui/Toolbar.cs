using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The bar at the top of the window: the File menu; for a project, Play or Stop; the viewport's mode,
// Game or Select; and time: slower, pause or resume, faster, and the speed simulated time runs at.
// Slowing down is what time control is mostly for; it speeds up to twice at most. While a project is
// edited, time stands still and the mouse selects, so those controls wait for Play.
internal sealed class Toolbar(EngineSession session, ViewportPanel viewport, Authoring authoring, Selection selection)
{
    private static readonly double[] Scales = [0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0];
    private static readonly Vector4 PlayColor = new(0.2f, 0.55f, 0.3f, 1.0f);
    private static readonly Vector4 StopColor = new(0.7f, 0.25f, 0.2f, 1.0f);
    private static readonly Vector4 PlayingText = new(1.0f, 0.65f, 0.3f, 1.0f);
    private static readonly Vector4 ErrorText = new(1.0f, 0.45f, 0.4f, 1.0f);
    private static readonly Vector4 InfoText = new(0.6f, 0.85f, 0.6f, 1.0f);

    public Action Save { get; init; } = () => { };
    public Action AddCode { get; init; } = () => { };
    public Action BuildAndReopen { get; init; } = () => { };
    public Action OtherProject { get; init; } = () => { };
    public Action? Export { get; init; }

    public void Draw()
    {
        if (!ImGui.BeginMainMenuBar())
            return;

        var project = session.ProjectFolder != null;
        var editing = project && session.Editing;

        if (ImGui.BeginMenu("File"))
        {
            if (ImGui.MenuItem("Save", "Ctrl+S", false, project))
                Save();
            if (Export != null && ImGui.MenuItem("Export game...", "", false, editing && !session.Unsaved))
                Export();
            if (ImGui.MenuItem("New or open project..."))
                OtherProject();
            ImGui.EndMenu();
        }

        if (ImGui.BeginMenu("Edit"))
        {
            var history = authoring.History;
            if (ImGui.MenuItem(history.UndoName is { } undo ? $"Undo {undo}" : "Undo", "Ctrl+Z", false, history.UndoName != null && editing))
                authoring.Undo();
            if (ImGui.MenuItem(history.RedoName is { } redo ? $"Redo {redo}" : "Redo", "Ctrl+Y", false, history.RedoName != null && editing))
                authoring.Redo();
            ImGui.Separator();
            var chosen = selection.Items.ToArray();
            if (ImGui.MenuItem("Duplicate", "Ctrl+D", false, chosen.Length > 0))
                authoring.Duplicate(chosen);
            if (ImGui.MenuItem("Delete", "Delete", false, chosen.Length > 0))
                authoring.Delete(chosen);
            ImGui.EndMenu();
        }

        if (project && ImGui.BeginMenu("Code"))
        {
            var hasCode = Native.ProjectBuild.HasCode(session.ProjectFolder!);
            if (ImGui.MenuItem("Add code", "", false, !hasCode))
                AddCode();
            if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled))
                ImGui.SetTooltip(hasCode
                    ? "The project has code: CMakeLists.txt and src/."
                    : "Writes CMakeLists.txt and src/ with a Project and an example Behaviour, then builds and reopens.");
            if (ImGui.MenuItem("Build and reopen", "", false, hasCode))
                BuildAndReopen();
            if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled))
                ImGui.SetTooltip("Builds the project's code and opens it again with it; the editor restarts to load it.");
            ImGui.EndMenu();
        }

        ImGui.Separator();
        if (project)
        {
            DrawPlayStop(editing);
            ImGui.Separator();
        }

        foreach (var mode in Enum.GetValues<ViewportMode>())
        {
            var active = viewport.Mode == mode;
            ImGui.BeginDisabled(editing && mode == ViewportMode.Game);
            if (active)
                ImGui.PushStyleColor(ImGuiCol.Button, ImGui.GetColorU32(ImGuiCol.ButtonActive));
            if (ImGui.Button(mode.ToString()))
                viewport.Mode = mode;
            if (active)
                ImGui.PopStyleColor();
            ImGui.EndDisabled();
            if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled))
                ImGui.SetTooltip(mode == ViewportMode.Select
                    ? "Click selects, Shift + click adds, drag selects by box, right click opens a menu to delete or create."
                    : editing ? "The mouse plays the game in Play." : "The mouse plays the game.");
        }

        var colliderTools = viewport.ColliderTools;
        ImGui.BeginDisabled(!editing);
        if (ImGui.Checkbox("Collider handles", ref colliderTools)) viewport.ColliderTools = colliderTools;
        if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled))
            ImGui.SetTooltip("Edit one collider: square resizes/radius, circle rotates, diamond changes capsule height. Drawables stay unchanged. Escape cancels.");
        if (colliderTools && selection.Items.Count == 1 && session.TypeIndex("CompoundCollider2D") is var compound && compound >= 0
            && session.Has(selection.Items[0], compound))
        {
            var (_, count) = session.RecordInfo(selection.Items[0], compound, session.FieldIndex(compound, "shapes"));
            if (count > 0)
            {
                viewport.ColliderShape = Math.Clamp(viewport.ColliderShape, 0, count - 1);
                ImGui.SetNextItemWidth(85);
                if (ImGui.BeginCombo("##collider-shape", $"Shape {viewport.ColliderShape + 1}"))
                {
                    for (var i = 0; i < count; i++)
                        if (ImGui.Selectable($"Shape {i + 1}", i == viewport.ColliderShape)) viewport.ColliderShape = i;
                    ImGui.EndCombo();
                }
            }
        }
        ImGui.EndDisabled();

        // Snapping, for moves in Select mode.
        var snap = viewport.Snap;
        if (ImGui.Checkbox("Snap", ref snap))
            viewport.Snap = snap;
        if (ImGui.IsItemHovered())
            ImGui.SetTooltip("Moves in Select mode put the dragged entity on a grid of this many world units.");
        ImGui.SetNextItemWidth(48);
        if (ImGui.BeginCombo("##grid", viewport.Grid.ToString(System.Globalization.CultureInfo.InvariantCulture)))
        {
            foreach (var grid in ViewportPanel.Grids)
            {
                if (ImGui.Selectable(grid.ToString(System.Globalization.CultureInfo.InvariantCulture), grid == viewport.Grid))
                    viewport.Grid = grid;
            }

            ImGui.EndCombo();
        }

        ImGui.Separator();

        var stats = session.Stats;
        var index = NearestIndex(stats.TimeScale);

        ImGui.BeginDisabled(editing);
        ImGui.BeginDisabled(index == 0);
        if (ImGui.Button("<< Slower"))
            session.TimeScale = Scales[index - 1];
        ImGui.EndDisabled();

        if (ImGui.Button(stats.Paused != 0 ? "Resume" : "Pause", new Vector2(70, 0)))
            session.Paused = stats.Paused == 0;

        ImGui.BeginDisabled(index == Scales.Length - 1);
        if (ImGui.Button("Faster >>"))
            session.TimeScale = Scales[index + 1];
        ImGui.EndDisabled();

        ImGui.TextUnformatted(stats.Paused != 0 ? $"time {stats.TimeScale:0.##}x, paused" : $"time {stats.TimeScale:0.##}x");
        ImGui.EndDisabled();

        if (Status.Visible(Hexa.NET.GLFW.GLFW.GetTime()))
        {
            ImGui.Separator();
            ImGui.TextColored(Status.IsError ? ErrorText : InfoText, Status.Text);
        }

        ImGui.EndMainMenuBar();
    }

    private void DrawPlayStop(bool editing)
    {
        ImGui.PushStyleColor(ImGuiCol.Button, editing ? PlayColor : StopColor);
        var pressed = ImGui.Button(editing ? "  Play  " : "  Stop  ");
        ImGui.PopStyleColor();
        if (ImGui.IsItemHovered())
            ImGui.SetTooltip(editing
                ? "Runs the scene as it is now, built again from its entities."
                : "Goes back to the scene as it was at Play; what happened while playing is not kept.");
        if (!editing)
            ImGui.TextColored(PlayingText, "PLAYING - changes are not kept");
        if (!pressed)
            return;

        try
        {
            if (editing)
            {
                selection.PreserveThrough(session, session.Play);
                viewport.Mode = ViewportMode.Game;
            }
            else
            {
                selection.PreserveThrough(session, session.Stop);
                viewport.Mode = ViewportMode.Select;
            }
        }
        catch (EngineException e)
        {
            Status.Show(e.Message, error: true);
        }
    }

    private static int NearestIndex(double scale)
    {
        var best = 0;
        for (var i = 1; i < Scales.Length; i++)
        {
            if (Math.Abs(Scales[i] - scale) < Math.Abs(Scales[best] - scale))
                best = i;
        }

        return best;
    }
}
