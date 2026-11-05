using System.Collections;
using System.Diagnostics;
using System.IO.Pipes;
using System.Reflection;
using System.Text;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Development;
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
    // Bypass only the managed notification in the native-fault fixture, never in product code.
    private static void RemoveNativeComponent(EngineSession session, uint entity, int type)
    {
        var handle = (nint)typeof(EngineSession).GetField("Handle", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session)!;
        EngineSession.Check(PixieCapi.ComponentRemove(handle, entity, type), "inject native component removal");
    }
    private static NamedPipeClientStream Connect(string name)
    {
        var pipe = new NamedPipeClientStream(".", "pixie-" + name, PipeDirection.InOut, PipeOptions.Asynchronous);
        try { pipe.Connect(5000); return pipe; }
        catch { pipe.Dispose(); throw; }
    }

    private static JsonNode Exchange(EditorChannel channel, NamedPipeClientStream pipe, StreamReader reader, params string[] requests)
    {
        pipe.Write(Encoding.UTF8.GetBytes(string.Join('\n', requests) + "\n"));
        using var deadline = new CancellationTokenSource(TimeSpan.FromSeconds(5));
        var reply = reader.ReadLineAsync(deadline.Token).AsTask();
        while (!reply.IsCompleted && !deadline.IsCancellationRequested)
        {
            channel.Drain();
            Thread.Sleep(1);
        }
        return JsonNode.Parse(reply.GetAwaiter().GetResult() ?? throw new EndOfStreamException("Editor channel pipe closed."))!;
    }

    private static string Request(int id, string method, string parameters = "{}") =>
        $$"""{"jsonrpc":"2.0","id":{{id}},"method":"{{method}}","params":{{parameters}}}""";

    private static void CheckHello(EngineSession session, Selection selection, InspectorPanel panel, History history, string folder)
    {
        var name = "editor-hello-" + Guid.NewGuid().ToString("N");
        var captures = Path.Combine(folder, "captures");
        using var channel = new EditorChannel(name, captures, session, selection, panel, history, _ => false);
        JsonNode Exchange(NamedPipeClientStream pipe, StreamReader reader, params string[] requests) =>
            Program.Exchange(channel, pipe, reader, requests);
        static bool BeforeHello(JsonNode response, int id) => response["id"]?.GetValue<int>() == id &&
            response["error"]?["message"]?.GetValue<string>() == "world.hello must be the first call." &&
            response["error"]?["data"]?["possibly_applied"]?.GetValue<bool>() == false;
        const string overflow = """{"overflow":1e9999}"""; // Managed JSON preserves it, but the native parser rejects it.
        const string badNotification = """{"jsonrpc":"2.0","method":"world.hello","params":{"overflow":1e9999}}""";
        const string goodNotification = """{"jsonrpc":"2.0","method":"world.hello","params":{}}""";
        using (var pipe = Connect(name))
        using (var reader = new StreamReader(pipe, leaveOpen: true))
        {
            Check(BeforeHello(Exchange(pipe, reader, Request(1, "editor.history")), 1), "editor-only-call-requires-hello");
            Check(BeforeHello(Exchange(pipe, reader, badNotification, Request(2, "editor.history")), 2),
                "failed-native-hello-notification-is-silent-and-does-not-admit");
            var nativeError = Exchange(pipe, reader, Request(3, "world.hello", overflow));
            Check(nativeError["error"]?["code"]?.GetValue<int>() == -32700, "hello-request-retains-native-parse-error");
            Check(BeforeHello(Exchange(pipe, reader, Request(4, "editor.history")), 4), "failed-hello-request-keeps-editor-gate-closed");
            var good = Exchange(pipe, reader, Request(5, "world.hello"));
            var value = good["result"]?["value"];
            var native = JsonNode.Parse(session.DispatchDevelopment(Request(99, "world.hello")))!["result"]!["value"]!;
            Check(good["id"]?.GetValue<int>() == 5 && value?["engine"] is JsonObject && JsonNode.DeepEquals(value["engine"], native["engine"]),
                "hello-request-preserves-actual-native-engine-object");
            var capabilities = value?["capabilities"];
            Check(capabilities?["editor"]?.GetValue<bool>() == true && capabilities["inspector"]?.GetValue<bool>() == true &&
                capabilities["capture"]?.GetValue<bool>() == true && capabilities["transport"]?.GetValue<bool>() == true &&
                value?["capture_directory"]?.GetValue<string>() == captures, "hello-overlays-editor-operations-and-capture-path");
            var invalid = Exchange(pipe, reader, Request(6, "world.hello", "[]"));
            Check(invalid["error"]?["code"]?.GetValue<int>() == -32602 &&
                BeforeHello(Exchange(pipe, reader, Request(7, "editor.history")), 7), "failed-repeat-hello-resets-before-parameter-validation");
            var admitted = Exchange(pipe, reader, goodNotification, Request(8, "editor.history"));
            Check(admitted["id"]?.GetValue<int>() == 8 && admitted["result"] is JsonObject, "valid-hello-notification-admits-and-stays-silent");
            Check(BeforeHello(Exchange(pipe, reader, badNotification, Request(9, "editor.history")), 9),
                "failed-repeat-hello-notification-revokes-admission-and-stays-silent");
            Check(Exchange(pipe, reader, Request(10, "world.hello"))["result"] is JsonObject, "valid-repeat-hello-reauthorizes-client");

            // Simulate a published edit awaiting rebinding, without publishing another World in this fixture.
            var retryCount = 0;
            session.Edits.Published();
            session.Edits.RetryRecovery = () =>
            {
                ++retryCount;
                session.Edits.Recovered();
                return true;
            };
            try
            {
                var duringRecovery = Exchange(pipe, reader, Request(11, "world.hello"))["result"];
                Check(duringRecovery?["recovery_required"]?.GetValue<bool>() == true && duringRecovery["value"]?["engine"] is JsonObject,
                    "read-only-hello-still-reports-recovery");
                var blocked = Exchange(pipe, reader, Request(12, "inspector.set"))["error"];
                Check(blocked?["message"]?.GetValue<string>().Contains("finish Inspector recovery", StringComparison.Ordinal) == true &&
                    blocked["data"]?["possibly_applied"]?.GetValue<bool>() == false && history.UndoCount == 0,
                    "successful-hello-does-not-bypass-the-recovery-edit-gate");
                var recovered = Exchange(pipe, reader, Request(13, "editor.do", """{"command":"recover"}"""))["result"]?["value"];
                Check(recovered?["outcome"]?.GetValue<string>() == "recovered" && recovered["world_replayed"]?.GetValue<bool>() == false &&
                    retryCount == 1 && history.UndoCount == 0, "channel-recovery-retry-keeps-world-and-history-intact");
            }
            finally
            {
                session.Edits.RetryRecovery = null;
                session.Edits.Recovered();
            }
        }
        using (var pipe = Connect(name))
        using (var reader = new StreamReader(pipe, leaveOpen: true))
            Check(BeforeHello(Exchange(pipe, reader, Request(14, "editor.history")), 14), "reconnected-client-must-complete-its-own-hello");
    }

    private sealed record Publication(PixieSceneEditToken Token, long Binding, long History, int Undo, int Redo,
        bool Unsaved, string Document);

    private static void CheckCommittedRecovery(EngineSdk sdk, string root)
    {
        const string scopeName = "Committed recovery";
        const string create = "scenecreate=" + scopeName;
        var folder = Path.Combine(root, "committed-recovery-" + Guid.NewGuid().ToString("N"));
        EngineSession.CreateProject(folder);
        var scene = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single();
        var document = JsonNode.Parse(File.ReadAllText(scene))!.AsObject();
        document["version"] = 2;
        document["sceneScopes"] = new JsonArray();
        document["qualificationOpaque"] = new JsonObject { ["retained"] = true };
        File.WriteAllText(scene, document.ToJsonString());

        Dictionary<string, string> SavedFiles() => Directory.EnumerateFiles(folder, "*", SearchOption.AllDirectories)
            .ToDictionary(path => Path.GetRelativePath(folder, path), EngineSdk.HashFile, StringComparer.Ordinal);
        var saved = SavedFiles();
        bool SavedUnchanged()
        {
            var current = SavedFiles();
            return current.Count == saved.Count && saved.All(file => current.GetValueOrDefault(file.Key) == file.Value);
        }

        using var session = EngineSession.OpenProject(320, 240, folder);
        var selection = new Selection();
        selection.Set(session.Entities()[0]);
        var history = new History();
        var authoring = new Authoring(session, selection, history);
        var recorder = new EditRecorder(session, history, () => authoring.Recording);
        using var panel = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
        panel.Rebind(selection);
        Publication Observe() => new(session.ProjectRevision(), session.BindingRevision, history.Revision,
            history.UndoCount, history.RedoCount, session.Unsaved, session.ProjectSnapshot());
        var fault = new InvalidOperationException("Injected post-publication binding failure.");
        var failRebind = true;
        var rebindCalls = 0;
        var tokenAtFault = default(PixieSceneEditToken);
        var undoAtFault = 0;
        var scopeAtFault = false;
        var editor = new SceneDocumentEditor(session, selection, history, recorder, () => false, () =>
        {
            ++rebindCalls;
            if (failRebind)
            {
                tokenAtFault = session.ProjectRevision();
                undoAtFault = history.UndoCount;
                scopeAtFault = session.SceneScopes().Any(scope => scope.Name == scopeName);
                throw fault;
            }
            panel.Rebind(selection);
        });
        authoring.SceneEditor = editor;
        var scenes = new SceneAuthoring(session, selection, history, editor);
        authoring.Scenes = scenes;
        var routed = new List<string>();
        var name = "editor-committed-recovery-" + Guid.NewGuid().ToString("N");
        var captures = Path.Combine(root, "captures-" + Guid.NewGuid().ToString("N"));
        using var channel = new EditorChannel(name, captures, session, selection, panel, history, command =>
        {
            routed.Add(command);
            if (command == create) return scenes.Run("scenecreate", scopeName);
            if (command == "undo") { editor.Undo(); return true; }
            if (command == "redo") { editor.Redo(); return true; }
            throw new InvalidOperationException("Unexpected committed recovery command: " + command);
        });
        using var pipe = Connect(name);
        using var reader = new StreamReader(pipe, leaveOpen: true);
        var id = 0;
        JsonNode Send(string method, string parameters = "{}")
        {
            var reply = Exchange(channel, pipe, reader, Request(++id, method, parameters));
            if (reply["id"]?.GetValue<int>() != id) throw new InvalidOperationException("Unexpected recovery response ID.");
            return reply;
        }
        JsonNode Gesture(string command, long generation) =>
            Send("editor.do", new JsonObject { ["command"] = command, ["generation"] = generation }.ToJsonString());
        static JsonNode? Value(JsonNode reply) => reply["result"]?["value"];
        static bool RecoveryRefusal(JsonNode reply) =>
            reply["error"]?["message"]?.GetValue<string>().Contains("finish Inspector recovery", StringComparison.Ordinal) == true &&
            reply["error"]?["data"]?["possibly_applied"]?.GetValue<bool>() == false &&
            reply["error"]?["data"]?["recovery_required"]?.GetValue<bool>() == true;
        static bool Refuses(Action action)
        {
            try { action(); return false; }
            catch (EngineException) { return true; }
        }

        var hello = Value(Send("world.hello"));
        Check(hello?["engine"]?["sdkId"]?.GetValue<string>() == sdk.Id &&
            hello["engine"]?["sdkBuildId"]?.GetValue<string>() == sdk.BuildId &&
            hello["capabilities"]?["save"]?.GetValue<bool>() == true, "committed-recovery-hello-identifies-selected-save-enabled-sdk");
        using (var process = Process.GetCurrentProcess())
        {
            var modules = process.Modules.Cast<ProcessModule>().ToArray();
            Check(modules.Any(module => string.Equals(module.FileName, sdk.RuntimePath, StringComparison.OrdinalIgnoreCase) &&
                    EngineSdk.HashFile(module.FileName) == sdk.RuntimeSha256) &&
                modules.All(module => !module.ModuleName.EndsWith("-module.dll", StringComparison.OrdinalIgnoreCase)),
                "committed-recovery-loads-exact-no-code-runtime");
        }
        var originalGeneration = Value(Send("inspector.tree"))!["generation"]!.GetValue<long>();
        var baseline = Observe();
        Check(session.SceneFormatVersion == 2 && session.CandidateEditRevision() == baseline.Token.WorldRevision &&
            session.SceneScopes().Length == 0 && !baseline.Unsaved && baseline.Undo == 0 && !session.Edits.RecoveryRequired,
            "committed-recovery-opens-owned-v2-candidate-session");

        var created = Gesture(create, originalGeneration);
        var committed = Observe();
        var scope = session.SceneScopes().Single();
        Check(created["result"]?["recovery_required"]?.GetValue<bool>() == true &&
            Value(created)?["history_before"]?["undo_count"]?.GetValue<int>() == 0 &&
            Value(created)?["history_after"]?["undo_count"]?.GetValue<int>() == 1,
            "committed-recovery-channel-reports-committed-history");
        Check(committed.Token.SessionIdentity == baseline.Token.SessionIdentity &&
            committed.Token.WorldRevision == baseline.Token.WorldRevision + 1 &&
            committed.Token.AuthoredRevision == baseline.Token.AuthoredRevision &&
            committed.Binding == baseline.Binding + 1 && committed.History == baseline.History + 1 &&
            committed.Undo == 1 && committed.Redo == 0 && committed.Unsaved && committed.Document != baseline.Document &&
            scope.Id != 0 && scope.Name == scopeName && session.Edits.RecoveryRequired && SavedUnchanged(),
            "committed-recovery-publishes-native-world-and-history-without-save");
        Check(rebindCalls == 1 && editor.RecoveryError == fault && tokenAtFault.Equals(committed.Token) &&
            undoAtFault == 1 && scopeAtFault, "committed-recovery-callback-observes-native-commit-and-undo");

        var readable = Send("world.entities");
        Check(readable["result"]?["recovery_required"]?.GetValue<bool>() == true && Value(readable)?["entities"] is JsonArray,
            "committed-recovery-allows-world-reads");
        Check(RecoveryRefusal(Send("world.create", """{"name":"must not be created"}""")) &&
            RecoveryRefusal(Send("inspector.set", new JsonObject
            {
                ["generation"] = originalGeneration, ["path"] = "Transform2D.positionX", ["value"] = 99,
            }.ToJsonString())) &&
            RecoveryRefusal(Gesture("save", originalGeneration)) && RecoveryRefusal(Gesture("undo", originalGeneration)) &&
            RecoveryRefusal(Send("world.capture", """{"name":"must-not-write"}""")) &&
            RecoveryRefusal(Send("editor.history")) && Refuses(session.Save) && Refuses(() => history.Undo(session)) &&
            routed.SequenceEqual([create]), "committed-recovery-refuses-writes-save-capture-and-history-before-routing");
        Check(Observe() == committed && session.Edits.RecoveryRequired && SavedUnchanged() &&
            !Directory.EnumerateFiles(captures, "*", SearchOption.AllDirectories).Any(),
            "committed-recovery-refusals-preserve-world-history-and-disk");

        var retry = Value(Send("editor.do", """{"command":"recover"}"""));
        Check(retry?["outcome"]?.GetValue<string>() == "recovery_pending" &&
            retry["world_replayed"]?.GetValue<bool>() == false && session.Edits.RecoveryRequired && rebindCalls == 2 &&
            Observe() == committed && SavedUnchanged(), "committed-recovery-failed-retry-keeps-published-state");
        failRebind = false;
        retry = Value(Send("editor.do", """{"command":"recover"}"""));
        Check(retry?["outcome"]?.GetValue<string>() == "recovered" &&
            retry["world_replayed"]?.GetValue<bool>() == false && !session.Edits.RecoveryRequired && rebindCalls == 3 &&
            editor.RecoveryError is null && selection.Scope == scope.Id && Observe() == committed && SavedUnchanged(),
            "committed-recovery-successful-retry-rebinds-without-republication");
        var rebound = Value(Send("editor.scenes"));
        var retained = Value(Send("editor.history"));
        Check(rebound?["selected_scope"]?.GetValue<string>() == scope.Id.ToString("x16") &&
            retained?["undo_count"]?.GetValue<int>() == 1 && retained["redo_count"]?.GetValue<int>() == 0,
            "committed-recovery-reports-rebound-scene-and-retained-history");

        var generation = Value(Send("inspector.tree"))!["generation"]!.GetValue<long>();
        var stale = Gesture("undo", originalGeneration);
        Check(generation != originalGeneration && stale["error"]?["data"]?["possibly_applied"]?.GetValue<bool>() == false &&
            stale["error"]?["message"]?.GetValue<string>().Contains("Stale", StringComparison.Ordinal) == true &&
            Observe() == committed && SavedUnchanged(), "committed-recovery-refuses-old-inspector-generation");
        Check(Gesture("undo", generation)["result"] is JsonObject, "committed-recovery-fresh-undo-uses-retained-step");
        var undone = Observe();
        Check(session.SceneScopes().Length == 0 && undone.Undo == 0 && undone.Redo == 1 &&
            undone.Token.WorldRevision == committed.Token.WorldRevision + 1 && undone.History == committed.History + 1 &&
            undone.Document == baseline.Document && SavedUnchanged(), "committed-recovery-undo-restores-exact-baseline-snapshot");
        generation = Value(Send("inspector.tree"))!["generation"]!.GetValue<long>();
        Check(Gesture("redo", generation)["result"] is JsonObject, "committed-recovery-fresh-redo-uses-retained-step");
        var redone = Observe();
        Check(session.SceneScopes().Single() == scope && redone.Undo == 1 && redone.Redo == 0 &&
            redone.Token.WorldRevision == undone.Token.WorldRevision + 1 && redone.History == undone.History + 1 &&
            redone.Document == committed.Document && SavedUnchanged() && routed.SequenceEqual([create, "undo", "redo"]),
            "committed-recovery-redo-restores-exact-committed-snapshot");
    }

    private static unsafe int Main(string[] args)
    {
        var committedRecovery = args is ["--committed-recovery", ..];
        if (args.Length != 15 || (!committedRecovery && !Path.IsPathFullyQualified(args[0])) ||
            !Path.IsPathFullyQualified(args[1]) || !Path.IsPathFullyQualified(args[2]) ||
            args[3] != "--sdk" || !Path.IsPathFullyQualified(args[4]) ||
            args[5] != "--sdk-profile" || args[6] != "local-development" ||
            args[7] != "--native-configuration" || args[8] is not ("Debug" or "Release") ||
            (committedRecovery && args[8] != "Debug") ||
            args[9] != "--sdk-id" || args[11] != "--sdk-build-id" || args[13] != "--sdk-manifest-sha256")
        {
            Console.Error.WriteLine("Usage: PixieEditor.ChannelTests <absolute probe DLL|--committed-recovery> <absolute scratch root> <absolute result JSON> " +
                "--sdk <absolute SDK root> --sdk-profile local-development --native-configuration <Debug|Release> " +
                "--sdk-id <expected SDK id> --sdk-build-id <expected build id> --sdk-manifest-sha256 <expected catalog hash> " +
                "(--committed-recovery requires Debug)");
            return 2;
        }
        var root = args[1];
        Directory.CreateDirectory(root);
        if (!committedRecovery) PipeQualification.Run(Check).GetAwaiter().GetResult();
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
        if (committedRecovery)
        {
            var io = ImGui.GetIO();
            io.IniFilename = null;
        }
        ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context); ImGuiImplOpenGL3.Init("#version 450");
        ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); ImGui.NewFrame();
        int exit = 1;
        try
        {
            var sdk = EngineSdk.Load(args[4], args[6], args[8], origin: "channel qualification",
                expectedId: args[10], expectedBuildId: args[12], expectedManifestSha256: args[14]);
            Check(sdk.DistributionClass == "local-private" && (sdk.CapabilityBits & 2) != 0, "explicit-private-sdk-advertises-native-automation");
            GameLibrary.SelectSdk(sdk);
            GameLibrary.Use(committedRecovery ? sdk.RuntimePath : args[0]);
            if (committedRecovery)
            {
                CheckCommittedRecovery(sdk, root);
                Console.WriteLine($"PASS: {Results.Count} private Debug committed recovery checks.");
                return 0;
            }
            foreach (var mode in new[] { "readonly", "pending", "stale", "partial", "rejected-setter" })
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
                    CheckHello(session, selection, panel, history, folder);
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
                else if (mode == "stale")
                {
                    session.RemoveComponent(second, type);
                    var receipt = panel.ChannelSet("Transform2D.positionX", JsonValue.Create(77), panel.ChannelGeneration);
                    Check(receipt["pixie_result"] == null && receipt["bind_failures"]!.ToJsonString().Contains("replaced World"),
                        "managed-removal-invalidates-bindings-before-native-access");
                    Check(session.Get(first, type, session.FieldIndex(type, "position")).X == 0 && history.UndoCount == 0,
                        "stale-bindings-do-not-write-other-targets-or-create-history");
                }
                else
                {
                    // Fault below protocol preflight: a later native target loses its component
                    // after binding, without managed invalidation. This tests adapter receipts,
                    // not MCP races; the separate stale case exercises the managed guard.
                    RemoveNativeComponent(session, second, type);
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
