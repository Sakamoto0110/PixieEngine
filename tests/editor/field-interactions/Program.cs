using System.Collections;
using System.Diagnostics;
using System.IO.Pipes;
using System.Reflection;
using System.Text.Json;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Editing;
using PixieEditor.Development;
using PixieEditor.Native;
using PixieEditor.Ui;
using PixieInspector;
using PixieInspector.ImGui;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

internal static class Program
{
    private static readonly List<string> Passed = [];
    private static void Check(bool condition, string name)
    {
        if (!condition) throw new InvalidOperationException(name);
        Passed.Add(name);
        Console.WriteLine("PASS " + name);
    }
    private static object Section(InspectorPanel panel, string name) =>
        ((IEnumerable)typeof(InspectorPanel).GetField("Sections", BindingFlags.NonPublic | BindingFlags.Instance)!.GetValue(panel)!)
        .Cast<object>().Single(s => (string)s.GetType().GetProperty("Name")!.GetValue(s)! == name);
    private static Inspector InspectorOf(InspectorPanel panel, string name) =>
        (Inspector)Section(panel, name).GetType().GetProperty("Inspector")!.GetValue(Section(panel, name))!;
    private static ImGuiInspectorView ViewOf(InspectorPanel panel, string name) =>
        (ImGuiInspectorView)Section(panel, name).GetType().GetProperty("View")!.GetValue(Section(panel, name))!;
    // Exercise the real view's public lifecycle/events around core gestures, without a second writer.
    // Actual mouse/keyboard routing is qualified by the Inspector ImGui input harness.
    private static InspectorEditGesture Begin(ImGuiInspectorView view, InspectorNode node) =>
        (InspectorEditGesture)typeof(ImGuiInspectorView).GetMethod("BeginInteraction", BindingFlags.NonPublic | BindingFlags.Instance)!.Invoke(view, [node])!;

    private static unsafe int Main(string[] args)
    {
        if (args.Length != 3) return 2;
        var native = Path.GetFullPath(args[0]);
        var root = Path.GetFullPath(args[1]);
        Directory.CreateDirectory(root);
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(1280, 900, "Field interaction qualification", default, default);
        if (window.Handle == null) return 4;
        GLFW.MakeContextCurrent(window);
        Gl.Load();
        var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        var io = ImGui.GetIO(); io.IniFilename = (byte*)null;
        ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context); ImGuiImplOpenGL3.Init("#version 450");
        ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); ImGui.NewFrame();
        GameLibrary.Use(native);
        try
        {
            var folder = Path.Combine(root, "project-" + Guid.NewGuid().ToString("N"));
            EngineSession.CreateProject(folder);
            using var session = EngineSession.OpenProject(1280, 900, folder);
            var shape = session.TypeIndex("Shape"); var collider = session.TypeIndex("Collider2D");
            var transform = session.TypeIndex("Transform2D");
            var size = session.FieldIndex(shape, "size"); var half = session.FieldIndex(collider, "halfSize");
            var first = session.Entities()[0]; var second = session.CreateEntity("second");
            foreach (var entity in new[] { first, second })
                foreach (var type in new[] { transform, shape, collider })
                    if (!session.Has(entity, type)) session.AddComponent(entity, type);
            session.Set(first, shape, size, new PixieValue { X = 40, Y = 80 });
            session.Set(second, shape, size, new PixieValue { X = 100, Y = 200 });
            session.Set(first, collider, half, new PixieValue { X = 20, Y = 40 });
            session.Set(second, collider, half, new PixieValue { X = 50, Y = 100 });
            var firstId = session.SceneId(first)!.Value; var secondId = session.SceneId(second)!.Value;
            session.Save();
            var selection = new Selection(); selection.Set(new[] { first, second });
            var history = new History(); var authoring = new Authoring(session, selection, history);
            var recorder = new EditRecorder(session, history, () => authoring.Recording);
            using var panel = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
            void Sync() { selection.Set(new[] { session.Find(firstId)!.Value, session.Find(secondId)!.Value }); panel.SynchronizeChannel(selection.Items, session.DevelopmentGeneration); }
            Sync();
            var s = InspectorOf(panel, "Shape"); var c = InspectorOf(panel, "Collider2D");
            var view = ViewOf(panel, "Shape");
            Check(view.EditIcons != null, "supplied-icon-textures-load-in-host");
            var x = s["sizeX"]; var y = s["sizeY"]; var cx = c["halfSizeX"]; var cy = c["halfSizeY"];
            Check(x.ChainsWith.Count == 3 && cx.ChainsWith.Contains(x), "cross-component-eligibility-is-symmetric");
            Check(!x.ChainsWith.Contains(InspectorOf(panel, "Transform2D")["positionX"]), "position-not-mixed-with-dimensions");
            x.EnableChain();
            Check(x.ChainedWith.Count == 1, "first-click-arms-singleton");
            cx.EnableChain(); cy.EnableChain(); y.EnableChain(); cy.IsEditLocked = true;
            Check(!session.Unsaved && history.UndoCount == 0, "state-toggles-do-not-dirty-or-record");
            using (var edit = Begin(view, x))
            {
                Check(edit.SetDelta(5).Accepted && edit.SetDelta(10).Accepted, "total-delta-accepted");
                Check(session.Get(first, shape, size).X == 50 && session.Get(first, shape, size).Y == 90 && session.Get(second, shape, size).X == 110 && session.Get(second, shape, size).Y == 210, "each-axis-and-bind-preserves-own-start");
                Check(session.Get(first, collider, half).X == 25 && session.Get(first, collider, half).Y == 45 && session.Get(second, collider, half).Y == 105, "view-delta-converts-native-half-extents-and-reaches-locked-peer");
            }
            view.EndInteraction();
            Check(history.UndoCount == 1, "whole-chain-is-one-undo-step");
            var positionField = session.FieldIndex(transform, "position");
            session.Set(first, transform, positionField, new PixieValue { X = 77 });
            history.Undo(session); Sync(); first = session.Find(firstId)!.Value; second = session.Find(secondId)!.Value;
            Check(session.Get(first, shape, size).X == 40 && session.Get(second, collider, half).Y == 100, "undo-restores-all-members-and-binds");
            Check(session.Get(first, transform, positionField).X == 77, "scalar-chain-undo-preserves-unrelated-native-fields");
            history.Redo(session); Sync(); first = session.Find(firstId)!.Value; second = session.Find(secondId)!.Value;
            Check(session.Get(first, shape, size).X == 50 && session.Get(second, collider, half).Y == 105, "redo-restores-all-members-and-binds");
            Check(InspectorOf(panel, "Shape")["sizeX"].IsChainEnabled && InspectorOf(panel, "Collider2D")["halfSizeY"].IsEditLocked, "scalar-history-retains-unchanged-bindings-and-ui-state");
            session.Set(first, transform, positionField, new PixieValue());
            session.Reset(); Sync(); first = session.Find(firstId)!.Value; second = session.Find(secondId)!.Value;
            s = InspectorOf(panel, "Shape"); c = InspectorOf(panel, "Collider2D"); view = ViewOf(panel, "Shape");
            x = s["sizeX"]; y = s["sizeY"]; cx = c["halfSizeX"]; cy = c["halfSizeY"];
            Check(!x.IsChainEnabled && !cy.IsEditLocked, "entity-replacement-resets-ui-state");
            x.EnableChain(); y.EnableChain(); cx.EnableChain(); cy.EnableChain();
            using (var edit = Begin(view, x)) Check(edit.SetValue(120d).Accepted, "absolute-view-entry-accepted");
            view.EndInteraction();
            Check(session.Get(first, shape, size).Y == 120 && session.Get(second, collider, half).X == 60, "absolute-value-shared-in-display-domain");
            cy.ReadOnly = true;
            var protectedBefore = session.Get(first, collider, half).Y;
            var result = Begin(view, x).SetValue(130d); view.EndInteraction();
            Check(!result.Accepted && result.MayHavePartialEffects && session.Get(first, collider, half).Y == protectedBefore && session.Get(first, shape, size).X == 130, "readonly-peer-is-refused-with-honest-partial-effect");
            cy.ReadOnly = false;
            // Programmatic handlers remain independent of both UI lock and propagation.
            cy.IsEditLocked = true; cy.SetValue(140d);
            Check(session.Get(first, collider, half).Y == 70 && session.Get(first, shape, size).X == 130, "programmatic-setter-neither-lock-blocked-nor-propagated");
            recorder.Close(); cy.IsEditLocked = false;
            var count = history.UndoCount;
            var before = session.Snapshot([first, second]);
            Begin(view, x).SetDelta(12); view.EndInteraction(cancel: true);
            Check(session.Snapshot([first, second]) == before && history.UndoCount == count, "cancel-restores-values-without-history");
            x.IsEditLocked = true;
            var refused = false; try { x.BeginEdit(); } catch (InvalidOperationException) { refused = true; }
            Check(refused && session.Snapshot([first, second]) == before, "locked-source-refuses-direct-edit");
            x.IsEditLocked = false; cy.ClearChain();
            Check(!x.IsChainEnabled && !cx.IsChainEnabled && !y.IsChainEnabled, "clear-any-member-clears-entire-chain");
            x.EnableChain(); y.EnableChain(); y.IsEditLocked = true;
            Begin(view, x).SetDelta(1); session.Play();
            Check(!view.IsEditing && history.UndoCount == count + 1, "play-finishes-gesture-before-world-replacement");
            session.Stop(); Sync();
            Check(!InspectorOf(panel, "Shape")["sizeX"].IsChainEnabled && !InspectorOf(panel, "Shape")["sizeY"].IsEditLocked, "play-stop-resets-ui-state");
            first = session.Find(firstId)!.Value; second = session.Find(secondId)!.Value;
            session.Save();
            using (var reopened = EngineSession.OpenProject(320, 240, folder))
                Check(reopened.Get(reopened.Find(firstId)!.Value, shape, size).X == session.Get(first, shape, size).X, "save-reopen-preserves-native-values");
            var node = InspectorOf(panel, "Shape")["sizeX"]; node.EnableChain(); node.IsEditLocked = true;
            panel.SynchronizeChannel([first], session.DevelopmentGeneration);
            Check(!InspectorOf(panel, "Shape")["sizeX"].IsChainEnabled && !InspectorOf(panel, "Shape")["sizeX"].IsEditLocked, "selection-change-resets-ui-state");
            var resetNode = InspectorOf(panel, "Shape")["sizeX"]; resetNode.EnableChain(); resetNode.IsEditLocked = true;
            session.Reset(); panel.SynchronizeChannel([session.Find(firstId)!.Value], session.DevelopmentGeneration);
            Check(!InspectorOf(panel, "Shape")["sizeX"].IsEditLocked, "rebuild-with-same-entity-handle-resets-ui-state");
            Check(!session.Unsaved, "ui-only-state-remains-clean-after-save");
            first = session.Find(firstId)!.Value;
            ImGui.SetNextWindowPos(new System.Numerics.Vector2(0, 0));
            ImGui.SetNextWindowSize(new System.Numerics.Vector2(540, 900));
            panel.Draw([first]);
            ImGui.Render(); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
            Screenshot.Save(Path.Combine(root, "inspector.png"), 1280, 900);
            ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); ImGui.NewFrame();
            var customNodes = (IDictionary)typeof(InspectorPanel).GetField("CustomControlNodes", BindingFlags.NonPublic | BindingFlags.Instance)!.GetValue(panel)!;
            Check(customNodes.Count >= 3, "custom-point-list-and-layer-mask-controls-render");
            var categoryField = session.FieldIndex(collider, "category");
            void MaskMouse(bool? down)
            {
                ImGui.EndFrame();
                ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame();
                ImGui.GetIO().AddMousePosEvent(45, 565);
                if (down is { } pressed) ImGui.GetIO().AddMouseButtonEvent(0, pressed);
                ImGui.NewFrame();
                ImGui.SetNextWindowPos(new System.Numerics.Vector2(0, 0));
                ImGui.SetNextWindowSize(new System.Numerics.Vector2(540, 900));
                panel.Draw([first]); ImGui.Render(); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
            }
            // Warm the child-window hover state before pressing, matching separate input frames.
            MaskMouse(null); MaskMouse(null);
            Screenshot.Save(Path.Combine(root, "inspector.png"), 1280, 900);
            MaskMouse(true); MaskMouse(false); MaskMouse(null);
            Screenshot.Save(Path.Combine(root, "mask-input.png"), 1280, 900);
            Console.WriteLine($"Mask input category={session.Get(first, collider, categoryField).X}; mouse={ImGui.GetIO().MousePos}");
            Check(session.Get(first, collider, categoryField).X == 3, "custom-mask-mouse-fixture-reaches-native-handler");
            var categoryControl = customNodes.Values.Cast<InspectorNode>().Single(n => n.Label == "belongs to");
            categoryControl.IsEditLocked = true;
            count = history.UndoCount;
            MaskMouse(null); MaskMouse(true); MaskMouse(false); MaskMouse(null);
            Check(session.Get(first, collider, categoryField).X == 3 && history.UndoCount == count, "custom-mask-lock-blocks-real-mouse-edit");
            session.RemoveComponent(first, collider);
            var compound = session.TypeIndex("CompoundCollider2D"); session.AddComponent(first, compound);
            var shapes = session.FieldIndex(compound, "shapes");
            session.EditRecord(first, compound, shapes, 0, remove: false);
            panel.SynchronizeChannel([first], session.DevelopmentGeneration);
            var recordSection = typeof(InspectorPanel).GetMethod("RecordInspector", BindingFlags.NonPublic | BindingFlags.Instance)!.Invoke(panel, [compound, shapes, 0])!;
            var recordInspector = (Inspector)recordSection.GetType().GetProperty("Inspector")!.GetValue(recordSection)!;
            var recordView = (ImGuiInspectorView)recordSection.GetType().GetProperty("View")!.GetValue(recordSection)!;
            var (itemType, _) = session.RecordInfo(first, compound, shapes);
            Console.WriteLine("Record fields: " + string.Join(", ", Enumerable.Range(0, session.FieldCount(itemType)).Select(f => session.FieldName(itemType, f))));
            var vector = Enumerable.Range(0, session.FieldCount(itemType)).First(f => session.FieldInfo(itemType, f).Kind is PixieFieldKind.Point or PixieFieldKind.Vector);
            var vectorName = session.FieldName(itemType, vector);
            var rx = recordInspector[vectorName + "X"]; var ry = recordInspector[vectorName + "Y"];
            rx.EnableChain(); ry.EnableChain(); ry.IsEditLocked = true;
            var recordBefore = session.GetRecord(first, compound, shapes, 0, vector);
            count = history.UndoCount;
            Begin(recordView, rx).SetDelta(4); recordView.EndInteraction();
            var recordAfter = session.GetRecord(first, compound, shapes, 0, vector);
            Check(recordAfter.X != recordBefore.X && recordAfter.Y != recordBefore.Y && history.UndoCount == count + 1, "compound-record-axes-chain-through-native-recorder");
            history.Undo(session); first = session.Find(firstId)!.Value;
            Check(EditRecorder.Same(session.GetRecord(first, compound, shapes, 0, vector), recordBefore), "compound-record-chain-undo-restores-storage");
            panel.SynchronizeChannel([first], session.DevelopmentGeneration);
            var shapeNode = InspectorOf(panel, "Shape")["sizeX"];
            recordSection = typeof(InspectorPanel).GetMethod("RecordInspector", BindingFlags.NonPublic | BindingFlags.Instance)!.Invoke(panel, [compound, shapes, 0])!;
            recordInspector = (Inspector)recordSection.GetType().GetProperty("Inspector")!.GetValue(recordSection)!;
            Check(shapeNode.ChainsWith.Contains(recordInspector["halfSizeX"]), "shape-to-compound-record-dimensions-eligible");
            shapeNode.EnableChain(); recordInspector["halfSizeX"].EnableChain();
            var mixedBefore = session.Snapshot([first]);
            count = history.UndoCount; view = ViewOf(panel, "Shape");
            Begin(view, shapeNode).SetDelta(2); view.EndInteraction(cancel: true);
            Check(session.Snapshot([first]) == mixedBefore && history.UndoCount == count, "mixed-field-record-cancel-restores-group-start-without-ghost-step");
            Begin(view, shapeNode).SetDelta(2); view.EndInteraction();
            Check(history.UndoCount == count + 1, "mixed-field-record-edit-is-one-history-step");
            var mixedAfter = session.Snapshot([first]);
            history.Undo(session); first = session.Find(firstId)!.Value;
            Check(session.Snapshot([first]) == mixedBefore, "mixed-field-record-undo-orders-entity-replacement-before-fields");
            history.Redo(session); first = session.Find(firstId)!.Value;
            Check(session.Snapshot([first]) == mixedAfter, "mixed-field-record-redo-restores-complete-observed-result");
            var probeType = session.TypeIndex("WriteProbe");
            if (probeType >= 0)
            {
                var probeEntity = session.Entities().First(e => session.Has(e, probeType));
                if (!session.Has(probeEntity, transform)) session.AddComponent(probeEntity, transform);
                var probeId = session.SceneId(probeEntity)!.Value;
                panel.SynchronizeChannel([probeEntity], session.DevelopmentGeneration);
                var probe = InspectorOf(panel, "WriteProbe")["value"];
                var position = InspectorOf(panel, "Transform2D")["positionX"];
                // Test-only eligibility injects a native throwing peer; production groups stay semantic.
                position.ChainsWith = new[] { probe }; position.EnableChain(); probe.EnableChain();
                view = ViewOf(panel, "Transform2D"); count = history.UndoCount;
                result = Begin(view, position).SetValue(101d); view.EndInteraction();
                Check(!result.Accepted && result.MayHavePartialEffects && session.Get(probeEntity, probeType, 0).X == 0 && history.UndoCount == count + 1, "native-peer-refusal-preserves-actual-effects-in-one-step");
                history.Undo(session); probeEntity = session.Find(probeId)!.Value;
                Check(history.UndoCount == count && session.Get(probeEntity, transform, session.FieldIndex(transform, "position")).X == 0, "partial-chain-remains-undoable");
            }
            // A real queued JSON-RPC structural request must close the view before native dispatch.
            selection.Set(session.Find(firstId)!.Value);
            var channelName = "field-chain-" + Guid.NewGuid().ToString("N");
            using (var channel = new EditorChannel(channelName, Path.Combine(root, "captures"), session, selection, panel, history, _ => false))
            using (var client = new NamedPipeClientStream(".", "pixie-" + channelName, PipeDirection.InOut, PipeOptions.Asynchronous))
            {
                client.Connect(5000);
                using var writer = new StreamWriter(client, leaveOpen: true) { AutoFlush = true };
                using var reader = new StreamReader(client, leaveOpen: true);
                string Request(string method)
                {
                    writer.WriteLine(JsonSerializer.Serialize(new { jsonrpc = "2.0", id = 1, method }));
                    var read = reader.ReadLineAsync().WaitAsync(TimeSpan.FromSeconds(5));
                    var timer = Stopwatch.StartNew();
                    while (!read.IsCompleted && timer.Elapsed < TimeSpan.FromSeconds(5)) { channel.Drain(); Thread.Sleep(1); }
                    return read.GetAwaiter().GetResult()!;
                }
                Check(!JsonDocument.Parse(Request("world.hello")).RootElement.TryGetProperty("error", out _), "source-mode-real-channel-handshake");
                var pos = InspectorOf(panel, "Transform2D")["positionX"]; view = ViewOf(panel, "Transform2D");
                long? finishedRevision = null; bool? abandoned = null;
                view.InteractionFinished += (_, e) => { finishedRevision = session.BindingRevision; abandoned = e.Cancelled; };
                var revision = session.BindingRevision; count = history.UndoCount;
                Begin(view, pos).SetDelta(3);
                Check(!JsonDocument.Parse(Request("world.reset")).RootElement.TryGetProperty("error", out _), "source-mode-real-channel-reset");
                Check(finishedRevision == revision && abandoned == false && !view.IsEditing && history.UndoCount == count + 1,
                    "structural-channel-closes-interaction-before-replacing-native-bindings");
            }
            File.WriteAllText(args[2], JsonSerializer.Serialize(new { passed = Passed.Count, checks = Passed }, new JsonSerializerOptions { WriteIndented = true }));
            return 0;
        }
        finally
        {
            ImGui.EndFrame(); ImGuiImplOpenGL3.Shutdown(); ImGuiImplGLFW.Shutdown();
            ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            ImGui.DestroyContext(context); GLFW.DestroyWindow(window); GLFW.Terminate();
        }
    }
}
