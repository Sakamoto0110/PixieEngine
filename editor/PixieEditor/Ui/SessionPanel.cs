using System.Diagnostics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The session's controls and numbers: a single step while paused, reset, the World's time and size,
// the cost of a step, and the other games built next to this one, each opened in a new editor.
internal sealed class SessionPanel(EngineSession session, string game)
{
    // Set when another game was opened, so the editor closes.
    public bool CloseRequested { get; private set; }

    public void Draw()
    {
        if (ImGui.Begin("Session"))
        {
            var stats = session.Stats;
            ImGui.TextUnformatted($"Game: {session.ProjectName}");

            // Pause and the speed of time are on the toolbar above.
            ImGui.BeginDisabled(stats.Paused == 0);
            if (ImGui.Button("Step"))
                session.Step();
            ImGui.EndDisabled();

            ImGui.SameLine();
            if (ImGui.Button("Reset"))
                session.Reset();

            ImGui.Separator();
            ImGui.TextUnformatted($"Steps: {stats.Steps}");
            ImGui.TextUnformatted($"Time: {stats.Elapsed:F2} s");
            ImGui.TextUnformatted($"Entities: {stats.Entities}");
            ImGui.TextUnformatted($"Last step: {stats.LastStepMilliseconds:F3} ms");
            ImGui.TextUnformatted($"Editor: {ImGui.GetIO().Framerate:F0} frames/s");

            ImGui.Separator();
            ImGui.TextUnformatted("Open another game:");
            foreach (var other in GameLibrary.Siblings().Where(g => g != game))
            {
                ImGui.SameLine();
                if (ImGui.Button(other))
                {
                    Process.Start(Environment.ProcessPath!, ["--game", other]);
                    CloseRequested = true;
                }
            }
        }

        ImGui.End();
    }
}
