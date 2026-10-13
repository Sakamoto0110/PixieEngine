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
// docking through its GLFW and OpenGL3 backends, and the four panels. One game per window.
internal sealed class EditorWindow(EditorOptions options)
{
    public unsafe int Run()
    {
        if (GLFW.Init() == 0)
            throw new InvalidOperationException("GLFW could not start.");

        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_FORWARD_COMPAT, GLFW.GLFW_TRUE);

        var window = GLFW.CreateWindow(1600, 900, $"PixieEditor - {options.Game}", default(GLFWmonitorPtr), default(GLFWwindowPtr));
        if (window.Handle == null)
        {
            GLFW.Terminate();
            throw new InvalidOperationException("GLFW could not open a window with an OpenGL 4.5 core context.");
        }

        GLFW.MakeContextCurrent(window);
        GLFW.SwapInterval(1);
        Gl.Load();

        var context = ImGui.CreateContext();
        ImGui.SetCurrentContext(context);
        var firstLayout = !File.Exists("imgui.ini");
        var io = ImGui.GetIO();
        // No keyboard navigation: the arrows belong to the game while the viewport has focus.
        io.ConfigFlags |= ImGuiConfigFlags.DockingEnable;
        KeyEvents.Install(window);
        ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context);
        ImGuiImplOpenGL3.Init("#version 450");

        var exitCode = 0;
        using (var session = EngineSession.Create(800, 600, options.Seed))
        using (var inspector = new InspectorPanel(session))
        {
            var viewport = new ViewportPanel(session).Started();
            var entities = new EntitiesPanel(session);
            var controls = new SessionPanel(session, options.Game);
            if (options.Select != null)
                entities.Selected = entities.Find(options.Select);

            var previous = GLFW.GetTime();
            var start = previous;
            var editsApplied = false;
            while (GLFW.WindowShouldClose(window) == 0 && !controls.CloseRequested)
            {
                GLFW.PollEvents();
                var now = GLFW.GetTime();
                var frameDelta = now - previous;
                previous = now;

                ImGuiImplOpenGL3.NewFrame();
                ImGuiImplGLFW.NewFrame();
                ImGui.NewFrame();

                var dockspace = ImGui.GetID("dockspace");
                if (firstLayout)
                {
                    DockDefaultLayout(dockspace);
                    firstLayout = false;
                }

                ImGui.DockSpaceOverViewport(dockspace);
                controls.Draw();
                entities.Draw();
                inspector.Draw(entities.Selected);
                viewport.Draw(frameDelta);

                // The command line's edits go through the inspectors, once they exist.
                if (!editsApplied && now - start >= options.EditAfter)
                {
                    exitCode = ApplyEdits(inspector) ? exitCode : 3;
                    editsApplied = true;
                }

                ImGui.Render();
                ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());

                var quitting = options.QuitAfter > 0 && now - start >= options.QuitAfter;
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
