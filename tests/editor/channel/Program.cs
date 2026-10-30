using System.Collections;
using System.Reflection;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Editing;
using PixieEditor.Native;
using PixieEditor.Ui;
using PixieInspector;
using PixieInspector.Options;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

internal static class Program
{
    private static readonly JsonArray Results = [];
    private static void Check(bool condition, string name)
    {
        Results.Add(new JsonObject { ["case"] = name, ["passed"] = condition });
        if (!condition) throw new InvalidOperationException(name);
    }
    // Test-only access to registered bindings; no injection surface is added to the channel.
    private static Inspector Bound(InspectorPanel panel) => ((IEnumerable)typeof(InspectorPanel)
        .GetField("Sections", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(panel)!)
        .Cast<object>().Select(s => (Inspector)s.GetType().GetProperty("Inspector")!.GetValue(s)!)
        .First(i => i.Any(n => n.Path == "positionX"));
    private static unsafe int Main(string[] args)
    {
        if (args.Length != 3) return 2;
        var native = Path.GetFullPath(args[0]);
        var root = Path.GetFullPath(args[1]);
        Directory.CreateDirectory(root);
        PipeQualification.Run(Check).GetAwaiter().GetResult();
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(320, 240, "Inspector channel fault qualification", default, default);
        if (window.Handle == null) return 4;
        GLFW.MakeContextCurrent(window);
        Gl.Load();
        var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context); ImGuiImplOpenGL3.Init("#version 450");
        ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); ImGui.NewFrame();
        GameLibrary.Use(native);
        int exit = 1;
        try
        {
            foreach (var mode in new[] { "readonly", "pending", "partial", "rejected-setter" })
            {
                var folder = Path.Combine(root, mode + "-" + Guid.NewGuid().ToString("N"));
                EngineSession.CreateProject(folder);
                using var session = EngineSession.OpenProject(320, 240, folder);
                var first = session.Entities()[0];
                var second = session.CreateEntity("second");
                var type = session.TypeIndex("Transform2D");
                session.AddComponent(second, type);
                if (mode == "rejected-setter")
                {
                    var probeType = session.TypeIndex("WriteProbe");
                    var probe = session.Entities().First(e => session.Has(e, probeType));
                    var chosen = new Selection(); chosen.Set(probe);
                    var probeHistory = new History();
                    var probeAuthoring = new Authoring(session, chosen, probeHistory);
                    var probeRecorder = new EditRecorder(session, probeHistory, () => probeAuthoring.Recording);
                    using var probePanel = new InspectorPanel(session, probeAuthoring, probeRecorder, new AssetChooser(folder));
                    probePanel.SynchronizeChannel(chosen.Items, session.DevelopmentGeneration);
                    var refusal = probePanel.ChannelSet("WriteProbe.value", JsonValue.Create(101), probePanel.ChannelGeneration);
                    Check(refusal["outcome"]!.GetValue<string>() == "failed_possibly_partial" && refusal["pixie_result"]!.GetValue<int>() == 5,
                        "native-setter-refusal-propagates-original-result");
                    Check(session.Get(probe, probeType, 0).X == 0 && probeHistory.UndoCount == 0,
                        "rejected-native-setter-does-not-create-ghost-history");
                    var locked = probePanel.ChannelSet("WriteProbe.locked", JsonValue.Create(50), probePanel.ChannelGeneration);
                    Check(locked["outcome"]!.GetValue<string>() == "refused" && session.Get(probe, probeType, 0).X == 0 && probeHistory.UndoCount == 0,
                        "native-descriptor-readonly-reaches-inspector-and-refuses-without-history");
                    continue;
                }
                var selection = new Selection(); selection.Set(new[] { first, second });
                var history = new History();
                var authoring = new Authoring(session, selection, history);
                var recorder = new EditRecorder(session, history, () => authoring.Recording);
                using var panel = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
                panel.SynchronizeChannel(selection.Items, session.DevelopmentGeneration);
                var inspector = Bound(panel);
                var node = inspector["positionX"];
                if (mode == "readonly")
                {
                    node.ReadOnly = true;
                    var receipt = panel.ChannelSet("Transform2D.positionX", JsonValue.Create(99), panel.ChannelGeneration);
                    Check(receipt["outcome"]!.GetValue<string>() == "refused", "readonly-refusal-overrides-handler-true");
                    Check(history.UndoCount == 0 && session.Get(first, type, session.FieldIndex(type, "position")).X == 0, "readonly-refusal-does-not-write-or-record");
                }
                else if (mode == "pending")
                {
                    inspector.Options.BinderControl = BinderControlMode.Manual;
                    var staged = panel.ChannelSet("Transform2D.positionX", JsonValue.Create(33), panel.ChannelGeneration);
                    Check(staged["outcome"]!.GetValue<string>() == "accepted_pending" && node.HasPendingValue, "manual-write-is-pending-not-native-success");
                    var refused = panel.ChannelSet("Transform2D.positionX", JsonValue.Create("invalid-number"), panel.ChannelGeneration);
                    Check(refused["outcome"]!.GetValue<string>() == "refused" && refused["exception"] != null && refused["possibly_applied"]!.GetValue<bool>() == false, "new-command-failure-precedes-existing-pending-state");
                    panel.SynchronizeChannel(selection.Items, session.DevelopmentGeneration);
                    Check(node.HasPendingValue && Convert.ToDouble(node.ViewValue) == 33 && history.UndoCount == 0,
                        "failed-command-and-refresh-preserve-staged-value-without-history");
                    Check(session.Get(first, type, session.FieldIndex(type, "position")).X == 0, "pending-value-never-written-to-native-storage");
                }
                else
                {
                    // Fault below protocol preflight: a later native target loses its component
                    // after binding. This tests adapter partial-failure receipts, not MCP races.
                    session.RemoveComponent(second, type);
                    var receipt = panel.ChannelSet("Transform2D.positionX", JsonValue.Create(77), panel.ChannelGeneration);
                    Check(receipt["handler_returned"]!.GetValue<bool>() && receipt["outcome"]!.GetValue<string>() == "failed_possibly_partial",
                        "handler-true-does-not-hide-later-native-target-failure");
                    Check(receipt["pixie_result"]!.GetValue<int>() == 2 && receipt["pixie_last_error"]!.GetValue<string>().Contains("Transform2D"),
                        "native-result-and-error-captured-before-readback");
                    Check(session.Get(first, type, session.FieldIndex(type, "position")).X == 77 && history.UndoCount == 1,
                        "partial-native-write-and-real-history-effects-reported");
                    Check(receipt["bind_failures"]!.AsArray().Count > 0 && receipt["possibly_applied"]!.GetValue<bool>(),
                        "immutable-bind-failure-receipt-survives-successful-other-target-read");
                    var read = panel.ChannelGet("Transform2D.positionX", live: true);
                    Check(read["read_succeeded"]!.GetValue<bool>() == false && read["read_failures"]!.AsArray().Count > 0,
                        "getter-caught-internally-is-read-failure-not-valid-null");
                    var gesture = panel.ChannelGesture(() => panel.Set("Transform2D", "positionX", "88"));
                    Check(gesture["handler_returned"]!.GetValue<bool>() && gesture["outcome"]!.GetValue<string>() == "failed_possibly_partial",
                        "editor-gesture-capture-detects-node-refusal-despite-handler-true");
                }
            }
            exit = 0;
        }
        catch (Exception e) { Console.Error.WriteLine(e); }
        finally
        {
            File.WriteAllText(args[2], Results.ToJsonString(new() { WriteIndented = true }));
            ImGui.EndFrame(); ImGuiImplOpenGL3.Shutdown(); ImGuiImplGLFW.Shutdown();
            ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            ImGui.DestroyContext(context); GLFW.DestroyWindow(window); GLFW.Terminate();
        }
        if (exit == 0) Console.WriteLine($"PASS: {Results.Count} native/Inspector adapter fault cases (direct owner-thread fixture, not MCP race acceptance).");
        return exit;
    }
}
