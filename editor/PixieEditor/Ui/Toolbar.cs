using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The bar at the top of the window: the File menu; for a project, Play or Stop; the viewport's mode,
// Game or Select; and time: slower, pause or resume, faster, and the speed simulated time runs at.
// Slowing down is what time control is mostly for; it speeds up to twice at most. While a project is
// edited, time stands still and the mouse selects, so those controls wait for Play.
internal sealed class Toolbar(EngineSession session, ViewportPanel viewport)
{
    private static readonly double[] Scales = [0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0];
    private static readonly Vector4 PlayColor = new(0.2f, 0.55f, 0.3f, 1.0f);
    private static readonly Vector4 StopColor = new(0.7f, 0.25f, 0.2f, 1.0f);
    private static readonly Vector4 PlayingText = new(1.0f, 0.65f, 0.3f, 1.0f);
    private static readonly Vector4 ErrorText = new(1.0f, 0.45f, 0.4f, 1.0f);
    private static readonly Vector4 InfoText = new(0.6f, 0.85f, 0.6f, 1.0f);

    public Action Save { get; init; } = () => { };
    public Action OtherProject { get; init; } = () => { };

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
            if (ImGui.MenuItem("New or open project..."))
                OtherProject();
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
                    ? "Click selects, Shift + click adds, drag selects by box, Ctrl + click deletes or creates."
                    : editing ? "The mouse plays the game in Play." : "The mouse plays the game.");
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
                session.Play();
                viewport.Mode = ViewportMode.Game;
            }
            else
            {
                session.Stop();
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
