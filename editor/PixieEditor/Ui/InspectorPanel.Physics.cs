using Hexa.NET.ImGui;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private void DrawBodyMass()
    {
        ImGui.Separator();
        ImGui.TextWrapped("Effective mass properties (read-only)");
        if (Entities.Length != 1)
        {
            ImGui.TextDisabled("Select one body to inspect its mass properties.");
            return;
        }

        if (session.BodyMass2D(Entities[0]) is not { } mass)
        {
            ImGui.TextWrapped("Mass properties unavailable. Enable valid body geometry and check movement diagnostics in Session.");
            return;
        }

        ImGui.TextUnformatted(FormattableString.Invariant($"Mass: {mass.Mass:G6} kg"));
        ImGui.TextUnformatted(FormattableString.Invariant($"Solid area: {mass.Area:G6} m^2"));
        if (ImGui.IsItemHovered())
            ImGui.SetTooltip("Sum of enabled solid shapes, including zero-density shapes. Sensors are excluded; overlapping shapes add. Uses the world's units per meter.");
        ImGui.TextWrapped(FormattableString.Invariant($"Local center: ({mass.LocalCenterX:G6}, {mass.LocalCenterY:G6}) world units"));
        ImGui.TextWrapped(FormattableString.Invariant($"Inertia: {mass.Inertia:G6} kg * world units^2"));
        if (ImGui.IsItemHovered())
            ImGui.SetTooltip("Effective backend inertia about the center of mass. Fixed rotation suppresses angular response.");
        ImGui.TextWrapped("Static and kinematic bodies have zero effective mass and inertia.");
    }
}
