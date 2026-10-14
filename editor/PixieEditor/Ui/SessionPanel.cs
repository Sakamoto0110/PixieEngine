using System.Diagnostics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The session's controls and numbers: the project and its folder, or the game; a single step while
// paused, reset, the World's time and size, the cost of a step; and, for a game, the other games built
// next to this one, each opened in a new editor.
internal sealed class SessionPanel(EngineSession session, string game)
{
    // Set when another game was opened, so the editor closes.
    public bool CloseRequested { get; private set; }

    public void Draw()
    {
        if (ImGui.Begin("Session"))
        {
            var stats = session.Stats;
            var project = session.ProjectFolder;
            var editing = project != null && session.Editing;
            if (project != null)
            {
                ImGui.TextUnformatted($"Project: {session.ProjectName}{(session.Unsaved ? " (not saved)" : "")}");
                ImGui.TextDisabled(project);
                var library = Path.GetFileName(GameLibrary.Loaded);
                ImGui.TextUnformatted(!ProjectBuild.HasCode(project) ? "No code of its own (Code, Add code)."
                    : library.StartsWith(GameLibrary.SceneGame, StringComparison.Ordinal) ? "Its code is not loaded: opened without it."
                    : $"Code: {library}");
                ImGui.TextUnformatted(editing ? "Editing: time stands still." : "Playing.");
            }
            else
            {
                ImGui.TextUnformatted($"Game: {session.ProjectName}");
            }

            // Pause and the speed of time are on the toolbar above.
            ImGui.BeginDisabled(stats.Paused == 0 || editing);
            if (ImGui.Button("Step"))
                session.Step();
            ImGui.EndDisabled();

            ImGui.SameLine();
            if (ImGui.Button(project != null ? "Rebuild" : "Reset"))
                session.Reset();
            if (ImGui.IsItemHovered())
                ImGui.SetTooltip(project != null
                    ? "Builds the World again from the scene's entities as they are now."
                    : "Builds the World again from the game.");

            ImGui.Separator();
            ImGui.TextUnformatted($"Steps: {stats.Steps}");
            ImGui.TextUnformatted($"Time: {stats.Elapsed:F2} s");
            ImGui.TextUnformatted($"Entities: {stats.Entities}");
            ImGui.TextUnformatted($"Last step: {stats.LastStepMilliseconds:F3} ms");
            ImGui.TextUnformatted($"Editor: {ImGui.GetIO().Framerate:F0} frames/s");

            if (project != null)
            {
                ImGui.End();
                return;
            }

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
