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
    public Action? DrawControls { get; set; }

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
    private PixieSceneScope[] EnvironmentScopes = [];
    private long NextEnvironmentScopeRead;
    private readonly bool[] ButtonsSent = new bool[8];
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
    // PressedOn/Boxing can remain cached after release; only PressedAt denotes a live press.
    internal bool HasActiveAuthoringGesture => PressedAt != null || Moving != null || ColliderDrag != null || LightDrag != null;

    public ViewportPanel Started()
    {
        // Start-screen/build-screen input belongs to the UI, never to the newly opened game.
        KeyEvents.Pending.Clear(); KeyEvents.Collect = true;
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
            DrawControls?.Invoke();
            if (session.ProjectFolder != null) { DrawEnvironmentChoice(); DrawCameraChoice(); }
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
            DrawLightHandles();
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
            FinishLightDrag(cancel: false);
#if PIXIE_DEVELOPMENT
            if (!AgentDriven)
#endif
            SendInput();
        }

        ImGui.End();
        ImGui.PopStyleVar();
    }

    private void DrawCameraChoice()
    {
        var choice = session.ViewportCameraSelection;
        var preview = choice.Kind == 0 ? "Free editor camera" : choice.Kind == 1 ? "Authored camera defaults" : $"Camera {choice.Entity:x16}";
        ImGui.SetNextItemWidth(Math.Min(260, Math.Max(1, ImGui.GetContentRegionAvail().X)));
        if (ImGui.BeginCombo("##viewport-camera", preview))
        {
            if (ImGui.Selectable("Free editor camera", choice.Kind == 0)) session.SelectViewportCamera(0);
            if (ImGui.Selectable("World camera default", choice.Kind == 1 && choice.Scene == 0)) session.SelectViewportCamera(1);
            foreach (var scope in session.SceneScopes())
                if (ImGui.Selectable($"Scene camera: {scope.Name}##camera-{scope.Id:x16}", choice.Kind == 1 && choice.Scene == scope.Id))
                    session.SelectViewportCamera(1, scene: scope.Id);
            foreach (var camera in session.PresentationSnapshot()["cameras"]!.AsArray())
            {
                var id = ulong.Parse(camera!["id"]!.GetValue<string>(), System.Globalization.NumberStyles.HexNumber);
                if (ImGui.Selectable($"{camera["name"]!.GetValue<string>()}##camera-entity-{id:x16}", choice.Kind == 2 && choice.Entity == id))
                    session.SelectViewportCamera(2, id, choice.Scene);
            }
            ImGui.EndCombo();
        }
        var diagnostic = session.CameraDiagnostic;
        if (diagnostic.Length > 0) ImGui.TextWrapped(diagnostic);
    }

    private void DrawEnvironmentChoice()
    {
        var (kind, sceneId) = session.ViewportEnvironmentSelection;
        var now = System.Environment.TickCount64;
        if (now >= NextEnvironmentScopeRead)
        {
            EnvironmentScopes = session.SceneScopes();
            NextEnvironmentScopeRead = now + 250;
        }
        var scopes = EnvironmentScopes;
        var selected = scopes.FirstOrDefault(scope => scope.Id == sceneId);
        var label = kind == PixieEnvironmentSelectionKind.LegacyWorld ? "Legacy World"
            : selected.Id != 0 ? $"Scene: {selected.Name}"
            : $"Scene {sceneId:x16} (unavailable)";
        ImGui.TextDisabled("Environment");
        ImGui.SameLine();
        ImGui.SetNextItemWidth(Math.Max(1, Math.Min(260, ImGui.GetContentRegionAvail().X)));
        if (ImGui.BeginCombo("##viewport-environment", label))
        {
            if (ImGui.Selectable("Legacy World", kind == PixieEnvironmentSelectionKind.LegacyWorld))
                session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.LegacyWorld);
            foreach (var scope in scopes)
                if (ImGui.Selectable($"Scene: {scope.Name}##environment-{scope.Id:x16}",
                        kind == PixieEnvironmentSelectionKind.Scene && sceneId == scope.Id))
                    session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, scope.Id);
            ImGui.EndCombo();
        }
        var resolved = session.ViewportEnvironment;
        if (resolved.Resolution == PixieEnvironmentResolutionKind.RemovedSceneFallback)
            ImGui.TextWrapped(resolved.Diagnostic);
        else if (resolved.Resolution == PixieEnvironmentResolutionKind.SceneOverride)
            ImGui.TextDisabled("Scene Environment override");
        else if (resolved.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld)
            ImGui.TextDisabled("Scene Environment inherits World lighting");
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

    private uint InputModifiers;
    private Vector2 LastHostCursor;

    private void SendInput()
    {
        while (KeyEvents.Pending.TryDequeue(out var raw))
        {
            var input = raw;
            if (input.Kind is 0 or 1) InputModifiers = input.Modifiers;
            else input.Modifiers = InputModifiers;
            if (input.Kind == 4)
            {
                session.InputEvent(input);
                Array.Clear(KeysSent); Array.Clear(ButtonsSent); InputModifiers = 0;
                continue;
            }
            if (input.Kind == 0)
            {
                if (input.Code < 0 || input.Code > LastKey) continue;
                if (input.Action == 0 ? !KeysSent[input.Code] : !Focused || (input.Action == 2 && !KeysSent[input.Code])) continue;
                session.InputEvent(input);
                KeysSent[input.Code] = input.Action != 0;
                continue;
            }
            var captured = ButtonsSent.Any(value => value);
            if (input.Kind == 2) LastHostCursor = new Vector2((float)input.X, (float)input.Y);
            if (input.Kind is 1 or 2)
            {
                var mouse = (new Vector2((float)input.X, (float)input.Y) - ImageOrigin) * Scale;
                if (input.Kind == 1)
                {
                    if (input.Code < 0 || input.Code >= ButtonsSent.Length) continue;
                    if (input.Action == 0 ? !ButtonsSent[input.Code] : !Hovered || !GoesToGame(input.Code)
                        || mouse.X < 0 || mouse.Y < 0 || mouse.X >= Width || mouse.Y >= Height) continue;
                    // A button carries its event-time pointer position even if the cursor moved
                    // elsewhere before this UI frame. This does not invent a world coordinate.
                    session.InputEvent(new() { Kind = 2, X = mouse.X, Y = mouse.Y, Modifiers = input.Modifiers });
                    session.InputEvent(input);
                    ButtonsSent[input.Code] = input.Action != 0;
                    CursorInside = true;
                }
                else if (captured || (Mode == ViewportMode.Game && Hovered))
                {
                    input.X = mouse.X; input.Y = mouse.Y;
                    session.InputEvent(input); CursorInside = true;
                }
            }
            else if (input.Kind == 3 && Hovered)
            {
                var mouse = (LastHostCursor - ImageOrigin) * Scale;
                session.InputEvent(new() { Kind = 2, X = mouse.X, Y = mouse.Y, Modifiers = input.Modifiers });
                session.InputEvent(input); CursorInside = true;
            }
        }
        if (Mode == ViewportMode.Game && (Hovered || ButtonsSent.Any(value => value)))
        {
            var mouse = MouseInView();
            session.InputEvent(new() { Kind = 2, X = mouse.X, Y = mouse.Y, Modifiers = InputModifiers });
            CursorInside = true;
        }
        for (var button = 0; button < ButtonsSent.Length; ++button)
            if (ButtonsSent[button] && !GoesToGame(button))
            {
                session.InputEvent(new() { Kind = 1, Code = button, Action = 0, Modifiers = InputModifiers });
                ButtonsSent[button] = false;
            }
        if (!Focused)
            for (var key = 0; key <= LastKey; ++key)
                if (KeysSent[key])
                {
                    session.InputEvent(new() { Kind = 0, Code = key, Action = 0 });
                    KeysSent[key] = false;
                }
        if (CursorInside && !Hovered && !ButtonsSent.Any(value => value))
        {
            session.InputEvent(new() { Kind = 2, X = -1, Y = -1, Modifiers = InputModifiers });
            CursorInside = false;
        }
    }

    // Select mode's gestures with the left button.
    private void Select()
    {
        if (HandleColliderInput() || HandleLightInput()) return;
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
