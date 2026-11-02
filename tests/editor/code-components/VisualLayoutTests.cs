using System.Numerics;
using System.Reflection;
using System.Text.Json;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Editing;
using PixieEditor.Native;
using PixieEditor.Ui;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

internal static class VisualLayoutTests
{
    internal static unsafe int Run(string native, string work)
    {
        Directory.CreateDirectory(work);
        var checks = new List<string>();
        void Check(bool ok, string name) { if (!ok) throw new Exception(name); checks.Add(name); Console.WriteLine("PASS " + name); }
        Check(!SessionPanel.IsProjectModule("pixie.dll") && !SessionPanel.IsProjectModule("libpixie.so"), "built-in-engine-is-not-presented-as-loaded-project-code");
        Check(SessionPanel.IsProjectModule("Game-module.dll") && SessionPanel.IsProjectModule("libGame-module.so"), "project-module-status-recognizes-module-names");
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(1280, 900, "Visual layout qualification", default, default);
        if (window.Handle == null) return 4;
        GLFW.MakeContextCurrent(window); Gl.Load(); GameLibrary.Use(Path.GetFullPath(native));
        var clear = (delegate* unmanaged[Stdcall]<uint, void>)GLFW.GetProcAddress("glClear");
        var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        var io = ImGui.GetIO(); io.IniFilename = (byte*)null;
        ImGui.StyleColorsDark(); ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context); ImGuiImplOpenGL3.Init("#version 450");
        try
        {
            var folder = Path.Combine(work, "scene-" + Guid.NewGuid().ToString("N")); EngineSession.CreateProject(folder);
            using var session = EngineSession.OpenProject(900, 650, folder);
            var entity = session.Entities()[0]; var id = session.SceneId(entity)!.Value;
            var selection = new Selection(); selection.Set(entity); var history = new History();
            var authoring = new Authoring(session, selection, history);
            var viewport = new ViewportPanel(session, selection, authoring, new AssetChooser(folder)).Started();
            viewport.Mode = ViewportMode.Select;
            var toolbar = new Toolbar(session, viewport, authoring, selection);
            var sessionPanel = new SessionPanel(session, "scene"); var entitiesPanel = new EntitiesPanel(session, selection, authoring);
            Vector2 controlsStart = default, controlsEnd = default;
            viewport.DrawControls = () => { controlsStart = ImGui.GetCursorScreenPos(); toolbar.DrawViewportControls(); controlsEnd = ImGui.GetCursorScreenPos(); };
            var viewportSize = new Vector2(940, 820);
            var framebufferScale = Vector2.One;
            T Read<T>(string field) => (T)typeof(ViewportPanel).GetField(field, BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(viewport)!;
            void Frame(Action? input = null)
            {
                GLFW.PollEvents(); ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame();
                io.DisplayFramebufferScale = framebufferScale; input?.Invoke(); ImGui.NewFrame();
                toolbar.Draw();
                ImGui.SetNextWindowPos(new Vector2(0, 22)); ImGui.SetNextWindowSize(new Vector2(270, 390)); sessionPanel.Draw();
                ImGui.SetNextWindowPos(new Vector2(0, 415)); ImGui.SetNextWindowSize(new Vector2(270, 420)); entitiesPanel.Draw();
                ImGui.SetNextWindowPos(new Vector2(280, 22)); ImGui.SetNextWindowSize(viewportSize); viewport.Draw(0.016);
                ImGui.Render(); clear(0x00004000); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
            }
            void Click(Vector2 point)
            {
                Frame(() => io.AddMousePosEvent(point.X, point.Y));
                Frame(() => io.AddMouseButtonEvent(0, true));
                Frame(() => io.AddMouseButtonEvent(0, false)); Frame();
            }
            Frame(); Frame(); Frame();
            var wideHeight = controlsEnd.Y - controlsStart.Y;
            Check(wideHeight > ImGui.GetFrameHeight() * 2 && Read<Vector2>("ImageOrigin").Y >= controlsEnd.Y, "viewport-toolbar-reserves-space-before-image-and-input-origin");
            Check(Read<int>("Height") > 600 && Read<int>("Width") > 850, "image-retains-usable-native-render-size");
            var camera = session.ViewToWorld(0, 0); var snapshot = session.Snapshot(session.Entities());
            Click(controlsStart + new Vector2(viewportSize.X - 20, 10));
            Check(selection.Items.Count == 1 && session.ViewToWorld(0, 0) == camera && session.Snapshot(session.Entities()) == snapshot, "toolbar-click-does-not-select-move-or-pan-the-scene");
            var origin = Read<Vector2>("ImageOrigin");
            Click(origin + new Vector2(25, 25));
            Check(selection.Items.Count == 0, "image-empty-click-still-clears-selection-after-toolbar-offset");
            var point = session.WorldToView(0, 0);
            Click(Read<Vector2>("ImageOrigin") + new Vector2((float)point.X, (float)point.Y));
            Check(selection.Items.Count == 1 && session.SceneId(selection.Items[0]) == id, "image-picking-matches-native-world-projection-after-toolbar-offset");
            framebufferScale = new Vector2(2); Frame(); Frame();
            Check(Read<int>("Width") > 1700, "synthetic-two-times-framebuffer-uses-scaled-native-image-size");
            selection.Set([]); point = session.WorldToView(0, 0);
            Click(Read<Vector2>("ImageOrigin") + new Vector2((float)point.X / 2, (float)point.Y / 2));
            Check(selection.Items.Count == 1 && session.SceneId(selection.Items[0]) == id, "synthetic-two-times-framebuffer-picking-respects-image-origin-and-scale");
            framebufferScale = Vector2.One; Frame(); Frame();
            var playWidth = ImGui.CalcTextSize("  Play  ").X + ImGui.GetStyle().FramePadding.X * 2;
            Vector2 Button(float x) => controlsStart + new Vector2(10 + x, 7 + ImGui.GetFrameHeight() / 2);
            Click(Button(playWidth / 2));
            Check(!session.Editing && viewport.Mode == ViewportMode.Game && session.SceneId(selection.Items[0]) == id, "viewport-play-preserves-selection-and-enters-game-input");
            Click(Button(playWidth + ImGui.GetStyle().ItemSpacing.X + 35));
            Check(session.Paused, "viewport-pause-routes-to-current-session");
            var steps = session.Stats.Steps;
            Click(Button(playWidth + 70 + ImGui.GetStyle().ItemSpacing.X * 2 + 20));
            Check(session.Stats.Steps == steps + 1, "viewport-step-advances-one-paused-fixed-step");
            Click(Button(playWidth / 2));
            Check(session.Editing && viewport.Mode == ViewportMode.Select && session.SceneId(selection.Items[0]) == id, "viewport-stop-restores-edit-mode-and-selected-scene-id");
            viewportSize.X = 320; Frame(); Frame(); Frame();
            Check(controlsEnd.Y - controlsStart.Y > wideHeight && Read<Vector2>("ImageOrigin").Y >= controlsEnd.Y, "narrow-viewport-wraps-controls-without-covering-image");
            Check(Read<int>("Width") > 280 && Read<int>("Height") > 500, "narrow-viewport-remains-renderable");
            Frame(() => io.AddMousePosEvent(1250, 880)); Frame();
            Screenshot.Save(Path.Combine(work, "narrow.png"), 1280, 900);
            viewportSize.X = 940; Frame(); Frame(); Frame();
            Check(Math.Abs(controlsEnd.Y - controlsStart.Y - wideHeight) < 2, "widening-viewport-recovers-compact-toolbar-height");
            Click(new Vector2(48, 460));
            Check(ImGui.IsPopupOpen((byte*)null, ImGuiPopupFlags.AnyPopupId | ImGuiPopupFlags.AnyPopupLevel), "entities-create-opens-one-explicit-preset-menu");
            Screenshot.Save(Path.Combine(work, "presets.png"), 1280, 900);
            Frame(() => io.AddKeyEvent(ImGuiKey.Escape, true)); Frame(() => io.AddKeyEvent(ImGuiKey.Escape, false));
            Check(session.Entities().Length == 1 && !session.Unsaved, "layout-navigation-does-not-change-or-save-authored-scene");
            File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks }, new JsonSerializerOptions { WriteIndented = true }));
            return 0;
        }
        finally
        {
            Console.WriteLine("Cleanup: renderer backend"); ImGuiImplOpenGL3.Shutdown();
            Console.WriteLine("Cleanup: window backend"); ImGuiImplGLFW.Shutdown();
            ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            Console.WriteLine("Cleanup: UI context"); ImGui.DestroyContext(context);
            Console.WriteLine("Cleanup: native window"); GLFW.DestroyWindow(window); GLFW.Terminate();
            Console.WriteLine("Cleanup: complete");
        }
    }
}
