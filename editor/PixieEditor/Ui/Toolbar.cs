using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The bar at the top of the window: the viewport's mode, Game or Select, and time: slower, pause or
// resume, faster, and the speed simulated time runs at. Slowing down is what time control is mostly
// for; it speeds up to twice at most.
internal sealed class Toolbar(EngineSession session, ViewportPanel viewport)
{
    private static readonly double[] Scales = [0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0];

    public void Draw()
    {
        if (!ImGui.BeginMainMenuBar())
            return;

        foreach (var mode in Enum.GetValues<ViewportMode>())
        {
            var active = viewport.Mode == mode;
            if (active)
                ImGui.PushStyleColor(ImGuiCol.Button, ImGui.GetColorU32(ImGuiCol.ButtonActive));
            if (ImGui.Button(mode.ToString()))
                viewport.Mode = mode;
            if (active)
                ImGui.PopStyleColor();
            if (ImGui.IsItemHovered())
                ImGui.SetTooltip(mode == ViewportMode.Game
                    ? "The mouse plays the game."
                    : "Click selects, Shift + click adds, drag selects by box, Ctrl + click deletes or creates.");
        }

        ImGui.Separator();

        var stats = session.Stats;
        var index = NearestIndex(stats.TimeScale);

        ImGui.BeginDisabled(index == 0);
        if (ImGui.Button("<< Slower"))
            session.TimeScale = Scales[index - 1];
        ImGui.EndDisabled();

        if (ImGui.Button(stats.Paused != 0 ? "Resume" : "Pause", new System.Numerics.Vector2(70, 0)))
            session.Paused = stats.Paused == 0;

        ImGui.BeginDisabled(index == Scales.Length - 1);
        if (ImGui.Button("Faster >>"))
            session.TimeScale = Scales[index + 1];
        ImGui.EndDisabled();

        ImGui.TextUnformatted(stats.Paused != 0 ? $"time {stats.TimeScale:0.##}x, paused" : $"time {stats.TimeScale:0.##}x");
        ImGui.EndMainMenuBar();
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
