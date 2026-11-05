using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// Global menus and viewport-local playback/tools. Input modes and simulation state remain separate.
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

        if (Status.Visible(Hexa.NET.GLFW.GLFW.GetTime()))
        {
            ImGui.Separator();
            ImGui.TextColored(Status.IsError ? ErrorText : InfoText, Status.Text);
        }
        ImGui.EndMainMenuBar();
    }

    // Drawn inside the viewport before its image, so all pointer conversions use the image's origin.
    public void DrawViewportControls()
    {
        ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, new Vector2(10, 7));
        ImGui.PushStyleColor(ImGuiCol.ChildBg, new Vector4(0.075f, 0.085f, 0.105f, 1));
        if (ImGui.BeginChild("##viewport-controls", Vector2.Zero,
                ImGuiChildFlags.AutoResizeY | ImGuiChildFlags.AlwaysUseWindowPadding,
                ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse))
        {
            var editing = session.ProjectFolder != null && session.Editing;
            var stats = session.Stats;
            RowHasItem = false;
            if (session.ProjectFolder != null) { Place(70); DrawPlayStop(editing); }
            ImGui.BeginDisabled(editing);
            Place(70);
            if (ImGui.Button(!editing && stats.Paused != 0 ? "Resume" : "Pause", new Vector2(70, 0))) session.Paused = stats.Paused == 0;
            ImGui.BeginDisabled(stats.Paused == 0);
            Place(45); if (ImGui.Button("Step")) session.Step();
            ImGui.EndDisabled();
            var index = NearestIndex(stats.TimeScale);
            ImGui.BeginDisabled(index == 0);
            Place(25); if (ImGui.Button("-##slower")) session.TimeScale = Scales[index - 1];
            ImGui.EndDisabled();
            if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled)) ImGui.SetTooltip("Slower simulation");
            Place(65); ImGui.SetNextItemWidth(65);
            if (ImGui.BeginCombo("##simulation-speed", $"{stats.TimeScale:0.##}x"))
            {
                foreach (var scale in Scales)
                    if (ImGui.Selectable($"{scale:0.##}x", scale == stats.TimeScale)) session.TimeScale = scale;
                ImGui.EndCombo();
            }
            if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled)) ImGui.SetTooltip("Simulation speed; available during Play");
            ImGui.BeginDisabled(index == Scales.Length - 1);
            Place(25); if (ImGui.Button("+##faster")) session.TimeScale = Scales[index + 1];
            ImGui.EndDisabled();
            if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled)) ImGui.SetTooltip("Faster simulation");
            ImGui.EndDisabled();
            var state = editing ? "Editing" : stats.Paused != 0 ? "Paused" : "Playing";
            Place(ImGui.CalcTextSize(state).X);
            ImGui.TextColored(editing ? new Vector4(0.65f, 0.72f, 0.8f, 1) : PlayingText, state);
            if (!editing && ImGui.IsItemHovered()) ImGui.SetTooltip("Runtime changes are discarded when Play stops.");

            ImGui.Separator(); RowHasItem = false;
            Place(0); ImGui.TextDisabled("Input");
            foreach (var mode in Enum.GetValues<ViewportMode>().Reverse())
            {
                var label = mode == ViewportMode.Game ? "Game input" : "Select";
                var active = viewport.Mode == mode;
                Place(ImGui.CalcTextSize(label).X + ImGui.GetStyle().FramePadding.X * 2);
                ImGui.BeginDisabled(editing && mode == ViewportMode.Game);
                if (active) ImGui.PushStyleColor(ImGuiCol.Button, ImGui.GetColorU32(ImGuiCol.ButtonActive));
                if (ImGui.Button(label)) viewport.Mode = mode;
                if (active) ImGui.PopStyleColor();
                ImGui.EndDisabled();
                if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled))
                    ImGui.SetTooltip(mode == ViewportMode.Select ? "Select entities and use editor tools in the viewport."
                        : editing ? "Enter Play to send viewport input to the game." : "Send viewport input to the game.");
            }
            ImGui.BeginDisabled(!editing);
            var handles = viewport.ColliderTools;
            Place(145); if (ImGui.Checkbox("Collider handles", ref handles)) viewport.ColliderTools = handles;
            if (ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled)) ImGui.SetTooltip("Edit collider geometry; drawables stay unchanged. Escape cancels a drag.");
            if (handles && selection.Items.Count == 1 && session.TypeIndex("CompoundCollider2D") is var compound and >= 0 && session.Has(selection.Items[0], compound))
            {
                var (_, count) = session.RecordInfo(selection.Items[0], compound, session.FieldIndex(compound, "shapes"));
                if (count > 0)
                {
                    viewport.ColliderShape = Math.Clamp(viewport.ColliderShape, 0, count - 1);
                    Place(90); ImGui.SetNextItemWidth(90);
                    if (ImGui.BeginCombo("##collider-shape", $"Shape {viewport.ColliderShape + 1}"))
                    {
                        for (var i = 0; i < count; ++i) if (ImGui.Selectable($"Shape {i + 1}", i == viewport.ColliderShape)) viewport.ColliderShape = i;
                        ImGui.EndCombo();
                    }
                }
            }
            ImGui.EndDisabled();
            var snap = viewport.Snap;
            Place(55); if (ImGui.Checkbox("Snap", ref snap)) viewport.Snap = snap;
            if (ImGui.IsItemHovered()) ImGui.SetTooltip("Snap entity movement to a grid in world units.");
            Place(58); ImGui.SetNextItemWidth(58);
            if (ImGui.BeginCombo("##grid", viewport.Grid.ToString(System.Globalization.CultureInfo.InvariantCulture)))
            {
                foreach (var grid in ViewportPanel.Grids)
                    if (ImGui.Selectable(grid.ToString(System.Globalization.CultureInfo.InvariantCulture), grid == viewport.Grid)) viewport.Grid = grid;
                ImGui.EndCombo();
            }
        }
        ImGui.EndChild(); ImGui.PopStyleColor(); ImGui.PopStyleVar();
    }

    private bool RowHasItem;
    private void Place(float width)
    {
        if (RowHasItem)
        {
            ImGui.SameLine();
            if (ImGui.GetContentRegionAvail().X < width) ImGui.NewLine();
        }
        RowHasItem = true;
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
