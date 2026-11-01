using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// Handles for the one selected Light2D while editing: drag the square on its range ring to change the
// range, the circle along its direction to turn it (spot and directional), and the diamond on the
// cone's edge to open or close the cone (spot). Each drag writes the fields through the recorder, as
// the Inspector does, so it is one step of the history; Escape puts the values back.
internal sealed unsafe partial class ViewportPanel
{
    public Editing.EditRecorder? Recorder { get; set; }

    private enum LightHandle { Range, Direction, Cone }

    private sealed record LightGesture(uint Entity, LightHandle Handle, PixieValue Range, PixieValue Rotation, PixieValue Cone);

    private LightGesture? LightDrag;

    private sealed record LightTarget(uint Entity, int LightType, int Transform, int Kind, Vector2 Position, double Rotation,
                                      double Range, double Cone);

    private LightTarget? SelectedLight()
    {
        if (!session.Editing || Mode != ViewportMode.Select || selection.Items.Count != 1) return null;
        var light = session.TypeIndex("Light2D");
        var transform = session.TypeIndex("Transform2D");
        var entity = selection.Items[0];
        if (light < 0 || transform < 0 || !session.Has(entity, light) || !session.Has(entity, transform)) return null;
        var position = session.Get(entity, transform, session.FieldIndex(transform, "position"));
        return new LightTarget(entity, light, transform, (int)session.Get(entity, light, session.FieldIndex(light, "type")).X,
            new Vector2((float)position.X, (float)position.Y), session.Get(entity, transform, session.FieldIndex(transform, "rotation")).X,
            session.Get(entity, light, session.FieldIndex(light, "range")).X, session.Get(entity, light, session.FieldIndex(light, "coneAngle")).X);
    }

    // World units that span `pixels` on screen.
    private double WorldPerPixels(float pixels)
    {
        var a = session.ViewToWorld(0, 0);
        var b = session.ViewToWorld(pixels * Scale.X, 0);
        return Math.Abs(b.X - a.X);
    }

    private Dictionary<LightHandle, Vector2> LightHandles(LightTarget light)
    {
        const int directional = 2, spot = 1;
        var handles = new Dictionary<LightHandle, Vector2>();
        var arm = light.Kind == directional ? WorldPerPixels(60) : light.Range * 0.6;
        Vector2 At(double angle, double length) =>
            HandleScreen(light.Position.X + Math.Cos(angle) * length, light.Position.Y + Math.Sin(angle) * length);
        if (light.Kind != directional) handles[LightHandle.Range] = At(light.Rotation, light.Range);
        if (light.Kind != 0) handles[LightHandle.Direction] = At(light.Rotation, arm);
        if (light.Kind == spot) handles[LightHandle.Cone] = At(light.Rotation + light.Cone / 2, arm);
        return handles;
    }

    private bool HandleLightInput()
    {
        if (LightDrag is { } gesture)
        {
            if (!session.Editing || !Focused || !selection.Contains(gesture.Entity) || SelectedLight() is not { } dragged)
                FinishLightDrag(cancel: false);
            else if (ImGui.IsKeyPressed(ImGuiKey.Escape))
                FinishLightDrag(cancel: true);
            else if (!ImGui.IsMouseDown(ImGuiMouseButton.Left))
                FinishLightDrag(cancel: false);
            else
            {
                var view = MouseInView();
                var at = session.ViewToWorld(view.X, view.Y);
                UpdateLightDrag(dragged, gesture.Handle, at.X, at.Y, ImGui.GetIO().KeyShift);
            }
            return true;
        }
        if (!Hovered || SelectedLight() is not { } light) return false;
        var mouse = ImGui.GetMousePos();
        LightHandle? nearest = null;
        var best = 8f;
        foreach (var (handle, at) in LightHandles(light))
        {
            var distance = Vector2.Distance(mouse, at);
            if (distance <= best) { best = distance; nearest = handle; }
        }
        if (nearest is not { } chosen) return false;
        ImGui.SetTooltip(chosen switch
        {
            LightHandle.Range => "Drag to change the light's range.",
            LightHandle.Direction => "Drag to turn the light. Hold Shift for 15-degree steps.",
            _ => "Drag to open or close the spot's cone."
        });
        if (ImGui.IsMouseClicked(ImGuiMouseButton.Left))
        {
            LightDrag = new LightGesture(light.Entity, chosen,
                session.Get(light.Entity, light.LightType, session.FieldIndex(light.LightType, "range")),
                session.Get(light.Entity, light.Transform, session.FieldIndex(light.Transform, "rotation")),
                session.Get(light.Entity, light.LightType, session.FieldIndex(light.LightType, "coneAngle")));
            PressedAt = null;
        }
        return ImGui.IsMouseDown(ImGuiMouseButton.Left);
    }

    private void UpdateLightDrag(LightTarget light, LightHandle handle, double x, double y, bool snap)
    {
        var dx = x - light.Position.X;
        var dy = y - light.Position.Y;
        var angle = Math.Atan2(dy, dx);
        switch (handle)
        {
            case LightHandle.Range:
                Write(light, light.LightType, "range", Math.Max(1.0, Math.Sqrt(dx * dx + dy * dy)));
                break;
            case LightHandle.Direction:
                if (snap) angle = Math.Round(angle / (Math.PI / 12)) * (Math.PI / 12);
                Write(light, light.Transform, "rotation", angle);
                break;
            case LightHandle.Cone:
                var half = Math.Abs(Math.IEEERemainder(angle - light.Rotation, 2 * Math.PI));
                Write(light, light.LightType, "coneAngle", Math.Clamp(2 * half, 0.0, 2 * Math.PI));
                break;
        }
    }

    // The scripted form of one whole drag of `handle` ("range", "direction" or "cone") on the selected
    // light to the world point (x, y), cancelled with Escape when `cancel`: the same writes and history.
    public bool DragLight(string handle, double x, double y, bool cancel)
    {
        LightHandle? chosen = handle switch
        {
            "range" => LightHandle.Range, "direction" => LightHandle.Direction, "cone" => LightHandle.Cone, _ => null
        };
        if (chosen is not { } kind || SelectedLight() is not { } light || !LightHandles(light).ContainsKey(kind)) return false;
        LightDrag = new LightGesture(light.Entity, kind,
            session.Get(light.Entity, light.LightType, session.FieldIndex(light.LightType, "range")),
            session.Get(light.Entity, light.Transform, session.FieldIndex(light.Transform, "rotation")),
            session.Get(light.Entity, light.LightType, session.FieldIndex(light.LightType, "coneAngle")));
        UpdateLightDrag(light, kind, x, y, snap: false);
        FinishLightDrag(cancel);
        return true;
    }

    private void Write(LightTarget light, int type, string field, double value) =>
        new ComponentHandle(session, light.Entity, type, Recorder).Set(session.FieldIndex(type, field), new PixieValue { X = value });

    private void FinishLightDrag(bool cancel)
    {
        if (LightDrag is not { } gesture) return;
        LightDrag = null;
        if (cancel && SelectedLight() is { } light && light.Entity == gesture.Entity)
        {
            Write(light, light.LightType, "range", gesture.Range.X);
            Write(light, light.Transform, "rotation", gesture.Rotation.X);
            Write(light, light.LightType, "coneAngle", gesture.Cone.X);
        }
        Recorder?.Close();
        PressedAt = null;
    }

    private void DrawLightHandles()
    {
        if (SelectedLight() is not { } light) return;
        var draw = ImGui.GetWindowDrawList();
        var center = HandleScreen(light.Position.X, light.Position.Y);
        draw.AddCircle(center, 3, OutlineColor);
        foreach (var (handle, at) in LightHandles(light))
        {
            switch (handle)
            {
                case LightHandle.Range:
                    draw.AddCircle(center, Vector2.Distance(center, at), OutlineColor, 64);
                    draw.AddRectFilled(at - new Vector2(5), at + new Vector2(5), OutlineColor);
                    break;
                case LightHandle.Direction:
                    draw.AddLine(center, at, OutlineColor);
                    draw.AddCircle(at, 6, OutlineColor, 0, 2);
                    break;
                case LightHandle.Cone:
                    draw.AddLine(center, at, OutlineColor);
                    var mirrored = HandleScreen(light.Position.X + Math.Cos(light.Rotation - light.Cone / 2) * light.Range * 0.6,
                        light.Position.Y + Math.Sin(light.Rotation - light.Cone / 2) * light.Range * 0.6);
                    draw.AddLine(center, mirrored, OutlineColor);
                    draw.AddQuadFilled(at + new Vector2(0, -6), at + new Vector2(6, 0), at + new Vector2(0, 6), at + new Vector2(-6, 0), OutlineColor);
                    break;
            }
        }
    }
}
