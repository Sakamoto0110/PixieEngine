using Hexa.NET.ImGui;
using System.Numerics;
using PixieEditor.Native;

namespace PixieEditor.Ui;

internal static class CandidateRecoveryPanel
{
    internal static void Draw(EngineSession session)
    {
        var viewport = ImGui.GetMainViewport();
        // Cover the last normal frame; obsolete controls must not appear usable during recovery.
        ImGui.SetNextWindowPos(viewport.Pos); ImGui.SetNextWindowSize(viewport.Size);
        ImGui.SetNextWindowBgAlpha(1);
        ImGui.Begin("##recovery-background", ImGuiWindowFlags.NoDecoration | ImGuiWindowFlags.NoInputs |
            ImGuiWindowFlags.NoSavedSettings | ImGuiWindowFlags.NoDocking | ImGuiWindowFlags.NoBringToFrontOnFocus);
        ImGui.End();
        ImGui.SetNextWindowPos(viewport.Pos + viewport.Size / 2, ImGuiCond.Appearing, new Vector2(0.5f));
        ImGui.SetNextWindowSize(new Vector2(460, 0), ImGuiCond.Appearing);
        var retry = false;
        if (ImGui.Begin("World edit recovery", ImGuiWindowFlags.AlwaysAutoResize | ImGuiWindowFlags.NoDocking))
        {
            ImGui.TextWrapped("The World change was applied. Editing waits until the Inspector bindings are recovered.");
            if (session.Edits.RecoveryRequired && session.Edits.RetryRecovery != null)
                retry = ImGui.Button("Retry Inspector recovery");
        }
        ImGui.End();
        // The current ImGui frame exists, but no stale Inspector window is on the stack.
        if (retry && session.Edits.RetryRecovery?.Invoke() == false)
            Status.Show("The World change remains applied; Inspector recovery still needs attention.", error: true);
    }
}
