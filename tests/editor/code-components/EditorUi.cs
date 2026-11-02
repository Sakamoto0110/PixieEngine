using System.Numerics;
using System.Reflection;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Editing;
using PixieEditor.Native;
using PixieEditor.Ui;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

internal static class EditorUi
{
    internal static unsafe void Run(Hexa.NET.GLFW.GLFWwindowPtr window, EngineSession session, uint[] entities, string folder, Action<bool, string> check)
    {
        GLFW.SetWindowSize(window, 1280, 1600); GLFW.PollEvents();
        Gl.Load(); var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        var io = ImGui.GetIO(); io.IniFilename = (byte*)null;
        ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context); ImGuiImplOpenGL3.Init("#version 450");
        var selection = new Selection(); selection.Set(entities); var history = new History();
        var authoring = new Authoring(session, selection, history); var recorder = new EditRecorder(session, history, () => authoring.Recording);
        using var panel = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
        panel.SynchronizeChannel(selection.Items, session.DevelopmentGeneration);
        var draw = typeof(InspectorPanel).GetMethod("DrawAddComponent", BindingFlags.Instance | BindingFlags.NonPublic)!;
        Vector2 add = default;
        void Frame(Action? input = null)
        {
            ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); input?.Invoke(); ImGui.NewFrame();
            ImGui.SetNextWindowPos(new Vector2(10, 10)); ImGui.SetNextWindowSize(new Vector2(600, 1400));
            ImGui.Begin("Component creation fixture"); draw.Invoke(panel, null);
            if (!ImGui.IsPopupOpen("add-component")) add = (ImGui.GetItemRectMin() + ImGui.GetItemRectMax()) / 2;
            ImGui.End(); ImGui.Render(); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
        }
        void Click(Vector2 position)
        {
            Frame(() => io.AddMousePosEvent(position.X, position.Y));
            Frame(() => io.AddMouseButtonEvent(0, true));
            Frame(() => io.AddMouseButtonEvent(0, false)); Frame();
        }
        try
        {
            Frame(); Frame(); Click(add); Frame();
            Screenshot.Save(Path.Combine(folder, "add-component-popup.png"), 1280, 1600);
            Console.WriteLine("UI popup captured; add button=" + add);
            check(ImGui.GetDrawData().TotalVtxCount > 0, "production-add-component-popup-renders-native-ui");
            var rows = Enumerable.Range(0, session.TypeCount).Count(type => !session.IsValueType(type) && !entities.All(entity => session.Has(entity, type)));
            var nameAt = add + new Vector2(100, (rows + 2) * ImGui.GetTextLineHeightWithSpacing() + 4);
            Click(nameAt);
            var name = "FromUi" + Guid.NewGuid().ToString("N")[..8];
            Frame(() => io.AddInputCharactersUTF8(name));
            var entered = (string)typeof(InspectorPanel).GetField("NewCodeName", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(panel)!;
            Screenshot.Save(Path.Combine(folder, "component-name-entered.png"), 1280, 1600);
            check(entered == name, "actual-popup-keyboard-input-reaches-component-name");
            Click(nameAt + new Vector2(0, ImGui.GetFrameHeightWithSpacing()));
            var request = panel.TakeCreatedCodeComponent();
            check(request != null && request.Name == name && request.Targets.SequenceEqual(entities.Select(session.SceneId).OfType<ulong>()),
                "actual-create-component-button-generates-reusable-code-for-stable-selection");
            check(File.Exists(ProjectCode.ResolveSource(folder, request!.Source)), "ui-created-component-declares-an-existing-user-implementation");
            var handoff = CodeReopen.Capture(session, entities, request);
            File.WriteAllText(Path.Combine(folder, "ui-handoff-path.txt"), handoff);
            // Exercise the production editor routing against the real C API, not a duplicate sink.
            var viewport = new ViewportPanel(session, selection, authoring, new AssetChooser(folder)).Started();
            void Set(string field, object value) => typeof(ViewportPanel).GetField(field, BindingFlags.Instance | BindingFlags.NonPublic)!.SetValue(viewport, value);
            Set("Focused", true); Set("Hovered", true); Set("Width", 100); Set("Height", 100);
            Set("ImageOrigin", new Vector2(10, 20)); Set("Scale", new Vector2(2));
            io.MousePos = new Vector2(30, 50);
            session.InputEvent(new() { Kind = 4 }); session.Play(); session.Paused = true;
            KeyEvents.Pending.Enqueue(new() { Kind = 2, X = 30, Y = 50 });
            foreach (var action in new[] { 1, 0, 1, 2 }) KeyEvents.Pending.Enqueue(new() { Kind = 0, Code = 65, Action = action, ScanCode = 30, Modifiers = 1 });
            KeyEvents.Pending.Enqueue(new() { Kind = 1, Code = 1, Action = 1, X = 30, Y = 50, Modifiers = 2 });
            KeyEvents.Pending.Enqueue(new() { Kind = 1, Code = 1, Action = 0, X = 30, Y = 50, Modifiers = 2 });
            KeyEvents.Pending.Enqueue(new() { Kind = 3, Y = 2 });
            var send = typeof(ViewportPanel).GetMethod("SendInput", BindingFlags.Instance | BindingFlags.NonPublic)!;
            send.Invoke(viewport, null); session.Step();
            var user = session.TypeIndex("UserLogic"); var keys = session.FieldIndex(user, "keyEvents");
            var current = session.Find(request.Targets[0])!.Value;
            check(session.Get(current, user, keys).X == 4 && session.Get(current, user, session.FieldIndex(user, "buttonEvents")).X == 2,
                "production-viewport-queue-preserves-fast-transitions-and-repeat");
            Set("Focused", false); KeyEvents.Pending.Enqueue(new() { Kind = 0, Code = 65, Action = 2 });
            send.Invoke(viewport, null); session.Step();
            check(session.Get(current, user, keys).X == 5, "viewport-capture-loss-refuses-repeat-and-releases-held-key-once");
            session.Stop();
            Screenshot.Save(Path.Combine(folder, "component-ui-completed.png"), 1280, 1600);
        }
        finally
        {
            ImGuiImplOpenGL3.Shutdown(); ImGuiImplGLFW.Shutdown();
            ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            ImGui.DestroyContext(context);
        }
    }
}
