using System.Diagnostics;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Editing;
using PixieEditor.Development;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieEditor.Ui;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

internal static class SceneCandidateTests
{
    internal static unsafe int Run(string native, string work, int? size = null, string family = "shape", bool authoringMeasure = false, bool moduleData = false)
    {
        Directory.CreateDirectory(work);
        var checks = new List<string>();
        void Check(bool value, string name) { if (!value) throw new InvalidOperationException(name); checks.Add(name); }
        bool Refuses(Action operation) { try { operation(); return false; } catch (EngineException) { return true; } }
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(640, 480, "Serial candidate qualification", default, default);
        if (window.Handle == null) { GLFW.Terminate(); return 4; }
        GLFW.MakeContextCurrent(window); Gl.Load(); GameLibrary.Use(Path.GetFullPath(native));
        var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        var io = ImGui.GetIO(); io.IniFilename = (byte*)null;
        ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context); ImGuiImplOpenGL3.Init("#version 450");
        try
        {
            var folder = Path.Combine(Path.GetFullPath(work), "project");
            EngineSession.CreateProject(folder);
            if (Path.GetFileName(native) != "pixie.dll" && !moduleData)
            {
                using var module = EngineSession.OpenProject(640, 480, folder);
                Check(Refuses(() => module.CandidateEditRevision()), "loaded-module-cannot-enter-candidate-authoring");
                var probeType = module.TypeIndex("WriteProbe");
                var probe = module.Entities().First(e => module.Has(e, probeType));
                var token = module.ProjectRevision();
                Check(Refuses(() => module.Set(probe, probeType, 0, new PixieValue { X = 101 })) &&
                    module.ProjectRevision().AuthoredRevision > token.AuthoredRevision &&
                    module.ProjectRevision().WorldRevision == token.WorldRevision,
                    "throwing-custom-setter-conservatively-invalidates-authored-token");
                File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks }));
                return 0;
            }
            var generationStart = Stopwatch.GetTimestamp();
            // Private test input: no conversion or Save is performed on an owner's project.
            var scenePath = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single();
            var document = JsonNode.Parse(File.ReadAllText(scenePath))!.AsObject();
            document["version"] = 2; document["sceneScopes"] = new JsonArray();
            document["qualificationOpaque"] = new JsonObject { ["retained"] = true };
            if (size is { } count)
            {
                var template = document["entities"]![0]!.DeepClone();
                if (moduleData)
                    template["components"]!["ProbeData"] = new JsonObject
                        { ["v"] = 1, ["value"] = 12.5, ["count"] = 7, ["limit"] = 9, ["ratio"] = 1.0 };
                var entities = new JsonArray();
                for (var i = 0; i < count; ++i)
                {
                    var entity = template.DeepClone(); entity["id"] = (i + 1).ToString("x16"); entity["name"] = "measured " + i;
                    entity["components"]!["Transform2D"]!["position"] = new JsonArray((i % 100) * 200, (i / 100) * 200);
                    if (family == "physics")
                    {
                        entity["components"]!["Collider2D"] = new JsonObject { ["v"] = 1 };
                        entity["components"]!["RigidBody2D"] = new JsonObject { ["v"] = 1 };
                    }
                    entities.Add(entity);
                }
                document["entities"] = entities;
            }
            File.WriteAllText(scenePath, document.ToJsonString());
            var disk = File.ReadAllBytes(scenePath);
            using var session = EngineSession.OpenProject(640, 480, folder);
            if (moduleData)
                Check(session.TypeIndex("ProbeData") >= 0 && session.Entities().Where(e => session.SceneId(e) != null)
                    .All(e => session.Has(e, session.TypeIndex("ProbeData"))), "measurement-uses-qualified-custom-data-on-authored-entities");
            var selection = new Selection(); var history = new History();
            if (session.Entities().Length > 0) selection.Set(session.Entities()[0]);
            var authoring = new Authoring(session, selection, history);
            var recorder = new EditRecorder(session, history, () => authoring.Recording);
            using var inspector = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
            var active = false; var failRebind = false; var attemptDirectTravel = false;
            void Bind()
            {
                if (failRebind) throw new InvalidOperationException("injected postcommit UI failure");
                if (attemptDirectTravel)
                {
                    attemptDirectTravel = false;
                    Check(Refuses(() => history.Undo(session)), "rebind-callback-cannot-reuse-history-travel-authority");
                }
                if (authoringMeasure) { inspector.Rebind(selection); return; }
                ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); ImGui.NewFrame();
                try { inspector.Draw(selection); }
                finally { ImGui.EndFrame(); }
            }
            var editor = new SceneDocumentEditor(session, selection, history, recorder, () => active, Bind);
            Bind(); Bind();
            if (size == null) QualifyPreparation(folder, work, document, Check, Refuses);
            var before = session.ProjectSnapshot();
            var initialSnapshotNativeCalls = session.LastSnapshotNativeCalls;
            var after = before;
            if (!authoringMeasure)
            {
                var target = JsonNode.Parse(before)!;
                target["sceneScopes"]!.AsArray().Add(new JsonObject { ["id"] = "ffff000000000001", ["name"] = "Empty scope" });
                after = target.ToJsonString();
            }
            var generatedMilliseconds = Stopwatch.GetElapsedTime(generationStart).TotalMilliseconds;
            if (size != null)
            {
                using var process = Process.GetCurrentProcess(); process.Refresh();
                var privateBefore = process.PrivateMemorySize64; var peakBefore = process.PeakWorkingSet64;
                var allocated = GC.GetAllocatedBytesForCurrentThread();
                var measurements = new List<SceneEditMeasurement>();
                if (authoringMeasure)
                {
                    var scenes = new SceneAuthoring(session, selection, history, editor);
                    var scope = scenes.Create("Measured Scene"); measurements.Add(scenes.LastMeasurement);
                    for (var i = 0; i < 3; ++i)
                    {
                        if (i % 2 == 0) scenes.Remove(scope); else scope = scenes.Create("Measured Scene");
                        measurements.Add(scenes.LastMeasurement);
                    }
                }
                else
                {
                    measurements.Add(editor.Apply("cold empty Scene", after));
                    for (var i = 0; i < 3; ++i) measurements.Add(editor.Apply("warm empty Scene", i % 2 == 0 ? before : after));
                }
                allocated = GC.GetAllocatedBytesForCurrentThread() - allocated;
                process.Refresh();
                File.WriteAllText(Path.Combine(work, "measurements.json"), JsonSerializer.Serialize(new
                {
                    entities = size, family, authoringMeasure, snapshotUtf8Bytes = Encoding.UTF8.GetByteCount(after),
                    components = document["entities"]!.AsArray().Sum(e => e!["components"]!.AsObject().Count),
                    generatedMilliseconds, measurements, managedThreadAllocatedBytes = allocated,
                    privateBeforeBytes = privateBefore, privateAfterBytes = process.PrivateMemorySize64,
                    peakWorkingSetBeforeBytes = peakBefore, peakWorkingSetBytes = process.PeakWorkingSet64,
                    peakPagedMemoryBytes = process.PeakPagedMemorySize64, retainedHistoryEntries = history.UndoCount,
                    retainedSnapshotBufferBytes = session.SnapshotBufferCapacity,
                    initialSnapshotNativeCalls,
                    historySnapshots = "Full before/after UTF16 strings retained. Managed thread allocation excludes native allocations; OS process counters cover both domains.",
                    limitations = "Fresh process per size/family. Process peaks include GL/startup and four edits, not isolated allocator attribution. Authoring mode includes creation of the proposed document; candidate mode starts with proposed text ready. Initial snapshot primes the buffer before either measurement. No full-frame/gesture-wait, asset, percentile or threshold claim."
                }, new JsonSerializerOptions { WriteIndented = true, IncludeFields = true }));
                Check(!editor.RecoveryRequired && measurements.All(m => m.Native.WorldRevision != 0), "measured-edits-committed-and-rebound");
            }
            else
            {
                var first = session.Entities()[0]; var id = session.SceneId(first)!.Value;
                var transform = session.TypeIndex("Transform2D"); var position = session.FieldIndex(transform, "position");
                var stale = new ComponentHandle(session, first, transform, recorder);
                var revision = session.CandidateEditRevision(); var binding = session.BindingRevision;
                Check(Task.Run(() => Refuses(() => session.CandidateEditRevision())).GetAwaiter().GetResult(),
                    "native-admission-refuses-a-nonowner-thread-before-world-access");
                active = true;
                Check(Refuses(() => editor.Apply("active", after)) && session.CandidateEditRevision() == revision && history.UndoCount == 0,
                    "active-gesture-refused-without-history-or-publication");
                active = false;
                Check(Refuses(() => editor.Apply("invalid", "{}")) && before == session.ProjectSnapshot() && binding == session.BindingRevision && !session.Unsaved,
                    "invalid-candidate-preserves-world-bindings-dirty-and-history");
                Check(Refuses(() => session.ApplySceneCandidate(after, revision + 1)), "stale-native-revision-refused");
                Check(Refuses(() => session.ApplySceneCandidate(after + "\0ignored", revision)) && session.CandidateEditRevision() == revision,
                    "embedded-nul-cannot-publish-a-different-prefix");
                var invalidBody = JsonNode.Parse(after)!;
                invalidBody["entities"]![0]!["components"]!["Collider2D"] = new JsonObject { ["v"] = 1, ["kind"] = "Circle", ["radius"] = -1 };
                Check(Refuses(() => editor.Apply("invalid physics", invalidBody.ToJsonString())) && before == session.ProjectSnapshot() && !session.Unsaved,
                    "candidate-physics-preparation-failure-retains-published-world");
                Action noPrecommit = () => throw new InvalidOperationException("prepublish binding callback must not run");
                session.BindingsReplacing += noPrecommit;
                var applied = editor.Apply("create empty Scene", after);
                Check(!applied.RecoveryRequired && history.UndoCount == 1 && session.SceneScopes().Length == 1, "candidate-publishes-empty-scene-and-history");
                Check(Refuses(() => stale.Set(position, new PixieValue { X = 99 })) && session.Get(session.Find(id)!.Value, transform, position).X == 0,
                    "old-binding-refuses-even-when-entity-slots-recur");
                Check(Refuses(() => history.Undo(session)) && history.UndoCount == 1 && session.SceneScopes().Length == 1,
                    "shared-history-cannot-bypass-candidate-coordinator");
                session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, 0xffff000000000001);
                editor.Undo();
                Check(session.SceneScopes().Length == 0 && history.RedoCount == 1 && selection.Items.Single() == session.Find(id), "candidate-undo-preserves-selection-and-empty-catalog");
                Check(session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.RemovedSceneFallback &&
                    session.ViewportEnvironmentSelection.SceneId == 0xffff000000000001 && session.ViewportEnvironment.Lighting == PixieEnvironmentLightingState.Absent,
                    "candidate-removal-retains-environment-intent-without-installing-lighting");
                editor.Redo();
                Check(session.SceneScopes().Length == 1 && history.UndoCount == 1 && session.ProjectSnapshot().Contains("qualificationOpaque"), "candidate-redo-preserves-unknown-document-data");
                Check(session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld,
                    "candidate-undo-restoration-resolves-retained-environment-intent");
                failRebind = true;
                var committed = editor.Apply("rebind fault", before);
                Check(committed.RecoveryRequired && history.UndoCount == 2 && session.SceneScopes().Length == 0,
                    "rebind-failure-retains-committed-world-and-history");
                Check(Refuses(() => history.Undo(session)) && history.UndoCount == 2,
                    "direct-history-cannot-bypass-recovery-admission");
                Check(Refuses(() => editor.Apply("blocked until recovered", after)), "recovery-state-blocks-further-editing");
                var recoveryToken = session.ProjectRevision(); var recoveryHistory = history.Revision;
                var blockedWrites = new Action[] { () => session.Set(session.Find(id)!.Value, transform, position, new PixieValue { X = 99 }),
                    () => session.SetName(session.Find(id)!.Value, "blocked"), () => session.CreateEntity("blocked"),
                    session.Save, session.Play, session.Reset, () => recorder.Write(session.Find(id)!.Value, transform, position, new PixieValue { X = 99 }),
                    () => authoring.ImportAsset(Path.Combine(work, "external.png"), "png"),
                    () => inspector.ChannelSet("Transform2D/positionX", JsonValue.Create(99), inspector.ChannelGeneration) };
                Check(blockedWrites.All(Refuses) && session.ProjectRevision().Equals(recoveryToken) &&
                    history.Revision == recoveryHistory && !recorder.HasPendingEdits && File.ReadAllBytes(scenePath).SequenceEqual(disk),
                    "committed-recovery-blocks-native-recorder-inspector-and-filesystem-mutations");
                Check(!JsonNode.Parse(session.DispatchDevelopment("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"world.hello\"}"))!.AsObject().ContainsKey("error") &&
                    Refuses(() => session.DispatchDevelopment("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"world.create\"}")),
                    "recovery-keeps-world-observations-and-refuses-development-mutations");
                var channelName = "candidate-recovery-" + Guid.NewGuid().ToString("N");
                using (var channel = new EditorChannel(channelName, Path.Combine(work, "captures"), session, selection, inspector, history,
                    _ => throw new InvalidOperationException("Blocked editor command reached its handler.")))
                using (var client = new NamedPipeClientStream(".", "pixie-" + channelName, PipeDirection.InOut, PipeOptions.Asynchronous))
                {
                    client.Connect(5000);
                    using var writer = new StreamWriter(client, leaveOpen: true) { AutoFlush = true };
                    using var reader = new StreamReader(client, leaveOpen: true);
                    JsonNode Request(string method, JsonObject? parameters = null)
                    {
                        writer.WriteLine(new JsonObject { ["jsonrpc"] = "2.0", ["id"] = 1, ["method"] = method, ["params"] = parameters ?? new() }.ToJsonString());
                        var read = reader.ReadLineAsync().WaitAsync(TimeSpan.FromSeconds(5));
                        var timer = Stopwatch.StartNew();
                        while (!read.IsCompleted && timer.Elapsed < TimeSpan.FromSeconds(5)) { channel.Drain(); Thread.Sleep(1); }
                        return JsonNode.Parse(read.GetAwaiter().GetResult()!)!;
                    }
                    Check(Request("world.hello")["result"] != null && Request("world.entities")["result"] != null,
                        "real-channel-recovery-allows-fresh-world-observations");
                    var blocked = Request("world.create"); var blockedInspector = Request("inspector.set");
                    Check(blocked["error"]!["data"]!["possibly_applied"]!.GetValue<bool>() == false &&
                        blockedInspector["error"]!["data"]!["possibly_applied"]!.GetValue<bool>() == false &&
                        blocked["error"]!["data"]!["recovery_required"]!.GetValue<bool>(),
                        "real-channel-refusal-precedes-handlers-generation-and-possibly-applied");
                    var pending = Request("editor.do", new() { ["command"] = "recover" });
                    Check(pending["result"]!["value"]!["outcome"]!.GetValue<string>() == "recovery_pending" &&
                        !pending["result"]!["value"]!["world_replayed"]!.GetValue<bool>(),
                        "real-channel-recovery-failure-reports-committed-state-without-replay");
                    failRebind = false;
                    var recovered = Request("editor.do", new() { ["command"] = "recover" });
                    Check(recovered["result"]!["value"]!["outcome"]!.GetValue<string>() == "recovered" &&
                        !recovered["result"]!["recovery_required"]!.GetValue<bool>() && session.ProjectRevision().Equals(recoveryToken) &&
                        history.Revision == recoveryHistory && !editor.RecoveryRequired,
                        "committed-rebind-can-be-retried-through-existing-editor-do-contract");
                }
                Check(File.ReadAllBytes(scenePath).SequenceEqual(disk), "candidate-edits-never-write-scene-file");
                selection.SetWorld(); editor.Apply("World target", after);
                Check(selection.IsWorld, "world-inspector-target-preserved");
                session.BindingsReplacing -= noPrecommit;
                selection.PreserveThrough(session, session.Play);
                Check(selection.IsWorld && Refuses(() => session.CandidateEditRevision()), "play-refuses-candidate-editing");
                selection.PreserveThrough(session, session.Stop);
                Check(selection.IsWorld && session.SceneScopes().Length == 1, "stop-restores-authored-catalog-and-world-target");
                var physicsDocument = JsonNode.Parse(session.ProjectSnapshot())!;
                physicsDocument["entities"]!.AsArray().Add(new JsonObject
                {
                    ["id"] = "ffff000000000002", ["name"] = "World physics",
                    ["components"] = new JsonObject { ["PhysicsSettings2D"] = new JsonObject { ["v"] = 1, ["gravity"] = new JsonArray(4, 5) } }
                });
                editor.Apply("World settings", physicsDocument.ToJsonString());
                var physics = session.TypeIndex("PhysicsSettings2D");
                var physicsOwner = session.Find(0xffff000000000002)!.Value;
                Check(session.Get(physicsOwner, physics, session.FieldIndex(physics, "gravity")).Y == 5,
                    "candidate-settings-use-world-authority");
                attemptDirectTravel = true;
                var beforeTravel = session.CandidateEditRevision(); var beforeTravelCount = history.UndoCount;
                editor.Undo();
                Check(!editor.RecoveryRequired && session.CandidateEditRevision() == beforeTravel + 1 && history.UndoCount == beforeTravelCount - 1,
                    "reentrant-rebind-does-not-publish-a-second-world");
                editor.Redo();
                session.Save();
                Check(File.ReadAllText(scenePath).Contains("World physics"), "already-v2-save-retains-candidate-settings");
                using (var reopened = EngineSession.OpenProject(640, 480, folder))
                    Check(reopened.SceneScopes().Length == 1 && reopened.Find(0xffff000000000002) != null,
                        "saved-candidate-reopens-with-settings-and-empty-scope");
                editor.Apply("unsaved rebuild handoff", physicsDocument.ToJsonString());
                var handoff = CodeReopen.Capture(session, selection.Items, null, selection.Anchor, selection.IsWorld);
                using (var rebuilt = EngineSession.OpenProject(640, 480, folder))
                {
                    var state = CodeReopen.Restore(rebuilt, handoff);
                    Check(state.World && rebuilt.Unsaved && rebuilt.SceneScopes().Length == 1 && rebuilt.Find(0xffff000000000002) != null,
                        "candidate-authored-data-survives-code-reopen-handoff");
                }
                var current = session.Find(id)!.Value;
                var beforeRecordedField = session.Get(current, transform, position);
                recorder.Write(current, transform, position, new PixieValue { X = 12 });
                Check(Refuses(() => editor.Apply("unfinished recorder", before)) && recorder.HasPendingEdits,
                    "pending-recorder-is-not-silently-closed-by-admission");
                recorder.Close();
                var currentRevision = session.CandidateEditRevision();
                editor.Undo();
                Check(session.CandidateEditRevision() == currentRevision + 1 &&
                    EditRecorder.Same(session.Get(session.Find(id)!.Value, transform, position), beforeRecordedField),
                    "v2-recorder-history-undo-uses-complete-candidate-publication");
                editor.Redo();
                Check(session.Get(session.Find(id)!.Value, transform, position).X == 12,
                    "v2-recorder-history-redo-restores-observed-field");
                // A deliberately foreign legacy step still belongs to its existing path.
                history.Record(new FieldStep("foreign history boundary", []));
                currentRevision = session.CandidateEditRevision();
                Check(Refuses(editor.Undo) && session.CandidateEditRevision() == currentRevision,
                    "foreign-history-step-stays-on-its-existing-authoring-path");
                var legacyFolder = Path.Combine(Path.GetFullPath(work), "legacy-conflict");
                EngineSession.CreateProject(legacyFolder);
                var legacyPath = Directory.GetFiles(Path.Combine(legacyFolder, "scenes"), "*.scene").Single();
                var legacyDocument = JsonNode.Parse(File.ReadAllText(legacyPath))!;
                foreach (var ownerId in new[] { "ffff000000000004", "ffff000000000005" })
                    legacyDocument["entities"]!.AsArray().Add(new JsonObject { ["id"] = ownerId,
                        ["components"] = new JsonObject { ["PhysicsSettings2D"] = new JsonObject { ["v"] = 1 } } });
                File.WriteAllText(legacyPath, legacyDocument.ToJsonString());
                using var legacy = EngineSession.OpenProject(640, 480, legacyFolder);
                var legacyBefore = legacy.ProjectSnapshot(); var legacyRevision = legacy.CandidateEditRevision();
                Check(Refuses(() => legacy.ApplySceneCandidate(after, legacyRevision)) && legacyBefore == legacy.ProjectSnapshot() && !legacy.Unsaved,
                    "legacy-conflicting-settings-preserve-read-and-refuse-new-candidate-mutation");
            }
            File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks }, new JsonSerializerOptions { WriteIndented = true }));
            Console.WriteLine($"PASS {checks.Count} serial candidate checks");
            return 0;
        }
        finally
        {
            ImGuiImplOpenGL3.Shutdown(); ImGuiImplGLFW.Shutdown();
            ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            ImGui.DestroyContext(context); GLFW.DestroyWindow(window); GLFW.Terminate();
        }
    }

    private static void QualifyPreparation(string folder, string work, JsonObject document,
        Action<bool, string> check, Func<Action, bool> refuses)
    {
        using var session = EngineSession.OpenProject(640, 480, folder);
        var selection = new Selection(); var history = new History();
        var recorder = new EditRecorder(session, history, () => true);
        var editor = new SceneDocumentEditor(session, selection, history, recorder, () => false, () => { });
        var snapshot = session.ProjectSnapshot();
        check(session.LastSnapshotNativeCalls == 1 && session.ProjectSnapshot() == snapshot && session.LastSnapshotNativeCalls == 1,
            "fitting-snapshot-captures-once-without-a-size-query");
        var first = session.Entities()[0]; var transform = session.TypeIndex("Transform2D");
        var position = session.FieldIndex(transform, "position");
        var pending = editor.Prepare(snapshot); var token = session.ProjectRevision();
        session.Set(first, transform, position, new PixieValue { X = 7 });
        check(session.ProjectRevision().WorldRevision == token.WorldRevision && session.ProjectRevision().AuthoredRevision > token.AuthoredRevision &&
            refuses(() => editor.Commit("stale content", pending)) && history.UndoCount == 0 && session.Get(first, transform, position).X == 7,
            "ordinary-field-edit-expires-prepared-document-without-storage-replacement");
        check(refuses(() => session.ApplySceneCandidate(snapshot, token)) && session.Get(first, transform, position).X == 7 &&
            session.ProjectRevision().WorldRevision == token.WorldRevision,
            "native-publication-rejects-expired-authored-token-independently-of-coordinator");
        check(session.ProjectSnapshot() != snapshot && session.LastSnapshotNativeCalls == 1,
            "reused-snapshot-buffer-serializes-fresh-content");
        token = session.ProjectRevision();
        check(refuses(() => session.Set(first, transform, -1, default)) && session.ProjectRevision().Equals(token),
            "pure-invalid-field-admission-does-not-expire-content-token");
        pending = editor.Prepare(snapshot);
        history.Record(new FieldStep("history-only boundary", []));
        check(refuses(() => editor.Commit("stale history", pending)) && session.ProjectRevision().Equals(token),
            "history-only-change-expires-prepared-document");
        pending = editor.Prepare(snapshot);
        using (var other = EngineSession.OpenProject(640, 480, folder))
        {
            var foreign = other.ProjectRevision(); foreign.WorldRevision = token.WorldRevision; foreign.AuthoredRevision = token.AuthoredRevision;
            check(refuses(() => session.ApplySceneCandidate(snapshot, foreign)) && session.ProjectRevision().Equals(token),
                "different-session-token-refused-even-with-matching-generations");
            var otherHistory = new History(); var otherRecorder = new EditRecorder(other, otherHistory, () => true);
            var otherEditor = new SceneDocumentEditor(other, new Selection(), otherHistory, otherRecorder, () => false, () => { });
            check(refuses(() => otherEditor.Commit("foreign prepared owner", pending)), "prepared-document-cannot-cross-coordinators");
        }
        session.Edits.GestureActive = () => true;
        check(refuses(() => editor.Commit("global gesture", pending)) && session.ProjectRevision().Equals(token),
            "global-gesture-admission-covers-prepared-publication");
        session.Edits.GestureActive = null;
        selection.SetWorld();
        editor.Commit("current view target", pending);
        check(selection.IsWorld && !session.Edits.IsBlocked, "prepared-publication-captures-current-selection-and-releases-gate");
        using (session.Edits.BeginCandidate(false))
            check(refuses(() => session.Set(session.Entities()[0], transform, position, default)) &&
                refuses(() => history.Record(new FieldStep("blocked bookkeeping", []))),
                "serial-preparation-lease-blocks-mutations-before-world-and-history");
        check(!session.Edits.IsBlocked, "uncommitted-preparation-lease-releases-admission");
        var largeFolder = Path.Combine(work, "snapshot-growth");
        EngineSession.CreateProject(largeFolder);
        var largePath = Directory.GetFiles(Path.Combine(largeFolder, "scenes"), "*.scene").Single();
        var large = document.DeepClone(); large["qualificationPadding"] = new string('x', 70000);
        File.WriteAllText(largePath, large.ToJsonString());
        using var growth = EngineSession.OpenProject(640, 480, largeFolder);
        var captured = growth.ProjectSnapshot();
        check(growth.LastSnapshotNativeCalls == 2 && captured.Contains(new string('x', 70000)) &&
            growth.ProjectSnapshot() == captured && growth.LastSnapshotNativeCalls == 1,
            "snapshot-overflow-retries-once-then-retains-capacity-without-stale-data");
        growth.ApplySceneCandidate(document.ToJsonString(), growth.ProjectRevision());
        var smaller = growth.ProjectSnapshot();
        check(growth.LastSnapshotNativeCalls == 1 && !smaller.Contains("qualificationPadding") && JsonNode.Parse(smaller) != null &&
            growth.SnapshotBufferCapacity > Encoding.UTF8.GetByteCount(smaller),
            "shrinking-snapshot-uses-returned-length-without-old-buffer-tail");
    }
}
