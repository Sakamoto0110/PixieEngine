using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// What the build screen ends with: the game's library to open the project with, or the engine's
// generic one (the project opened without its code).
internal sealed record BuildOutcome(string? Library);

// The screen shown while a project's code builds, before the editor opens it: the project, CMake's
// output as it comes, and, when the build fails, why, with Try again and Open without code. A build
// that succeeds opens the project at once.
internal sealed class BuildScreen(string folder) : IDisposable
{
    private static readonly Vector4 ErrorColor = new(1.0f, 0.45f, 0.4f, 1.0f);
    private static readonly Vector4 DimColor = new(0.6f, 0.6f, 0.6f, 1.0f);
    private const float Margin = 30f;

    private readonly ProjectBuild Build = new(folder);
    private int LinesShown;

    public void Start() => Build.Start();

    public BuildOutcome? Draw()
    {
        BuildOutcome? outcome = null;
        var viewport = ImGui.GetMainViewport();
        ImGui.SetNextWindowPos(viewport.WorkPos);
        ImGui.SetNextWindowSize(viewport.WorkSize);
        const ImGuiWindowFlags flags = ImGuiWindowFlags.NoDecoration | ImGuiWindowFlags.NoMove |
                                       ImGuiWindowFlags.NoSavedSettings | ImGuiWindowFlags.NoBringToFrontOnFocus;
        if (ImGui.Begin("Build", flags))
        {
            var width = Math.Max(1f, viewport.WorkSize.X - 2f * Margin);
            ImGui.SetCursorPos(new Vector2(Margin, Margin));
            if (ImGui.BeginChild("##build", new Vector2(width, viewport.WorkSize.Y - 2f * Margin)))
            {
                var name = Path.GetFileName(folder.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
                ImGui.TextUnformatted(Build.Finished
                    ? Build.Succeeded ? $"Built {name}; opening it." : $"{name} did not build."
                    : $"Building {name}'s code ({ProjectBuild.Configuration}). The first build compiles the engine too, which takes a few minutes.");
                ImGui.TextColored(DimColor, folder);
                if (Build.Problem is { } problem)
                    ImGui.TextColored(ErrorColor, problem);

                if (Build.Finished && !Build.Succeeded)
                {
                    if (ImGui.Button("Try again", new Vector2(140, 0)))
                        Build.Start();
                    ImGui.SameLine();
                    if (ImGui.Button("Open without code", new Vector2(160, 0)))
                        outcome = new BuildOutcome(null);
                    if (ImGui.IsItemHovered())
                        ImGui.SetTooltip("Opens the scene with the engine's own components only; what the code adds is kept in the file, untouched.");
                }

                ImGui.Spacing();
                DrawOutput();
            }

            ImGui.EndChild();
        }

        ImGui.End();

        if (Build.Finished && Build.Succeeded)
            outcome = new BuildOutcome(ProjectBuild.Library(folder));
        return outcome;
    }

    // CMake's output, following the end while new lines come.
    private void DrawOutput()
    {
        if (!ImGui.BeginChild("##output", Vector2.Zero, ImGuiChildFlags.Borders, ImGuiWindowFlags.HorizontalScrollbar))
        {
            ImGui.EndChild();
            return;
        }

        var lines = Build.Lines;
        foreach (var line in lines)
        {
            var failed = line.Contains(": error", StringComparison.OrdinalIgnoreCase) ||
                         line.Contains("CMake Error", StringComparison.Ordinal);
            if (failed)
                ImGui.PushStyleColor(ImGuiCol.Text, ErrorColor);
            ImGui.TextUnformatted(line);
            if (failed)
                ImGui.PopStyleColor();
        }

        if (lines.Length != LinesShown)
        {
            ImGui.SetScrollHereY(1f);
            LinesShown = lines.Length;
        }

        ImGui.EndChild();
    }

    public void Dispose() => Build.Dispose();
}
