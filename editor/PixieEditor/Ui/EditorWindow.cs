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
        using var inspector = new InspectorPanel(session);
        var selection = new Selection();
        var viewport = new ViewportPanel(session, selection).Started();
        viewport.Mode = options.Mode ?? (project ? ViewportMode.Select : ViewportMode.Game);
        var entities = new EntitiesPanel(session, selection);
        var controls = new SessionPanel(session, gameName);
        if (options.Select != null && entities.Find(options.Select) is { } chosen)
            selection.Set(chosen);

        if (options.TimeScale is { } scale)
            session.TimeScale = scale;
        if (options.Paused)
            session.Paused = true;

        var leaving = Leaving.None;
        var askToSave = false;
        var confirmed = false;
        var toolbar = new Toolbar(session, viewport)
        {
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

            if (inspector.RebuildRequested)
            {
                inspector.RebuildRequested = false;
                session.Reset();
            }

            var io = ImGui.GetIO();
            if (project && io.KeyCtrl && ImGui.IsKeyPressed(ImGuiKey.S, false))
                Save(session);

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

            // The command line's edits go through the inspectors, once they exist.
            if (!editsApplied && now - Start >= options.EditAfter)
            {
                exitCode = ApplyEdits(inspector) ? exitCode : 3;
                editsApplied = true;
            }

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

            if (options.QuitAfter > 0 && now - Start >= options.QuitAfter)
            {
                var stats = session.Stats;
                Console.Error.WriteLine(string.Create(System.Globalization.CultureInfo.InvariantCulture,
                    $"[info] Simulated {stats.Elapsed:F3} s in {stats.Steps} steps over {now - Start:F3} s of wall time, at {stats.TimeScale:0.##}x"));
                confirmed = true;
            }

            EndFrame(window, now);
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
