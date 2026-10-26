using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

internal sealed unsafe partial class ViewportPanel
{
    public bool ColliderTools { get; set; }
    public int ColliderShape { get; set; }
    private ColliderGesture? ColliderDrag;

    // Close the gesture before a release-frame toolbar action or a Ctrl shortcut can
    // replace its entity/save the scene. Escape still restores the pre-drag snapshot.
    public void CloseColliderGestureBeforeCommands()
    {
        if (ColliderDrag != null && (!ImGui.IsMouseDown(ImGuiMouseButton.Left) || ImGui.GetIO().KeyCtrl
            || ImGui.IsKeyPressed(ImGuiKey.Delete)))
            FinishColliderDrag(cancel: ImGui.IsKeyPressed(ImGuiKey.Escape));
    }

    private (uint Entity, int Shape, PixieColliderHandles2D Handles)? ColliderTarget()
    {
        if (!ColliderTools || !session.Editing || Mode != ViewportMode.Select || selection.Items.Count != 1)
            return null;
        var entity = selection.Items[0];
        var compound = session.TypeIndex("CompoundCollider2D");
        var shape = compound >= 0 && session.Has(entity, compound) ? ColliderShape : -1;
        return session.ColliderHandles2D(entity, shape) is { } handles ? (entity, shape, handles) : null;
    }

    private Vector2 HandleScreen(double x, double y)
    {
        var p = session.WorldToView(x, y);
        return ImageOrigin + new Vector2((float)p.X, (float)p.Y) / Scale;
    }

    private void FinishColliderDrag(bool cancel = false)
    {
        if (ColliderDrag is not { } gesture) return;
        ColliderDrag = null;
        authoring.EndColliderDrag(gesture, cancel);
        PressedAt = null;
    }

    private bool HandleColliderInput()
    {
        if (ColliderDrag is { } gesture)
        {
            if (!ColliderTools || !session.Editing || !Focused || !selection.Contains(gesture.Entity))
                FinishColliderDrag();
            else if (ImGui.IsKeyPressed(ImGuiKey.Escape))
                FinishColliderDrag(cancel: true);
            else if (!ImGui.IsMouseDown(ImGuiMouseButton.Left))
                FinishColliderDrag();
            else
            {
                var view = MouseInView();
                var at = session.ViewToWorld(view.X, view.Y);
                authoring.UpdateColliderDrag(gesture, at.X, at.Y, ImGui.GetIO().KeyShift);
            }
            return true;
        }
        if (!Hovered || ColliderTarget() is not { } target) return false;
        var mouse = ImGui.GetMousePos();
        var h = target.Handles;
        var nearest = Vector2.Distance(mouse, HandleScreen(h.ResizeX, h.ResizeY));
        var kind = 0;
        if (h.CanRotate != 0 && Vector2.Distance(mouse, HandleScreen(h.RotateX, h.RotateY)) is var rotate && rotate < nearest)
        { nearest = rotate; kind = 1; }
        if (h.CanHeight != 0 && Vector2.Distance(mouse, HandleScreen(h.HeightX, h.HeightY)) is var height && height < nearest)
        { nearest = height; kind = 2; }
        if (nearest > 8) return false;
        ImGui.SetTooltip(kind == 2 ? "Change full capsule height; radius and center stay fixed. Minimum height is twice the radius."
            : kind == 1 ? "Rotate collider about its local origin. Hold Shift for 15-degree steps."
            : h.CanHeight != 0 ? "Change capsule radius; the straight segment and center stay fixed."
            : "Resize collider about its local origin. Polygon resize is uniform.");
        if (ImGui.IsMouseClicked(ImGuiMouseButton.Left))
        {
            ColliderDrag = authoring.BeginColliderDrag(target.Entity, target.Shape, kind);
            PressedAt = null;
        }
        return ImGui.IsMouseDown(ImGuiMouseButton.Left);
    }

    private void DrawColliderHandles()
    {
        if (ColliderTarget() is not { } target) return;
        var h = target.Handles;
        var draw = ImGui.GetWindowDrawList();
        var center = HandleScreen(h.CenterX, h.CenterY);
        var resize = HandleScreen(h.ResizeX, h.ResizeY);
        draw.AddCircle(center, 3, OutlineColor);
        draw.AddRectFilled(resize - new Vector2(5), resize + new Vector2(5), OutlineColor);
        if (h.CanHeight != 0)
        {
            var height = HandleScreen(h.HeightX, h.HeightY);
            draw.AddLine(center, height, OutlineColor);
            draw.AddQuadFilled(height + new Vector2(0, -6), height + new Vector2(6, 0),
                height + new Vector2(0, 6), height + new Vector2(-6, 0), OutlineColor);
        }
        if (h.CanRotate != 0)
        {
            var rotate = HandleScreen(h.RotateX, h.RotateY);
            draw.AddLine(center, rotate, OutlineColor);
            draw.AddCircle(rotate, 6, OutlineColor, 0, 2);
        }
    }
}
