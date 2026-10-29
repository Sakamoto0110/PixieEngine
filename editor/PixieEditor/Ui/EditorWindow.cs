using System.Diagnostics;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Native;
// Hexa.NET.GLFW and the GLFW backend each have a window handle type, over the same GLFW window.
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;
using GLFWmonitorPtr = Hexa.NET.GLFW.GLFWmonitorPtr;
using GLFWwindowPtr = Hexa.NET.GLFW.GLFWwindowPtr;

namespace PixieEditor.Ui;

// The editor's window: GLFW with an OpenGL 4.5 core context, which the engine shares, Dear ImGui with
// docking through its GLFW and OpenGL3 backends, and the four panels. Without a game or a project on
// the command line, it shows the start screen first. One game or project per window: P/Invoke binds
// one library per process, so another one opens in a new editor.
internal sealed class EditorWindow(EditorOptions options)
{
    // What closing the window, or opening another project, waits on when the scene is not saved.
    private enum Leaving { None, Close, OtherProject, Reopen }

    private const int ViewWidth = 800;
    private const int ViewHeight = 600;
    private const string UnsavedPopup = "Unsaved changes";

    private readonly EditorSettings Settings = EditorSettings.Load();
    private double Start;
    // Whether there was no saved dock layout when the editor started; ImGui writes one while the start
    // screen shows, so it is decided before the first frame.
    private bool FirstLayout;

    public unsafe int Run()
    {
        if (GLFW.Init() == 0)
            throw new InvalidOperationException("GLFW could not start.");

        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_FORWARD_COMPAT, GLFW.GLFW_TRUE);

        var window = GLFW.CreateWindow(1600, 900, "PixieEditor", default(GLFWmonitorPtr), default(GLFWwindowPtr));
        if (window.Handle == null)
        {
            GLFW.Terminate();
            throw new InvalidOperationException("GLFW could not open a window with an OpenGL 4.5 core context.");
        }

        WindowIcon.Apply(window);
        GLFW.MakeContextCurrent(window);
        GLFW.SwapInterval(1);
        Gl.Load();

        FirstLayout = !File.Exists("imgui.ini");
        var context = ImGui.CreateContext();
        ImGui.SetCurrentContext(context);
        var io = ImGui.GetIO();
        // No keyboard navigation: the arrows belong to the game while the viewport has focus.
        io.ConfigFlags |= ImGuiConfigFlags.DockingEnable;
        KeyEvents.Install(window);
        DropEvents.Install(window);
        ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context);
        ImGuiImplOpenGL3.Init("#version 450");

        Start = GLFW.GetTime();
        var exitCode = 0;
        var session = Open(window);
        if (session != null)
        {
            using (session)
            {
                exitCode = Edit(window, session);
            }
        }

        ImGuiImplOpenGL3.Shutdown();
        ImGuiImplGLFW.Shutdown();
        // Each backend DLL keeps its own copy of the current context, which DestroyContext does not
        // clear; left pointing at the freed context, it faulted the process at exit about half the
        // time (the cause was found in PixieInspector: Sakamoto0110/PixieInspector#6).
        ImGuiImplOpenGL3.SetCurrentContext(default);
        ImGuiImplGLFW.SetCurrentContext(default);
        ImGui.DestroyContext(context);
        GLFW.DestroyWindow(window);
        GLFW.Terminate();
        return exitCode;
    }

    // The session the command line asks for, or the one chosen on the start screen; null when the
    // window was closed first.
    private EngineSession? Open(GLFWwindowPtr window)
    {
        if (options.After is { } pid)
            WaitForExit(pid);

        if (options.Project is { } folder)
            return OpenProject(window, folder);

        if (options.Game is { } game)
        {
            GameLibrary.Use(GameLibrary.Find(game));
            return EngineSession.Create(ViewWidth, ViewHeight, options.Seed);
        }

        return ChooseProject(window);
    }

    private unsafe EngineSession? ChooseProject(GLFWwindowPtr window)
    {
        var screen = new StartScreen(Settings);
        while (GLFW.WindowShouldClose(window) == 0)
        {
            GLFW.PollEvents();
            BeginFrame();
            var choice = screen.Draw();
            // Keys typed on the start screen are not for the game.
            KeyEvents.Pending.Clear();
            EndFrame(window, GLFW.GetTime());
            if (choice == null)
                continue;

            try
            {
                if (choice.Create)
                {
                    // Creating needs a library: the generic one, which a new project opens with.
                    if (!GameLibrary.IsLoaded)
                        GameLibrary.Use(GameLibrary.Find(GameLibrary.SceneGame));
                    EngineSession.CreateProject(choice.Folder);
                    Console.Error.WriteLine($"[info] Created {choice.Folder}");
                }

                return OpenProject(window, choice.Folder);
            }
            catch (Exception e) when (e is EngineException or FileNotFoundException or DllNotFoundException)
            {
                screen.Error = e.Message;
            }
        }

        return null;
    }

    // Opens a project: its code, if it has any, is built first on the build screen. Null when the
    // window was closed, or when another editor opens the project instead (below).
    private EngineSession? OpenProject(GLFWwindowPtr window, string folder)
    {
        var library = GameLibrary.Find(GameLibrary.SceneGame);
        if (ProjectBuild.HasCode(folder))
        {
            if (BuildProject(window, folder) is not { } outcome)
                return null;
            library = outcome.Library ?? library;
        }

        // P/Invoke binds one library per process: if another is bound already (the generic one, after
        // creating a project on the start screen), a new editor opens this project.
        if (GameLibrary.IsLoaded && !string.Equals(GameLibrary.Loaded, Path.GetFullPath(library), StringComparison.OrdinalIgnoreCase))
        {
            Relaunch(["--project", folder]);
            return null;
        }

        if (!GameLibrary.IsLoaded)
            GameLibrary.Use(library);
        var session = EngineSession.OpenProject(ViewWidth, ViewHeight, folder);
        if (options.Recent)
            Settings.Remember(folder);
        Console.Error.WriteLine($"[info] Opened {folder} with {Path.GetFileName(library)}");
        return session;
    }

    // The build screen until the build succeeds, or the user opens the project without its code; null
    // when the window was closed.
    private unsafe BuildOutcome? BuildProject(GLFWwindowPtr window, string folder)
    {
        using var screen = new BuildScreen(folder);
        screen.Start();
        while (GLFW.WindowShouldClose(window) == 0)
        {
            GLFW.PollEvents();
            BeginFrame();
            var outcome = screen.Draw();
            KeyEvents.Pending.Clear();
            EndFrame(window, GLFW.GetTime(), timed: false);
            if (outcome != null)
                return outcome;
        }

        return null;
    }

    // Waits for an earlier editor to exit, so the game library it held can be built again.
    private static void WaitForExit(int pid)
    {
        try
        {
            using var earlier = Process.GetProcessById(pid);
            if (!earlier.WaitForExit(TimeSpan.FromSeconds(15)))
                Console.Error.WriteLine($"[warning] The editor {pid} is still running; its game library may be in use.");
        }
        catch (ArgumentException)
        {
            // It has exited already.
        }
    }

    private unsafe int Edit(GLFWwindowPtr window, EngineSession session)
    {
        // The command line's times count from here, not from a build before.
        Start = GLFW.GetTime();
        var project = session.ProjectFolder != null;
        var firstLayout = FirstLayout;
        var gameName = options.Game is { } game ? Program.GameName(game) : GameLibrary.SceneGame;
        var selection = new Selection();
        var history = new Editing.History();
        var authoring = new Authoring(session, selection, history);
        var recorder = new Editing.EditRecorder(session, history, () => authoring.Recording);
        var files = new AssetChooser(session.ProjectFolder);
        using var inspector = new InspectorPanel(session, authoring, recorder, files);
        var viewport = new ViewportPanel(session, selection, authoring, files).Started();
        viewport.Mode = options.Mode ?? (project ? ViewportMode.Select : ViewportMode.Game);
        var entities = new EntitiesPanel(session, selection, authoring);
        var controls = new SessionPanel(session, gameName);
        if (options.Select != null && entities.Find(options.Select) is { } chosen)
            selection.Set(chosen);

        if (options.TimeScale is { } scale)
            session.TimeScale = scale;
        if (options.Paused)
            session.Paused = true;

        var leaving = Leaving.None;
        Action<string>? startExport = null;
        Func<bool> exporting = () => false;
#if PIXIE_DEVELOPMENT
        using var exportPanel = new Development.ExportPanel(session);
        startExport = exportPanel.Start;
        exporting = () => exportPanel.Running;
#endif
        var askToSave = false;
        var confirmed = false;
        var toolbar = new Toolbar(session, viewport, authoring, selection)
        {
#if PIXIE_DEVELOPMENT
            Export = exportPanel.Open,
#endif
            Save = () => Save(session),
            AddCode = () =>
            {
                try
                {
                    ProjectBuild.AddCode(session.ProjectFolder!);
                    Status.Show($"Added code to {session.ProjectName}; building it");
                }
                catch (EngineException e)
                {
                    Status.Show(e.Message, error: true);
                    return;
                }

                leaving = Leaving.Reopen;
                if (session.Unsaved)
                    askToSave = true;
                else
                    confirmed = true;
            },
            BuildAndReopen = () =>
            {
                leaving = Leaving.Reopen;
                if (session.Unsaved)
                    askToSave = true;
                else
                    confirmed = true;
            },
            OtherProject = () =>
            {
                if (project && session.Unsaved)
                {
                    leaving = Leaving.OtherProject;
                    askToSave = true;
                }
                else
                {
                    OpenStartScreen();
                    confirmed = true;
                    GLFW.SetWindowShouldClose(window, GLFW.GLFW_TRUE);
                }
            },
        };

        var title = "";
        var exitCode = 0;
        var previous = GLFW.GetTime();
        var editsApplied = false;
        var nextCommand = 0;
        var commandResumeAt = 0.0;
        var savedOnce = false;
        while (!controls.CloseRequested)
        {
            if (GLFW.WindowShouldClose(window) != 0)
            {
                if (confirmed || !project || !session.Unsaved)
                    break;
                // The scene has changes: ask first.
                GLFW.SetWindowShouldClose(window, GLFW.GLFW_FALSE);
                leaving = Leaving.Close;
                askToSave = true;
            }

            GLFW.PollEvents();
            var now = GLFW.GetTime();
            var frameDelta = now - previous;
            previous = now;
            BeginFrame();

            viewport.CloseColliderGestureBeforeCommands();
            if (inspector.RebuildRequested)
            {
                inspector.RebuildRequested = false;
                session.Reset();
            }

            var io = ImGui.GetIO();
            if (project && io.KeyCtrl && ImGui.IsKeyPressed(ImGuiKey.S, false))
                Save(session);
            // The editing shortcuts, unless a text field has the keyboard.
            if (!io.WantTextInput)
            {
                if (io.KeyCtrl && !io.KeyShift && ImGui.IsKeyPressed(ImGuiKey.Z, false))
                    authoring.Undo();
                if (io.KeyCtrl && (ImGui.IsKeyPressed(ImGuiKey.Y, false) || (io.KeyShift && ImGui.IsKeyPressed(ImGuiKey.Z, false))))
                    authoring.Redo();
                if (io.KeyCtrl && ImGui.IsKeyPressed(ImGuiKey.D, false))
                    authoring.Duplicate([.. selection.Items]);
                if (ImGui.IsKeyPressed(ImGuiKey.Delete, false))
                    authoring.Delete([.. selection.Items]);
            }

            // A rebuild, a deletion in the viewport, play, stop, or the game itself may have taken
            // entities away.
            selection.Prune(session);

            var dockspace = ImGui.GetID("dockspace");
            if (firstLayout)
            {
                DockDefaultLayout(dockspace);
                firstLayout = false;
            }

            toolbar.Draw();
            ImGui.DockSpaceOverViewport(dockspace);
            controls.Draw();
            entities.Draw();
            inspector.Draw(selection.Items);
            viewport.Draw(frameDelta);
            files.Draw();
#if PIXIE_DEVELOPMENT
            exportPanel.Draw();
            if (exportPanel.Failed) exitCode = 3;
#endif

            if (askToSave)
            {
                ImGui.OpenPopup(UnsavedPopup);
                askToSave = false;
            }

            if (DrawUnsavedPrompt(session) is { } leave)
            {
                if (leave)
                {
                    if (leaving == Leaving.OtherProject)
                        OpenStartScreen();
                    confirmed = true;
                    GLFW.SetWindowShouldClose(window, GLFW.GLFW_TRUE);
                }

                if (leaving != Leaving.Reopen || !leave)
                    leaving = Leaving.None;
            }

            // Code is built by a new editor, once this one has let go of the game's library.
            if (leaving == Leaving.Reopen && confirmed)
            {
                Relaunch(["--project", session.ProjectFolder!]);
                GLFW.SetWindowShouldClose(window, GLFW.GLFW_TRUE);
                leaving = Leaving.None;
            }

            // The command line's edits go through the inspectors, once they exist; then its commands,
            // one per frame.
            if (!editsApplied && now - Start >= options.EditAfter)
            {
                exitCode = ApplyEdits(inspector) ? exitCode : 3;
                editsApplied = true;
            }
            else if (editsApplied && nextCommand < options.Commands.Count && now >= commandResumeAt)
            {
                var command = options.Commands[nextCommand++];
                if (command.StartsWith("wait=", StringComparison.Ordinal)
                    && double.TryParse(command[5..], System.Globalization.NumberStyles.Float, System.Globalization.CultureInfo.InvariantCulture, out var seconds)
                    && double.IsFinite(seconds) && seconds >= 0 && seconds <= 3600)
                    commandResumeAt = now + seconds;
                else
                    exitCode = RunCommand(command, session, authoring, selection, entities, viewport, inspector, startExport) ? exitCode : 3;
            }

            // An inspector edit is one step, from the first write until no control is held.
            if (!ImGui.IsAnyItemActive())
                recorder.Close();

            if (project && options.SaveAfter is { } saveAfter && !savedOnce && now - Start >= saveAfter)
            {
                Save(session);
                savedOnce = true;
            }

            var name = project ? session.ProjectName : gameName;
            var current = $"PixieEditor - {name}{(project && session.Unsaved ? " *" : "")}{(project && !session.Editing ? " - playing" : "")}";
            if (current != title)
            {
                GLFW.SetWindowTitle(window, current);
                title = current;
            }

            var exportInProgress = exporting();
#if PIXIE_DEVELOPMENT
            if (!exportInProgress && exportPanel.Failed) exitCode = 3;
#endif
            if (options.QuitAfter > 0 && now - Start >= options.QuitAfter && !exportInProgress)
            {
                var stats = session.Stats;
                Console.Error.WriteLine(string.Create(System.Globalization.CultureInfo.InvariantCulture,
                    $"[info] Simulated {stats.Elapsed:F3} s in {stats.Steps} steps over {now - Start:F3} s of wall time, at {stats.TimeScale:0.##}x"));
                confirmed = true;
            }

            EndFrame(window, now, timed: !exportInProgress);
        }

        return exitCode;
    }

    private static void Save(EngineSession session)
    {
        try
        {
            session.Save();
            Status.Show($"Saved {session.ProjectName}");
        }
        catch (EngineException e)
        {
            Status.Show(e.Message, error: true);
        }
    }

    // A new editor at the start screen; this one closes.
    private static void OpenStartScreen() => Relaunch([]);

    // A new editor with these options, which waits for this one to exit; this one closes.
    private static void Relaunch(string[] arguments) =>
        Process.Start(Environment.ProcessPath!, [.. arguments, "--after", Environment.ProcessId.ToString(System.Globalization.CultureInfo.InvariantCulture)]);

    // Asks what to do with unsaved changes; true to go on leaving, false to stay, null while open.
    private static bool? DrawUnsavedPrompt(EngineSession session)
    {
        bool? answer = null;
        if (!ImGui.BeginPopupModal(UnsavedPopup, ImGuiWindowFlags.AlwaysAutoResize))
            return null;

        ImGui.TextUnformatted($"{session.ProjectName} has changes that are not saved.");
        ImGui.Spacing();
        if (ImGui.Button("Save", new System.Numerics.Vector2(120, 0)))
        {
            Save(session);
            answer = !session.Unsaved;
            if (answer == false)
                answer = null;
        }

        ImGui.SameLine();
        if (ImGui.Button("Don't save", new System.Numerics.Vector2(120, 0)))
            answer = true;
        ImGui.SameLine();
        if (ImGui.Button("Cancel", new System.Numerics.Vector2(120, 0)) || ImGui.IsKeyPressed(ImGuiKey.Escape))
            answer = false;
        if (answer != null)
            ImGui.CloseCurrentPopup();
        ImGui.EndPopup();
        return answer;
    }

    private static void BeginFrame()
    {
        ImGuiImplOpenGL3.NewFrame();
        ImGuiImplGLFW.NewFrame();
        ImGui.NewFrame();
    }

    // Draws the frame; with --quit-after, saves the screenshot and closes once the time is up, except
    // while a project's code builds (`timed` false): the times count from when the project opens.
    private unsafe void EndFrame(GLFWwindowPtr window, double now, bool timed = true)
    {
        ImGui.Render();
        ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());

        var quitting = timed && options.QuitAfter > 0 && now - Start >= options.QuitAfter;
        if (quitting && options.Screenshot != null)
        {
            int width, height;
            GLFW.GetFramebufferSize(window, &width, &height);
            Screenshot.Save(options.Screenshot, width, height);
            Console.Error.WriteLine($"[info] Screenshot: {options.Screenshot} ({width}x{height})");
        }

        GLFW.SwapBuffers(window);
        if (quitting)
            GLFW.SetWindowShouldClose(window, GLFW.GLFW_TRUE);
    }

    // One command of --do, as the editor's own gestures would run it; the state is logged after it.
    private static bool RunCommand(string command, EngineSession session, Authoring authoring, Selection selection,
        EntitiesPanel entities, ViewportPanel viewport, InspectorPanel inspector, Action<string>? export = null)
    {
        var equals = command.IndexOf('=');
        var verb = equals < 0 ? command : command[..equals].TrimEnd('+');
        var adding = equals > 0 && command[equals - 1] == '+';
        var argument = equals < 0 ? "" : command[(equals + 1)..];
        var chosen = selection.Items.ToArray();
        var ok = true;
        switch (verb)
        {
#if PIXIE_DEVELOPMENT
            case "export":
                if (export == null || session.ProjectFolder == null || !session.Editing || session.Unsaved || string.IsNullOrWhiteSpace(argument))
                    ok = false;
                else export(argument);
                break;
#endif
            case "create":
                ok = authoring.CreateEntity() != null;
                break;
            case "collider":
                ok = authoring.CreateMovementEntity(false) != null;
                break;
            case "character":
                ok = authoring.CreateMovementEntity(true) != null;
                break;
            case "compound":
                ok = authoring.MakeCompound(chosen);
                break;
            case "addshape":
            case "removeshape":
                if (chosen.Length == 1 && session.TypeIndex("CompoundCollider2D") is var compoundType and >= 0 && session.Has(chosen[0], compoundType))
                {
                    var list = session.FieldIndex(compoundType, "shapes");
                    var index = verb == "addshape" ? session.RecordInfo(chosen[0], compoundType, list).Count : int.Parse(argument, System.Globalization.CultureInfo.InvariantCulture);
                    authoring.EditRecord(chosen[0], compoundType, list, index, remove: verb == "removeshape");
                }
                else ok = false;
                break;
            case "shape":
                var shapeParts = argument.Split(':', 3);
                ok = shapeParts.Length == 3 && int.TryParse(shapeParts[0], out var shapeIndex) && inspector.SetShape(shapeIndex, shapeParts[1], shapeParts[2]);
                break;
            case "layername":
                var nameParts = argument.Split(':', 2);
                ok = nameParts.Length == 2 && int.TryParse(nameParts[0], out var layerSlot) && inspector.RenameLayer(layerSlot, nameParts[1]);
                break;
            case "layer":
                var layerParts = argument.Split(':', 3);
                ok = layerParts.Length == 3 && int.TryParse(layerParts[1], out var slot) && bool.TryParse(layerParts[2], out var enabled)
                    && inspector.SetLayer("Collider2D", layerParts[0], slot, enabled);
                break;
            case "shapelayer":
                var shapeLayerParts = argument.Split(':', 4);
                ok = shapeLayerParts.Length == 4 && int.TryParse(shapeLayerParts[0], out var itemIndex)
                    && int.TryParse(shapeLayerParts[2], out var itemSlot) && bool.TryParse(shapeLayerParts[3], out var itemEnabled)
                    && inspector.SetLayer("Collider2D", shapeLayerParts[1], itemSlot, itemEnabled, itemIndex);
                break;
            case "dynamic":
            case "compoundbox":
            case "kinematic":
            case "sensor":
            case "world":
                var preset = verb switch
                {
                    "dynamic" => PhysicsPreset.Dynamic, "compoundbox" => PhysicsPreset.Compound, "kinematic" => PhysicsPreset.Kinematic,
                    "sensor" => PhysicsPreset.Sensor, _ => PhysicsPreset.World
                };
                ok = authoring.CreatePhysicsEntity(preset) != null;
                break;
            case "duplicate":
                authoring.Duplicate(chosen);
                break;
            case "delete":
                authoring.Delete(chosen);
                break;
            case "undo":
                authoring.Undo();
                break;
            case "redo":
                authoring.Redo();
                break;
            case "select" when entities.Find(argument) is { } found:
                if (adding)
                    selection.Add([found]);
                else
                    selection.Set(found);
                break;
            case "add" when chosen.Length > 0 && session.TypeIndex(argument) is var added and >= 0:
                authoring.AddComponent(chosen, added);
                break;
            case "font" when chosen.Length > 0 && session.TypeIndex("Text") is var textType and >= 0
                              && chosen.All(e => session.Has(e, textType)):
                if (authoring.ImportAsset(argument, "ttf") is { } fontPath)
                    authoring.SetText(chosen, textType, session.FieldIndex(textType, "font"), fontPath);
                else
                    ok = false;
                break;
            case "remove" when chosen.Length > 0 && session.TypeIndex(argument) is var removed and >= 0:
                authoring.Change(chosen, $"removal of {argument}", e => session.RemoveComponent(e, removed));
                break;
            case "save":
                session.Save();
                break;
            case "rename" when chosen.Length == 1:
                authoring.Rename(chosen[0], argument);
                break;
            case "drop" when argument.Split('@') is [var files, var point] && point.Split(',') is [var px, var py]
                              && double.TryParse(px, System.Globalization.CultureInfo.InvariantCulture, out var vx)
                              && double.TryParse(py, System.Globalization.CultureInfo.InvariantCulture, out var vy):
                ok = viewport.Drop(files.Split(';'), new System.Numerics.Vector2((float)vx, (float)vy)).Length > 0;
                break;
            case "image" when chosen.Length > 0 && session.TypeIndex("Sprite") is var spriteType and >= 0
                               && chosen.All(e => session.Has(e, spriteType)):
                if (authoring.ImportAsset(argument, "png") is { } imagePath)
                    authoring.SetText(chosen, spriteType, session.FieldIndex(spriteType, "image"), imagePath);
                else
                    ok = false;
                break;
            case "addpoint" when chosen.Length > 0 && FieldOf(session, argument) is { } list:
                authoring.AddPoint(chosen, list.Type, list.Field);
                break;
            case "removepoint" when chosen.Length > 0 && argument.Split('@') is [var listName, var indexText]
                                     && FieldOf(session, listName) is { } named
                                     && int.TryParse(indexText, System.Globalization.CultureInfo.InvariantCulture, out var pointIndex):
                authoring.RemovePoint(chosen, named.Type, named.Field, pointIndex);
                break;
            case "move" when chosen.Length > 0 && argument.Split(',') is [var x, var y]
                             && double.TryParse(x, System.Globalization.CultureInfo.InvariantCulture, out var dx)
                             && double.TryParse(y, System.Globalization.CultureInfo.InvariantCulture, out var dy):
                if (authoring.BeginMove(chosen, chosen[0]) is { } move)
                {
                    move.By(dx, dy);
                    authoring.EndMove(move);
                }
                break;
            case "colliderhandles":
                viewport.ColliderTools = true;
                break;
            case "physicsdebug" when uint.TryParse(argument, out var debugFlags) && debugFlags <= 127:
                session.PhysicsDebug = debugFlags;
                break;
            case "colliderdrag" when chosen.Length == 1 && argument.Split(':') is [var shapeText, var operation, var target]
                && int.TryParse(shapeText, out var colliderShapeIndex) && target.Split(',') is [var targetX, var targetY]
                && double.TryParse(targetX, System.Globalization.CultureInfo.InvariantCulture, out var worldX)
                && double.TryParse(targetY, System.Globalization.CultureInfo.InvariantCulture, out var worldY)
                && operation is "resize" or "rotate" or "height" or "cancel" or "cancelheight":
                viewport.ColliderTools = true;
                viewport.ColliderShape = Math.Max(0, colliderShapeIndex);
                if (authoring.BeginColliderDrag(chosen[0], colliderShapeIndex,
                    operation == "rotate" ? 1 : operation is "height" or "cancelheight" ? 2 : 0) is { } colliderGesture)
                {
                    authoring.UpdateColliderDrag(colliderGesture, worldX, worldY, snap: false);
                    authoring.EndColliderDrag(colliderGesture, cancel: operation is "cancel" or "cancelheight");
                }
                break;
            case "play":
                selection.PreserveThrough(session, session.Play);
                viewport.Mode = ViewportMode.Game;
                break;
            case "stop":
                selection.PreserveThrough(session, session.Stop);
                viewport.Mode = ViewportMode.Select;
                break;
            default:
                ok = false;
                break;
        }

        selection.Prune(session);
        Console.Error.WriteLine($"[{(ok ? "info" : "error")}] After {command}: {Describe(session, authoring, selection)}");
        return ok;
    }

    // "Type.field" as a type and a field index; null when either is unknown.
    private static (int Type, int Field)? FieldOf(EngineSession session, string name)
    {
        var dot = name.IndexOf('.');
        if (dot < 0 || session.TypeIndex(name[..dot]) is not (var type and >= 0))
            return null;
        try
        {
            return (type, session.FieldIndex(type, name[(dot + 1)..]));
        }
        catch (EngineException)
        {
            return null;
        }
    }

    // Entities, the selection with names, positions, images, texts, and path points, and what undo and
    // redo would do.
    private static string Describe(EngineSession session, Authoring authoring, Selection selection)
    {
        var transform = session.TypeIndex("Transform2D");
        var sprite = session.TypeIndex("Sprite");
        var path = session.TypeIndex("Path");
        var text = session.TypeIndex("Text");
        var chosen = selection.Items.Select(e =>
        {
            var name = authoring.Describe(e);
            if (text >= 0 && session.Has(e, text))
                name += $" \"{session.GetText(e, text, session.FieldIndex(text, "text"))}\" ({session.GetText(e, text, session.FieldIndex(text, "font"))})";
            if (sprite >= 0 && session.Has(e, sprite))
                name += $" [{session.GetText(e, sprite, session.FieldIndex(sprite, "image"))}]";
            if (path >= 0 && session.Has(e, path))
                name += " <" + string.Join("; ", session.GetPoints(e, path, session.FieldIndex(path, "points"))
                    .Select(p => string.Create(System.Globalization.CultureInfo.InvariantCulture, $"{p.X:0.##},{p.Y:0.##}"))) + ">";
            if (transform < 0 || !session.Has(e, transform))
                return name;
            var at = session.Get(e, transform, session.FieldIndex(transform, "position"));
            return string.Create(System.Globalization.CultureInfo.InvariantCulture, $"{name} ({at.X:0.##}, {at.Y:0.##})");
        });
        return $"{session.Stats.Entities} entities; selected: [{string.Join(", ", chosen)}]; undo: {authoring.History.UndoName ?? "-"}; redo: {authoring.History.RedoName ?? "-"}; unsaved: {(session.Unsaved ? "yes" : "no")}";
    }

    private bool ApplyEdits(InspectorPanel inspector)
    {
        var ok = true;
        foreach (var (type, property, value) in options.Edits)
        {
            try
            {
                if (inspector.Set(type, property, value))
                {
                    Console.Error.WriteLine($"[info] Set {type}.{property} = {value} through the inspector");
                    continue;
                }

                Console.Error.WriteLine($"[error] The selected entity has no {type}.{property} to set");
            }
            catch (Exception e)
            {
                Console.Error.WriteLine($"[error] Setting {type}.{property}: {e.Message}");
            }

            ok = false;
        }

        return ok;
    }

    // Session above Entities on the left, the Inspector on the right, the Viewport in the middle.
    private static unsafe void DockDefaultLayout(uint dockspace)
    {
        ImGuiP.DockBuilderRemoveNode(dockspace);
        ImGuiP.DockBuilderAddNode(dockspace, (ImGuiDockNodeFlags)ImGuiDockNodeFlagsPrivate.Space);
        ImGuiP.DockBuilderSetNodeSize(dockspace, ImGui.GetMainViewport().Size);

        uint left, rest, right, middle, top, bottom;
        ImGuiP.DockBuilderSplitNode(dockspace, ImGuiDir.Left, 0.2f, &left, &rest);
        ImGuiP.DockBuilderSplitNode(rest, ImGuiDir.Right, 0.3f, &right, &middle);
        ImGuiP.DockBuilderSplitNode(left, ImGuiDir.Up, 0.3f, &top, &bottom);
        ImGuiP.DockBuilderDockWindow("Session", top);
        ImGuiP.DockBuilderDockWindow("Entities", bottom);
        ImGuiP.DockBuilderDockWindow("Inspector", right);
        ImGuiP.DockBuilderDockWindow("Viewport", middle);
        ImGuiP.DockBuilderFinish(dockspace);
    }
}
