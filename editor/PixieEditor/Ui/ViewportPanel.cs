using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// How the mouse over the viewport is used: played with, or used to choose and edit entities.
internal enum ViewportMode
{
    // The mouse goes to the game, as in its player.
    Game,
    // The mouse selects: a click chooses an entity, Shift + click adds or takes out one, a drag on
    // empty space chooses by box, and Ctrl + click deletes the boid, attractor, spawner, or particle
    // under the cursor, or asks the game to create one on empty space. The middle button and the wheel
    // still go to the game, to pan and zoom.
    Select,
}

// The game's view: the session draws the World into its texture, sized to this window, and the window
// shows it with ImGui.Image, with the selection outlined on top. The mouse over the image goes to the
// session as input in the image's pixels in Game mode, and selects in Select mode; the keys go to the
// session while the window has focus, in both.
internal sealed unsafe class ViewportPanel(EngineSession session, Selection selection)
{
    private const int LastKey = 348;
    private const int MiddleButton = 2;
    // How far, in pixels, a click may miss a shape, and how far the mouse moves before a press is a drag.
    private const float PickTolerance = 3f;
    private const float DragThreshold = 4f;
    // How close, in pixels, a Ctrl + click must be to a particle to delete it.
    private const float ParticleReach = 6f;

    private static readonly uint OutlineColor = ImGui.ColorConvertFloat4ToU32(new Vector4(1.0f, 0.78f, 0.25f, 1.0f));
    private static readonly uint BoxColor = ImGui.ColorConvertFloat4ToU32(new Vector4(0.45f, 0.75f, 1.0f, 1.0f));
    private static readonly uint BoxFill = ImGui.ColorConvertFloat4ToU32(new Vector4(0.45f, 0.75f, 1.0f, 0.12f));
    private static readonly uint PlayingFrame = ImGui.ColorConvertFloat4ToU32(new Vector4(1.0f, 0.65f, 0.3f, 1.0f));

    public ViewportMode Mode { get; set; } = ViewportMode.Game;

    private Vector2 ImageOrigin;
    private Vector2 Scale = Vector2.One;
    private bool Hovered;
    private bool CursorInside;
    private bool Focused;
    private int Width;
    private int Height;
    private readonly bool[] ButtonsSent = new bool[3];
    private readonly bool[] KeysSent = new bool[LastKey + 1];

    // A left press in Select mode, in view pixels, until it is released.
    private Vector2? PressedAt;
    private bool PressShift;
    private bool Boxing;

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
            Scale = ImGui.GetIO().DisplayFramebufferScale;
            var width = Math.Max(1, (int)(size.X * Scale.X));
            var height = Math.Max(1, (int)(size.Y * Scale.Y));
            if (width != Width || height != Height)
            {
                session.Resize(width, height);
                Width = width;
                Height = height;
            }

            Focused = ImGui.IsWindowFocused();
            ImageOrigin = ImGui.GetCursorScreenPos();
            // Hover from this frame's mouse, before the image is drawn, so a press is judged where it
            // happened rather than where the mouse was a frame ago.
            var mouse = ImGui.GetMousePos();
            Hovered = ImGui.IsWindowHovered() && mouse.X >= ImageOrigin.X && mouse.Y >= ImageOrigin.Y &&
                      mouse.X < ImageOrigin.X + size.X && mouse.Y < ImageOrigin.Y + size.Y;
            SendInput();
            if (Mode == ViewportMode.Select)
                Select();
            session.Advance(frameDelta);
            var texture = session.Render();

            // GL's first row is the bottom of the image; ImGui draws top down, so flip the coordinates.
            ImGui.Image(new ImTextureRef(null, new ImTextureID(texture)), size, new Vector2(0, 1), new Vector2(1, 0));
            DrawSelection();
            // A project at play is framed, so it is never mistaken for the scene being edited.
            if (session.ProjectFolder != null && !session.Editing)
                ImGui.GetWindowDrawList().AddRect(ImageOrigin, ImageOrigin + size, PlayingFrame, 0f, ImDrawFlags.None, 3f);
        }
        else
        {
            Hovered = false;
            Focused = false;
            PressedAt = null;
            SendInput();
        }

        ImGui.End();
        ImGui.PopStyleVar();
    }

    // The mouse in view pixels.
    private Vector2 MouseInView() => (ImGui.GetMousePos() - ImageOrigin) * Scale;

    private bool GoesToGame(int button) => Mode == ViewportMode.Game || button == MiddleButton;

    private void SendInput()
    {
        var mouse = MouseInView();
        var wheel = Hovered ? ImGui.GetIO().MouseWheel : 0f;
        var captured = ButtonsSent.Any(b => b);
        // In Select mode the game sees the pointer only to pan and zoom, so boids do not chase it.
        var pointing = Mode == ViewportMode.Game ? Hovered || captured : captured || wheel != 0f;
        if (pointing)
        {
            session.Cursor(mouse.X, mouse.Y);
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
            var down = ImGui.IsMouseDown((ImGuiMouseButton)button) && GoesToGame(button);
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

        if (wheel != 0f)
            session.Scroll(0.0, wheel);

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

    // Select mode's gestures with the left button.
    private void Select()
    {
        var io = ImGui.GetIO();
        var mouse = MouseInView();
        if (Hovered && ImGui.IsMouseClicked(ImGuiMouseButton.Left))
        {
            if (io.KeyCtrl)
            {
                DeleteOrCreate(mouse);
                return;
            }

            PressedAt = mouse;
            PressShift = io.KeyShift;
            Boxing = false;
        }

        if (PressedAt is not { } start)
            return;

        if (ImGui.IsMouseDown(ImGuiMouseButton.Left))
        {
            Boxing |= Vector2.Distance(start, mouse) > DragThreshold * Scale.X;
            return;
        }

        // Released: a box, or a click.
        PressedAt = null;
        if (Boxing)
        {
            var boxed = session.PickBox(start.X, start.Y, mouse.X, mouse.Y);
            if (PressShift)
                selection.Add(boxed);
            else
                selection.Set(boxed);
            Log($"Selected {boxed.Length} in a box; {selection.Items.Count} selected");
            return;
        }

        if (session.Pick(start.X, start.Y, PickTolerance * Scale.X) is { } picked)
        {
            if (PressShift)
                selection.Toggle(picked);
            else
                selection.Set(picked);
            Log($"Clicked #{picked & 0xFFFFF}; {selection.Items.Count} selected");
        }
        else if (!PressShift)
        {
            selection.Clear();
        }
    }

    // Ctrl + click: delete what is under the cursor, an entity first, then a particle; on empty space,
    // ask the game to create something.
    private void DeleteOrCreate(Vector2 at)
    {
        if (session.Pick(at.X, at.Y, PickTolerance * Scale.X) is { } picked)
        {
            var name = session.EntityName(picked);
            session.Destroy(picked);
            selection.Prune(session);
            Log($"Deleted {(name.Length > 0 ? name + " " : "")}#{picked & 0xFFFFF}");
        }
        else if (session.RemoveParticleAt(at.X, at.Y, ParticleReach * Scale.X))
        {
            Log("Deleted a particle");
        }
        else
        {
            Log(session.SpawnAt(at.X, at.Y) ? $"Created one at ({at.X:0}, {at.Y:0})" : "The game creates nothing there");
        }
    }

    // Outlines the selected entities where the last render drew them, and the box being dragged.
    private void DrawSelection()
    {
        var draw = ImGui.GetWindowDrawList();
        foreach (var entity in selection.Items)
        {
            if (session.ViewBounds(entity) is not { } bounds)
                continue;
            var center = ImageOrigin + new Vector2((float)bounds.X, (float)bounds.Y) / Scale;
            draw.AddCircle(center, (float)bounds.Radius / Scale.X + 4f, OutlineColor, 0, 1.5f);
        }

        if (Mode == ViewportMode.Select && Boxing && PressedAt is { } start)
        {
            var a = ImageOrigin + start / Scale;
            var b = ImGui.GetMousePos();
            var min = Vector2.Min(a, b);
            var max = Vector2.Max(a, b);
            draw.AddRectFilled(min, max, BoxFill);
            draw.AddRect(min, max, BoxColor);
        }
    }

    private static void Log(string message) => Console.Error.WriteLine($"[info] {message}");
}
