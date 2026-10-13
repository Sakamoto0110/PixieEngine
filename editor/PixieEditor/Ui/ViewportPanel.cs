using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// The game's view: the session draws the World into its texture, sized to this window, and the window
// shows it with ImGui.Image. The mouse over the image, and the keys while the window has focus, go to
// the session as input in the image's pixels.
internal sealed unsafe class ViewportPanel(EngineSession session)
{
    private const int LastKey = 348;

    private Vector2 ImageOrigin;
    private bool Hovered;
    private bool CursorInside;
    private bool Focused;
    private int Width;
    private int Height;
    private readonly bool[] ButtonsSent = new bool[3];
    private readonly bool[] KeysSent = new bool[LastKey + 1];

    public ViewportPanel Started()
    {
        // The session's cursor starts at (0, 0), inside the view; move it out until the mouse comes in.
        session.Cursor(-1.0, -1.0);
        return this;
    }

    public void Draw(double frameDelta)
    {
        ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, Vector2.Zero);
        if (ImGui.Begin("Viewport", ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse))
        {
            var size = ImGui.GetContentRegionAvail();
            var scale = ImGui.GetIO().DisplayFramebufferScale;
            var width = Math.Max(1, (int)(size.X * scale.X));
            var height = Math.Max(1, (int)(size.Y * scale.Y));
            if (width != Width || height != Height)
            {
                session.Resize(width, height);
                Width = width;
                Height = height;
            }

            Focused = ImGui.IsWindowFocused();
            ImageOrigin = ImGui.GetCursorScreenPos();
            SendInput(scale);
            session.Advance(frameDelta);
            var texture = session.Render();

            // GL's first row is the bottom of the image; ImGui draws top down, so flip the coordinates.
            ImGui.Image(new ImTextureRef(null, new ImTextureID(texture)), size, new Vector2(0, 1), new Vector2(1, 0));
            Hovered = ImGui.IsItemHovered();
        }
        else
        {
            Hovered = false;
            Focused = false;
            SendInput(Vector2.One);
        }

        ImGui.End();
        ImGui.PopStyleVar();
    }

    private void SendInput(Vector2 scale)
    {
        var mouse = ImGui.GetMousePos() - ImageOrigin;
        var captured = ButtonsSent.Any(b => b);
        if (Hovered || captured)
        {
            session.Cursor(mouse.X * scale.X, mouse.Y * scale.Y);
            CursorInside = true;
        }
        else if (CursorInside)
        {
            // Off the image: a position outside the view, so the game sees no pointer.
            session.Cursor(-1.0, -1.0);
            CursorInside = false;
        }

        for (var button = 0; button < ButtonsSent.Length; button++)
        {
            var down = ImGui.IsMouseDown((ImGuiMouseButton)button);
            if (down && !ButtonsSent[button] && Hovered && ImGui.IsMouseClicked((ImGuiMouseButton)button))
            {
                session.MouseButton(button, true);
                ButtonsSent[button] = true;
            }
            else if (!down && ButtonsSent[button])
            {
                session.MouseButton(button, false);
                ButtonsSent[button] = false;
            }
        }

        if (Hovered && ImGui.GetIO().MouseWheel != 0f)
            session.Scroll(0.0, ImGui.GetIO().MouseWheel);

        // Keys reach the game while the viewport has focus; the rest of the editor keeps them otherwise.
        while (KeyEvents.Pending.TryDequeue(out var e))
        {
            if (e.Key < 0 || e.Key > LastKey || (e.Down && !Focused) || e.Down == KeysSent[e.Key])
                continue;
            session.Key(e.Key, e.Down);
            KeysSent[e.Key] = e.Down;
        }

        if (!Focused)
        {
            // Focus left with keys held: release them, so the game does not see them stuck.
            for (var key = 0; key <= LastKey; key++)
            {
                if (KeysSent[key])
                {
                    session.Key(key, false);
                    KeysSent[key] = false;
                }
            }
        }
    }
}
