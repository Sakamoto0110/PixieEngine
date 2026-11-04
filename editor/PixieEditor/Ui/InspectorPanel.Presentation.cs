using Hexa.NET.ImGui;
using System.Text.Json.Nodes;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private void DrawPresentation(ulong scope)
    {
        if (authoring.Scenes is not { } scenes) return;
        var snapshot = session.PresentationSnapshot();
        var canonical = snapshot["canonical"]!.GetValue<bool>();
        if (!canonical && scope == 0)
        {
            ImGui.TextWrapped("Lighting uses the legacy component adapter. Migrating makes Environment, primary view and Renderer separate authorities; Undo restores the previous representation.");
            ImGui.BeginDisabled(scenes.UnavailableReason != null || scenes.NeedsConversion || scenes.HasPending);
            if (ImGui.Button("Migrate presentation settings")) scenes.Queue("migrate-presentation");
            ImGui.EndDisabled();
            return;
        }
        var model = scenes.PresentationModel(scope);
        var changed = false;
        ImGui.BeginDisabled(scenes.UnavailableReason != null || scenes.NeedsConversion);
        if (scope == 0)
        {
            if (ImGui.CollapsingHeader("Environment", ImGuiTreeNodeFlags.DefaultOpen))
            {
                changed |= Choice("Lighting capability", model, "lightingState", ["absent", "disabled", "configured"]);
                ImGui.TextWrapped("Configuring lighting is explicit. Disabled and absent capabilities are not enabled by a Scene selection.");
                changed |= Environment(model["environment"]!.AsObject());
            }
            if (ImGui.CollapsingHeader("Primary view", ImGuiTreeNodeFlags.DefaultOpen))
            {
                var view = model["view"]!.AsObject();
                changed |= Number("Exposure", view, "exposure");
                changed |= Choice("Tone mapping", view, "toneMapping", ["none", "aces"]);
                changed |= Flag("Bloom", view, "bloom");
                changed |= Number("Bloom threshold", view, "bloomThreshold");
                changed |= Number("Bloom intensity", view, "bloomIntensity");
            }
            if (ImGui.CollapsingHeader("Renderer", ImGuiTreeNodeFlags.DefaultOpen))
            {
                var renderer = model["renderer"]!.AsObject();
                changed |= Choice("Quality", renderer, "quality", ["low", "medium", "high"]);
                var cap = renderer["maxParticleLights"]!.GetValue<int>();
                SizePresentationInput("Particle light budget");
                if (ImGui.InputInt("Particle light budget", ref cap)) { renderer["maxParticleLights"] = cap; changed = true; }
                ImGui.TextWrapped("AO and normal-map compatibility values are retained read-only.");
            }
        }
        else if (ImGui.CollapsingHeader("Environment override", ImGuiTreeNodeFlags.DefaultOpen))
        {
            var environment = model["environment"]!.AsObject();
            changed |= Choice("Mode", environment, "mode", ["inherit", "override"]);
            ImGui.BeginDisabled(environment["mode"]!.GetValue<string>() == "inherit");
            changed |= Environment(environment["value"]!.AsObject());
            ImGui.EndDisabled();
            var effective = session.ViewportEnvironmentSelection;
            ImGui.TextWrapped(effective.Kind == Native.PixieEnvironmentSelectionKind.Scene && effective.SceneId == scope
                ? "This Scene is explicitly selected as the viewport Environment."
                : "Inspecting or editing this Scene does not select its Environment for the viewport.");
        }
        if (scope != 0 && ImGui.CollapsingHeader("Scene gravity", ImGuiTreeNodeFlags.DefaultOpen))
        {
            var gravity = model["gravity"]!.AsObject();
            changed |= Choice("Gravity mode", gravity, "mode", ["inherit", "multiply", "override"]);
            var vector = gravity["value"]!.AsArray();
            var x = vector[0]!.GetValue<double>(); var y = vector[1]!.GetValue<double>();
            ImGui.BeginDisabled(gravity["mode"]!.GetValue<string>() != "override");
            SizePresentationInput("Gravity X");
            var vectorChanged = ImGui.InputDouble("Gravity X", ref x);
            SizePresentationInput("Gravity Y");
            vectorChanged |= ImGui.InputDouble("Gravity Y", ref y);
            if (vectorChanged)
            {
                if (double.IsFinite(x) && double.IsFinite(y)) { gravity["value"] = new JsonArray(x, y); changed = true; }
                else Status.Show("Gravity values must be finite.", error: true);
            }
            ImGui.EndDisabled();
            ImGui.BeginDisabled(gravity["mode"]!.GetValue<string>() != "multiply");
            changed |= Number("Gravity multiplier", gravity, "multiplier");
            ImGui.EndDisabled();
            var current = snapshot["scopes"]!.AsArray().Single(s => s!["id"]!.GetValue<string>() == scope.ToString("x16"))!;
            var effective = current["effectiveGravity"]!;
            ImGui.TextWrapped($"Authored resolution: ({effective["x"]!.GetValue<double>():G6}, {effective["y"]!.GetValue<double>():G6}) from {gravity["mode"]!.GetValue<string>()}.");
            var problem = effective["diagnostic"]!.GetValue<string>();
            if (problem.Length > 0) ImGui.TextWrapped(problem);
            ImGui.TextWrapped("Motors and particle pools use Scene gravity. Bodies use World gravity.");
        }
        changed |= CameraReference("Default camera", model, snapshot["cameras"]!.AsArray());
        ImGui.EndDisabled();
        if (changed) scenes.QueuePresentation(scope, model);
    }

    private static bool Number(string label, JsonObject model, string key)
    {
        SizePresentationInput(label);
        var value = model[key]!.GetValue<double>();
        if (!ImGui.InputDouble(label, ref value)) return false;
        if (!double.IsFinite(value)) { Status.Show($"{label} must be finite.", error: true); return false; }
        model[key] = value; return true;
    }
    private static bool Flag(string label, JsonObject model, string key)
    {
        var value = model[key]!.GetValue<bool>();
        if (!ImGui.Checkbox(label, ref value)) return false;
        model[key] = value; return true;
    }
    private static bool Choice(string label, JsonObject model, string key, string[] options)
    {
        SizePresentationInput(label);
        var current = model[key]!.GetValue<string>(); var changed = false;
        if (ImGui.BeginCombo(label, current))
        {
            foreach (var option in options)
                if (ImGui.Selectable(option, current == option) && current != option) { model[key] = option; changed = true; }
            ImGui.EndCombo();
        }
        return changed;
    }
    private static bool Color(string label, JsonObject model, string key)
    {
        SizePresentationInput(label);
        var channels = model[key]!.AsArray();
        var value = new System.Numerics.Vector4(channels[0]!.GetValue<int>() / 255f, channels[1]!.GetValue<int>() / 255f,
            channels[2]!.GetValue<int>() / 255f, channels[3]!.GetValue<int>() / 255f);
        if (!ImGui.ColorEdit4(label, ref value)) return false;
        if (!float.IsFinite(value.X) || !float.IsFinite(value.Y) || !float.IsFinite(value.Z) || !float.IsFinite(value.W))
        { Status.Show($"{label} channels must be finite.", error: true); return false; }
        model[key] = new JsonArray((int)Math.Clamp(MathF.Round(value.X * 255), 0, 255), (int)Math.Clamp(MathF.Round(value.Y * 255), 0, 255),
            (int)Math.Clamp(MathF.Round(value.Z * 255), 0, 255), (int)Math.Clamp(MathF.Round(value.W * 255), 0, 255)); return true;
    }
    private static bool Environment(JsonObject model)
    {
        var changed = Color("Ambient", model, "ambient");
        changed |= Number("Ambient intensity", model, "ambientIntensity");
        changed |= Flag("Fog", model, "fog");
        changed |= Color("Fog color", model, "fogColor");
        changed |= Number("Fog density", model, "fogDensity");
        changed |= Number("Fog top", model, "fogTop");
        changed |= Number("Fog bottom", model, "fogBottom");
        return changed;
    }
    private static bool CameraReference(string label, JsonObject model, JsonArray cameras)
    {
        SizePresentationInput(label);
        var current = model["camera"]?.GetValue<string>();
        var selected = cameras.FirstOrDefault(c => c!["id"]!.GetValue<string>() == current);
        var preview = current == null ? "Next fallback" : selected?["name"]?.GetValue<string>() ?? $"Missing {current}";
        var changed = false;
        if (ImGui.BeginCombo(label, preview))
        {
            if (ImGui.Selectable("Next fallback", current == null) && current != null) { model["camera"] = null; changed = true; }
            foreach (var camera in cameras)
            {
                var id = camera!["id"]!.GetValue<string>();
                if (ImGui.Selectable($"{camera["name"]!.GetValue<string>()}##{id}", id == current) && id != current)
                { model["camera"] = id; changed = true; }
            }
            ImGui.EndCombo();
        }
        return changed;
    }

    private static void SizePresentationInput(string label) => ImGui.SetNextItemWidth(Math.Max(1,
        ImGui.GetContentRegionAvail().X - ImGui.CalcTextSize(label).X - ImGui.GetStyle().ItemInnerSpacing.X));
}
