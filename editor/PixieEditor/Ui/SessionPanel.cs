using System.Diagnostics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// Session state, reset, collapsible diagnostics and project details. Playback lives in the viewport.
// Other games built next to this one can each be opened in a new editor.
internal sealed class SessionPanel(EngineSession session, string game)
{
    private string MovementStatus = "";
    private long NextStatusRead;
#if PIXIE_DEVELOPMENT
    public bool Deterministic { get; init; }
#endif
    // Set when another game was opened, so the editor closes.
    public bool CloseRequested { get; private set; }

    internal static bool IsProjectModule(string file) => file.Contains("-module.", StringComparison.OrdinalIgnoreCase);

    private static void Wrapped(string text)
    {
        ImGui.PushTextWrapPos(0); ImGui.TextUnformatted(text); ImGui.PopTextWrapPos();
    }

    private static void Row(string label, string value)
    {
        ImGui.TableNextRow(); ImGui.TableSetColumnIndex(0); ImGui.TextDisabled(label);
        ImGui.TableSetColumnIndex(1); Wrapped(value);
    }

    public void Draw()
    {
        if (ImGui.Begin("Session"))
        {
            var stats = session.Stats;
            var project = session.ProjectFolder;
            var editing = project != null && session.Editing;
            ImGui.TextDisabled(project != null ? "PROJECT" : "GAME");
            Wrapped(session.ProjectName);
            ImGui.Separator();
            if (ImGui.BeginTable("##session-state", 2, ImGuiTableFlags.SizingStretchProp))
            {
                ImGui.TableSetupColumn("Label", ImGuiTableColumnFlags.WidthFixed, 62);
                ImGui.TableSetupColumn("Value", ImGuiTableColumnFlags.WidthStretch);
                Row("Mode", editing ? "Editing" : stats.Paused != 0 ? "Paused" : "Playing");
                if (project != null) Row("Scene", session.Unsaved ? "Unsaved changes" : "Saved");
                Row("Time", $"{stats.Elapsed:F2} s");
                ImGui.EndTable();
            }
            ImGui.Spacing();
            ImGui.TextDisabled("CODE");
            var library = Path.GetFileName(GameLibrary.Loaded);
            if (project != null && !ProjectBuild.HasCode(project))
            {
                Wrapped("Built-in scene runtime");
                if (ImGui.IsItemHovered()) ImGui.SetTooltip("Use Code > Add code to add a C++ project.");
            }
            else if (project != null && !IsProjectModule(library)) Wrapped("Project code not loaded");
            else Wrapped(library);
            if (ImGui.Button(project != null ? "Reset scene" : "Reset game")) session.Reset();
            if (ImGui.IsItemHovered()) ImGui.SetTooltip("Recreates the World from the current authored data. This does not compile C++ code.");

            if (ImGui.CollapsingHeader("Statistics"))
            {
                if (ImGui.BeginTable("##session-statistics", 2, ImGuiTableFlags.SizingStretchProp))
                {
                    ImGui.TableSetupColumn("Label", ImGuiTableColumnFlags.WidthFixed, 76);
                    ImGui.TableSetupColumn("Value", ImGuiTableColumnFlags.WidthStretch);
                    Row("Entities", stats.Entities.ToString());
                    Row("Steps", stats.Steps.ToString());
#if PIXIE_DEVELOPMENT
                    if (!Deterministic)
#endif
                    {
                        Row("Step cost", $"{stats.LastStepMilliseconds:F3} ms");
                        Row("Editor", $"{ImGui.GetIO().Framerate:F0} frames/s");
                    }
                    ImGui.EndTable();
                }
            }
            if (project != null && ImGui.CollapsingHeader("Project details"))
            {
                ImGui.TextDisabled("Folder"); Wrapped(project);
                ImGui.TextDisabled("Loaded library"); Wrapped(library);
            }
            if (ImGui.CollapsingHeader("Physics debug"))
            {
                string[] labels = ["Collider outlines", "Body origins", "Centers of mass", "Velocities", "Sensors", "Awake/asleep", "Contact normals"];
                var flags = session.PhysicsDebug;
                for (var i = 0; i < labels.Length; i++)
                {
                    var bit = 1u << i;
                    var enabled = (flags & bit) != 0;
                    if (ImGui.Checkbox(labels[i], ref enabled))
                    {
                        flags = enabled ? flags | bit : flags & ~bit;
                        session.PhysicsDebug = flags;
                    }
                }
                ImGui.TextWrapped("Velocity: 0.25 s of travel. Green: awake; gray: asleep; pink: sensors. Contact normals may be speculative.");
            }
            if (ImGui.CollapsingHeader("Lighting debug"))
            {
                string[] labels = ["Light bounds", "Occluder edges", "Light buffer", "Light overdraw"];
                var flags = session.LightingDebug;
                for (var i = 0; i < labels.Length; i++)
                {
                    var bit = 1u << i;
                    var enabled = (flags & bit) != 0;
                    if (ImGui.Checkbox(labels[i], ref enabled))
                    {
                        flags = enabled ? flags | bit : flags & ~bit;
                        session.LightingDebug = flags;
                    }
                }
                ImGui.TextWrapped("Drawn over scenes with LightingSettings2D. Yellow: light ranges, spot cones and directions; cyan: occluder edges. " +
                                  "Light buffer replaces the frame with the light alone; overdraw, with the lights shading each pixel (blue 1, green 4, red 8 or more).");
            }
            if (Environment.TickCount64 >= NextStatusRead)
            {
                MovementStatus = session.MovementStatus;
                NextStatusRead = Environment.TickCount64 + 250;
            }
            if (MovementStatus.Length > 0)
            {
                ImGui.Separator();
                Wrapped("Movement configuration: " + MovementStatus);
            }

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
