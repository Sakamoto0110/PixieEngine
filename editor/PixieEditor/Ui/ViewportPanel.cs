using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// How the mouse over the viewport is used: played with, or used to choose and edit entities.
internal enum ViewportMode
{
    // The mouse goes to the game, as in its player.
    Game,
    // The mouse selects and moves: a click chooses an entity, Shift + click adds or takes out one, a
    // drag on an entity moves it (and the selection with it, when it was selected), Shift holding the
    // drag to one axis and Snap rounding it to the grid; a drag on empty space chooses by box; and a
    // right click opens a menu of what can be done there: duplicate or delete the entities, or, on
    // empty space, create an entity or a sprite, ask the game to create one, or delete a particle. The
    // middle button and the wheel still go to the game, to pan and zoom.
    Select,
}

// The game's view: the session draws the World into its texture, sized to this window, and the window
// shows it with ImGui.Image, with the selection outlined on top. The mouse over the image goes to the
// session as input in the image's pixels in Game mode, and selects in Select mode; the keys go to the
// session while the window has focus, in both. PNG files dropped on the window become sprites where
// the mouse is, or in the middle of the view when it is elsewhere, in either mode.
internal sealed unsafe partial class ViewportPanel(EngineSession session, Selection selection, Authoring authoring, AssetChooser files)
{
    // The grids Snap can round to, in world units.
    public static readonly double[] Grids = [8, 16, 32, 64];

    // Whether a move rounds the dragged entity's position to the grid, and the grid's size.
    public bool Snap { get; set; }
    public double Grid { get; set; } = 16;

    private const int LastKey = 348;
    private const int MiddleButton = 2;
    // How far, in pixels, a click may miss a shape, and how far the mouse moves before a press is a drag.
    private const float PickTolerance = 3f;
    private const float DragThreshold = 4f;
    // How close, in pixels, the menu's point must be to a particle to delete it.
    private const float ParticleReach = 6f;

    private static readonly uint OutlineColor = ImGui.ColorConvertFloat4ToU32(new Vector4(1.0f, 0.78f, 0.25f, 1.0f));
    private static readonly uint BoxColor = ImGui.ColorConvertFloat4ToU32(new Vector4(0.45f, 0.75f, 1.0f, 1.0f));
    private static readonly uint BoxFill = ImGui.ColorConvertFloat4ToU32(new Vector4(0.45f, 0.75f, 1.0f, 0.12f));
    private static readonly uint PlayingFrame = ImGui.ColorConvertFloat4ToU32(new Vector4(1.0f, 0.65f, 0.3f, 1.0f));

    public ViewportMode Mode { get; set; } = ViewportMode.Game;
#if PIXIE_DEVELOPMENT
    public Development.DeterministicCapture? Capture { get; set; }
    public bool AgentDriven { get; set; }
    public bool ChannelVisible { get; private set; }
    public Action<EngineException>? AgentRenderFailure { get; set; }
#endif

    private Vector2 ImageOrigin;
    private Vector2 Scale = Vector2.One;
    private bool Hovered;
    private bool CursorInside;
    private bool Focused;
    private int Width;
    private int Height;
    private readonly bool[] ButtonsSent = new bool[3];
    private readonly bool[] KeysSent = new bool[LastKey + 1];

    // A left press in Select mode, in view pixels, until it is released; the entity it was on; and the
    // move it became, if it did.
    private Vector2? PressedAt;
    private uint? PressedOn;
    private Move? Moving;
    private (double X, double Y) MoveFrom;

    // Where the menu was opened, in view pixels, and what it was opened on.
    private const string MenuId = "viewport-menu";
    private Vector2 MenuAt;
    private uint? MenuTarget;
    private bool MenuParticles;
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
#if PIXIE_DEVELOPMENT
            ChannelVisible = true;
#endif
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
#if PIXIE_DEVELOPMENT
            if (Capture == null && !AgentDriven)
#endif
            SendInput();
            if (Mode == ViewportMode.Select
#if PIXIE_DEVELOPMENT
                && !AgentDriven
#endif
            )
                Select();
            while (DropEvents.Pending.TryDequeue(out var dropped))
            {
#if PIXIE_DEVELOPMENT
                if (AgentDriven) continue;
#endif
                Drop(dropped, Hovered ? MouseInView() : null);
            }
#if PIXIE_DEVELOPMENT
            if (!AgentDriven)
            {
                if (Capture != null) Capture.Advance();
                else session.Advance(frameDelta);
            }
#else
            session.Advance(frameDelta);
#endif
            var texture = RenderTexture();

            // GL's first row is the bottom of the image; ImGui draws top down, so flip the coordinates.
            if (texture != 0) ImGui.Image(new ImTextureRef(null, new ImTextureID(texture)), size, new Vector2(0, 1), new Vector2(1, 0));
            DrawSelection();
            DrawColliderHandles();
            DrawMenu();
            // A project at play is framed, so it is never mistaken for the scene being edited.
            if (session.ProjectFolder != null && !session.Editing)
                ImGui.GetWindowDrawList().AddRect(ImageOrigin, ImageOrigin + size, PlayingFrame, 0f, ImDrawFlags.None, 3f);
        }
        else
        {
#if PIXIE_DEVELOPMENT
            ChannelVisible = false;
            // A saved collapsed/tabbed-out viewport must not stall a finite capture run.
            Capture?.Advance();
            if (AgentDriven) RenderTexture();
#endif
            Hovered = false;
            Focused = false;
            PressedAt = null;
            FinishColliderDrag();
#if PIXIE_DEVELOPMENT
            if (!AgentDriven)
#endif
            SendInput();
        }

        ImGui.End();
        ImGui.PopStyleVar();
    }

    private uint RenderTexture()
    {
#if PIXIE_DEVELOPMENT
        if (AgentDriven)
        {
            try { return session.Render(); }
            catch (EngineException error) { AgentRenderFailure?.Invoke(error); return 0; }
        }
#endif
        return session.Render();
    }

    // The mouse in view pixels.
    private Vector2 MouseInView() => (ImGui.GetMousePos() - ImageOrigin) * Scale;

    // Makes sprites of the PNG files among `files`, at `at` in view pixels (the middle of the view when
    // null), as one step, each at most half the view's height or width; returns them.
    public uint[] Drop(IReadOnlyList<string> files, Vector2? at)
    {
        var point = at ?? new Vector2(Width / 2f, Height / 2f);
        try
        {
            var corner = session.ViewToWorld(0, 0);
            var opposite = session.ViewToWorld(Width, Height);
            var fit = 0.5 * Math.Min(Math.Abs(opposite.X - corner.X), Math.Abs(opposite.Y - corner.Y));
            if (!(fit > 0))
                fit = double.PositiveInfinity;
            var made = authoring.PlaceImages(files, session.ViewToWorld(point.X, point.Y), fit);
            if (made.Length > 0)
                Log($"Dropped {made.Length} image(s) at ({point.X:0}, {point.Y:0})");
            return made;
        }
        catch (EngineException e)
        {
            Status.Show(e.Message, error: true);
            return [];
        }
    }

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
        if (HandleColliderInput()) return;
        var io = ImGui.GetIO();
        var mouse = MouseInView();
        if (Hovered && ImGui.IsMouseClicked(ImGuiMouseButton.Left))
        {
            PressedAt = mouse;
            PressShift = io.KeyShift;
            Boxing = false;
            PressedOn = session.Pick(mouse.X, mouse.Y, PickTolerance * Scale.X);
        }

        if (Hovered && ImGui.IsMouseClicked(ImGuiMouseButton.Right))
            OpenMenu(mouse);

        if (PressedAt is not { } start)
            return;

        if (ImGui.IsMouseDown(ImGuiMouseButton.Left))
        {
            if (Moving == null && !Boxing && Vector2.Distance(start, mouse) > DragThreshold * Scale.X)
                StartDrag(start);
            if (Moving != null)
                DragTo(mouse, io.KeyShift);
            return;
        }

        // Released: a move, a box, or a click.
        PressedAt = null;
        if (Moving != null)
        {
            authoring.EndMove(Moving);
            Moving = null;
            return;
        }

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

    // A drag that starts on an entity moves it, with the selection when it is selected; one that starts
    // on empty space selects by box.
    private void StartDrag(Vector2 start)
    {
        if (PressedOn is not { } target || !session.IsAlive(target))
        {
            Boxing = true;
            return;
        }

        if (!selection.Contains(target))
        {
            if (PressShift)
                selection.Add([target]);
            else
                selection.Set(target);
        }

        Moving = authoring.BeginMove([.. selection.Items], target);
        MoveFrom = session.ViewToWorld(start.X, start.Y);
        if (Moving == null)
            Boxing = true;
    }

    private void DragTo(Vector2 mouse, bool oneAxis)
    {
        var at = session.ViewToWorld(mouse.X, mouse.Y);
        var dx = at.X - MoveFrom.X;
        var dy = at.Y - MoveFrom.Y;
        if (oneAxis)
        {
            if (Math.Abs(dx) >= Math.Abs(dy))
                dy = 0;
            else
                dx = 0;
        }

        if (Snap && Grid > 0)
        {
            // The dragged entity lands on the grid; the others keep their places around it.
            var anchor = Moving!.Anchor;
            dx = Math.Round((anchor.X + dx) / Grid) * Grid - anchor.X;
            dy = Math.Round((anchor.Y + dy) / Grid) * Grid - anchor.Y;
            if (oneAxis && Math.Abs(at.X - MoveFrom.X) >= Math.Abs(at.Y - MoveFrom.Y))
                dy = 0;
            else if (oneAxis)
                dx = 0;
        }

        Moving!.By(dx, dy);
    }

    // A right click: on an entity, it joins the selection if it was not in it, and the menu offers to
    // duplicate or delete the selection; on empty space, to create there.
    private void OpenMenu(Vector2 at)
    {
        MenuAt = at;
        MenuTarget = session.Pick(at.X, at.Y, PickTolerance * Scale.X);
        if (MenuTarget is { } target && !selection.Contains(target))
            selection.Set(target);
        var particleSystem = session.TypeIndex("ParticleSystem");
        MenuParticles = MenuTarget == null && particleSystem >= 0 && session.Entities().Any(id => session.Has(id, particleSystem));
        ImGui.OpenPopup(MenuId);
    }

    private void DrawMenu()
    {
        if (!ImGui.BeginPopup(MenuId))
            return;

        // A menu action the engine refuses is reported, never fatal.
        try
        {
            DrawMenuItems();
        }
        catch (EngineException e)
        {
            Status.Show(e.Message, error: true);
        }

        ImGui.EndPopup();
    }

    private void DrawMenuItems()
    {
        if (MenuTarget != null)
        {
            var chosen = selection.Items.ToArray();
            if (chosen.Length == 0)
            {
                // What the menu was for is gone.
                ImGui.CloseCurrentPopup();
                return;
            }

            ImGui.TextDisabled(chosen.Length == 1 ? Describe(chosen[0]) : $"{chosen.Length} entities");
            ImGui.Separator();
            if (ImGui.MenuItem(chosen.Length == 1 ? "Duplicate" : $"Duplicate {chosen.Length} entities", "Ctrl+D"))
                authoring.Duplicate(chosen);
            if (ImGui.MenuItem(chosen.Length == 1 ? "Delete" : $"Delete {chosen.Length} entities", "Delete"))
                authoring.Delete(chosen);
        }
        else
        {
            if (ImGui.MenuItem("Create entity here"))
            {
                if (authoring.CreateEntity(session.ViewToWorld(MenuAt.X, MenuAt.Y)) is { } created)
                    Log($"Created {Describe(created)} at ({MenuAt.X:0}, {MenuAt.Y:0})");
            }

            if (session.TypeIndex("Sprite") >= 0 && ImGui.MenuItem("New sprite here..."))
            {
                var at = MenuAt;
                files.Open("png", file => Drop([file], at));
            }

            if (session.TypeIndex("Collider2D") >= 0 && ImGui.MenuItem("New collider here"))
                authoring.CreateMovementEntity(false, session.ViewToWorld(MenuAt.X, MenuAt.Y));
            if (session.TypeIndex("CharacterMotor2D") >= 0 && ImGui.MenuItem("New character here"))
                authoring.CreateMovementEntity(true, session.ViewToWorld(MenuAt.X, MenuAt.Y));
            if (session.TypeIndex("RigidBody2D") >= 0 && ImGui.BeginMenu("Physics"))
            {
                foreach (var (kind, label) in Authoring.PhysicsPresets)
                    if (ImGui.MenuItem(label)) authoring.CreatePhysicsEntity(kind, session.ViewToWorld(MenuAt.X, MenuAt.Y));
                ImGui.EndMenu();
            }

            // A project's own scene has nothing to ask the game for.
            if (session.ProjectFolder == null && ImGui.MenuItem("Ask the game to create one here"))
                Log(session.SpawnAt(MenuAt.X, MenuAt.Y) ? $"The game created one at ({MenuAt.X:0}, {MenuAt.Y:0})" : "The game creates nothing there");

            if (MenuParticles && ImGui.MenuItem("Delete the particle here"))
            {
                if (session.RemoveParticleAt(MenuAt.X, MenuAt.Y, ParticleReach * Scale.X))
                    Log("Deleted a particle");
                else
                    Status.Show($"No particle within {ParticleReach:0} pixels.");
            }
        }
    }

    private string Describe(uint entity)
    {
        var name = session.EntityName(entity);
        return name.Length > 0 ? $"{name} #{entity & 0xFFFFF}" : $"#{entity & 0xFFFFF}";
    }

    // Outlines the selected entities where the last render drew them, and the box being dragged.
    private void DrawSelection()
    {
        var draw = ImGui.GetWindowDrawList();
        foreach (var entity in selection.Items)
        {
            // A sprite is outlined by its rectangle, and a text by its block, turned as they are drawn; a
            // path along its line; anything else by a circle.
            if (session.ViewQuad(entity) is { } quad)
            {
                var corners = quad.Select(c => ImageOrigin + new Vector2((float)c.X, (float)c.Y) / Scale).ToArray();
                draw.AddQuad(corners[0], corners[1], corners[2], corners[3], OutlineColor, 1.5f);
                continue;
            }

            if (session.ViewPath(entity) is { Length: > 1 } line)
            {
                var points = line.Select(c => ImageOrigin + new Vector2((float)c.X, (float)c.Y) / Scale).ToArray();
                for (var i = 1; i < points.Length; i++)
                    draw.AddLine(points[i - 1], points[i], OutlineColor, 1.5f);
                foreach (var point in new[] { points[0], points[^1] })
                    draw.AddCircle(point, 4f, OutlineColor, 0, 1.5f);
                continue;
            }

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
