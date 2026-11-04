using System.Diagnostics;
using System.IO.Pipes;
using System.Numerics;
using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Development;
using PixieEditor.Editing;
using PixieEditor.Native;
using PixieEditor.Ui;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

internal static class SceneAuthoringTests
{
    internal static unsafe int Run(string native, string work)
    {
        Directory.CreateDirectory(work);
        var checks = new List<string>();
        void Check(bool ok, string name) { if (!ok) throw new InvalidOperationException(name); checks.Add(name); Console.WriteLine("PASS " + name); }
        bool Refuses(Action action) { try { action(); return false; } catch (EngineException) { return true; } }
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(1280, 900, "Scene authoring qualification", default, default);
        if (window.Handle == null) { GLFW.Terminate(); return 4; }
        GLFW.MakeContextCurrent(window); Gl.Load(); GameLibrary.Use(Path.GetFullPath(native));
        var clear = (delegate* unmanaged[Stdcall]<uint, void>)GLFW.GetProcAddress("glClear");
        var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        var io = ImGui.GetIO(); io.IniFilename = (byte*)null;
        ImGui.StyleColorsDark(); ImGuiImplGLFW.SetCurrentContext(context);
        ImGuiImplGLFW.InitForOpenGL(new BackendWindow((Hexa.NET.ImGui.Backends.GLFW.GLFWwindow*)window.Handle), true);
        ImGuiImplOpenGL3.SetCurrentContext(context); ImGuiImplOpenGL3.Init("#version 450");
        try
        {
            var folder = Path.Combine(Path.GetFullPath(work), "project"); EngineSession.CreateProject(folder);
            var path = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single();
            var legacyDocument = JsonNode.Parse(File.ReadAllText(path))!;
            legacyDocument["qualificationRoot"] = new JsonObject { ["retained"] = true };
            legacyDocument["entities"]![0]!["components"]!["FutureReference"] = new JsonObject
                { ["v"] = 7, ["target"] = legacyDocument["entities"]![0]!["id"]!.DeepClone() };
            File.WriteAllText(path, legacyDocument.ToJsonString());
            var original = File.ReadAllBytes(path);
            using var session = EngineSession.OpenProject(640, 480, folder);
            var selection = new Selection(); var history = new History();
            var authoring = new Authoring(session, selection, history);
            var recorder = new EditRecorder(session, history, () => authoring.Recording);
            using var inspector = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
            var failRebind = false;
            var editor = new SceneDocumentEditor(session, selection, history, recorder, () => false,
                () => { if (failRebind) throw new InvalidOperationException("injected binding failure"); inspector.Rebind(selection); });
            var scenes = new SceneAuthoring(session, selection, history, editor);
            authoring.SceneEditor = editor; authoring.Scenes = scenes;
            var entitiesPanel = new EntitiesPanel(session, selection, authoring);
            var viewport = new ViewportPanel(session, selection, authoring, new AssetChooser(folder)).Started();
            viewport.Mode = ViewportMode.Select;
            var toolbar = new Toolbar(session, viewport, authoring, selection);
            viewport.DrawControls = toolbar.DrawViewportControls;
            var inspectorWidth = 410f;
            void Frame(string? capture = null, Action? input = null)
            {
                GLFW.PollEvents(); ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); input?.Invoke(); ImGui.NewFrame();
                scenes.ProcessPending();
                ImGui.SetNextWindowPos(new Vector2(0, 20)); ImGui.SetNextWindowSize(new Vector2(280, 850)); entitiesPanel.Draw();
                ImGui.SetNextWindowPos(new Vector2(285, 20)); ImGui.SetNextWindowSize(new Vector2(980 - inspectorWidth, 850)); viewport.Draw(0.016);
                ImGui.SetNextWindowPos(new Vector2(1270 - inspectorWidth, 20)); ImGui.SetNextWindowSize(new Vector2(inspectorWidth, 850)); inspector.Draw(selection);
                ImGui.Render(); clear(0x00004000); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
                if (capture != null) Screenshot.Save(Path.Combine(work, capture + ".png"), 1280, 900);
            }
            void Click(Vector2 point)
            {
                Frame(input: () => io.AddMousePosEvent(point.X, point.Y));
                Frame(input: () => io.AddMouseButtonEvent(0, true));
                Frame(input: () => io.AddMouseButtonEvent(0, false)); Frame(); Frame();
            }
            Frame(); Frame("v1-conversion-required");
            Check(scenes.NeedsConversion && Refuses(() => scenes.Create("premature")) && !session.Unsaved && history.UndoCount == 0 &&
                File.ReadAllBytes(path).SequenceEqual(original), "v1-authoring-requires-explicit-conversion-before-any-world-or-file-edit");
            var firstId = session.SceneId(session.Entities()[0])!.Value;
            selection.Set(session.Find(firstId)!.Value);
            authoring.Rename(selection.Items.Single(), "unsaved before conversion");
            var beforeConversionToken = session.ProjectRevision(); var beforeConversionHistory = history.Revision;
            File.WriteAllText(path + ".v1-backup", "reserved");
            Check(Refuses(scenes.Convert) && session.ProjectRevision().Equals(beforeConversionToken) && session.Unsaved &&
                history.Revision == beforeConversionHistory && session.SceneFormatVersion == 1 && File.ReadAllBytes(path).SequenceEqual(original),
                "conversion-backup-conflict-preserves-world-dirty-history-token-and-original");
            File.Delete(path + ".v1-backup");
            scenes.Convert();
            Check(Refuses(() => session.ApplySceneCandidate(session.ProjectSnapshot(), beforeConversionToken)), "successful-conversion-expires-an-old-native-content-token");
            Check(session.SceneFormatVersion == 2 && !session.Unsaved && session.ProjectRevision().WorldRevision == beforeConversionToken.WorldRevision &&
                session.ProjectRevision().AuthoredRevision > beforeConversionToken.AuthoredRevision && history.Revision == beforeConversionHistory &&
                File.ReadAllBytes(path + ".v1-backup").SequenceEqual(original) && File.ReadAllText(path).Contains("unsaved before conversion"),
                "explicit-conversion-saves-current-authorship-preserves-original-and-invalidates-content-only");
            authoring.Undo();
            Check(session.SceneFormatVersion == 2 && session.EntityName(session.Find(firstId)!.Value) != "unsaved before conversion",
                "old-v1-entity-history-remains-usable-without-format-undo");
            authoring.Redo();
            var disk = File.ReadAllBytes(path);
            var scope = scenes.Create("Room A");
            Check(scope != 0 && selection.Scope == scope && !selection.IsWorld && selection.Items.Count == 0 && session.Unsaved,
                "create-empty-scene-selects-distinct-scope-target");
            var environment = session.ViewportEnvironmentSelection;
            Frame(); Frame("scene-properties");
            Check(session.ViewportEnvironmentSelection == environment, "scene-inspection-does-not-select-viewport-environment");
            authoring.Undo(); Check(session.SceneScopes().Length == 0 && selection.Scope == null, "normal-authoring-undo-routes-candidate-scene-step");
            authoring.Redo(); Check(session.SceneScopes().Single().Id == scope && selection.Scope == scope, "normal-authoring-redo-restores-scene-identity-and-target");
            scenes.Rename(scope, "Room B");
            Check(session.SceneScopes().Single().Name == "Room B", "scene-rename-preserves-identity");
            var annotated = JsonNode.Parse(session.ProjectSnapshot())!;
            annotated["sceneScopes"]![0]!["futureScope"] = new JsonObject { ["retained"] = 23 };
            editor.Apply("unknown scope metadata fixture", annotated.ToJsonString());
            selection.Set(session.Find(firstId)!.Value);
            authoring.Rename(selection.Items.Single(), "replacement in World");
            var second = authoring.CreateEntity()!.Value; var secondId = session.SceneId(second)!.Value;
            selection.Set([session.Find(firstId)!.Value, second]);
            scenes.Transfer([firstId, secondId], scope);
            Check(selection.Items.Count == 2 && session.EntityScope(session.Find(firstId)!.Value) == scope &&
                session.EntityScope(session.Find(secondId)!.Value) == scope, "batch-transfer-preserves-ids-and-multiselection");
            Frame(); Frame("entity-membership");
            session.Edits.GestureActive = () => recorder.HasPendingEdits || inspector.HasActiveAuthoringGesture ||
                viewport.HasActiveAuthoringGesture || ImGui.IsAnyItemActive();
            Click(new Vector2(1125, 73)); Frame("membership-popup"); Click(new Vector2(950, 100));
            Check(session.EntityScope(session.Find(firstId)!.Value) == 0 && session.EntityScope(session.Find(secondId)!.Value) == 0,
                "actual-membership-combo-transfers-the-whole-selection-to-world");
            var sameMembershipHistory = history.UndoCount;
            Click(new Vector2(1125, 73)); Click(new Vector2(950, 100));
            Check(history.UndoCount == sameMembershipHistory, "selecting-current-world-membership-does-not-rebuild-or-add-history");
            Click(new Vector2(1125, 73)); Click(new Vector2(950, 117));
            Check(session.EntityScope(session.Find(firstId)!.Value) == scope && session.EntityScope(session.Find(secondId)!.Value) == scope,
                "actual-membership-combo-transfers-the-whole-selection-to-scene");
            sameMembershipHistory = history.UndoCount;
            Click(new Vector2(1125, 73)); Click(new Vector2(950, 117));
            Check(history.UndoCount == sameMembershipHistory, "selecting-current-scene-membership-does-not-rebuild-or-add-history");
            session.Edits.GestureActive = null;
            var occupied = session.ProjectSnapshot(); var count = history.UndoCount; var token = session.ProjectRevision();
            Check(Refuses(() => scenes.Remove(scope)) && occupied == session.ProjectSnapshot() && history.UndoCount == count && session.ProjectRevision().Equals(token),
                "occupied-removal-refuses-without-membership-or-history-changes");
            Check(Refuses(() => scenes.Transfer([firstId, ulong.MaxValue], 0)) && occupied == session.ProjectSnapshot() && history.UndoCount == count,
                "partly-missing-transfer-refuses-integrally");
            Check(Refuses(() => scenes.Transfer([firstId, firstId], 0)) && occupied == session.ProjectSnapshot(), "duplicate-transfer-identities-refuse-integrally");
            authoring.Delete([session.Find(secondId)!.Value]);
            Check(session.Find(secondId) == null, "ordinary-entity-deletion-after-membership-change");
            authoring.Undo(); Check(session.EntityScope(session.Find(secondId)!.Value) == scope, "entity-replacement-undo-retains-scene-membership");
            authoring.Undo(); Check(session.EntityScope(session.Find(firstId)!.Value) == 0 && session.EntityScope(session.Find(secondId)!.Value) == 0,
                "mixed-history-undo-routes-transfer-after-entity-step");
            authoring.Redo(); authoring.Redo();
            Check(session.Find(secondId) == null && session.EntityScope(session.Find(firstId)!.Value) == scope, "mixed-history-redo-restores-transfer-and-deletion-order");
            scenes.Transfer([firstId], 0);
            scenes.Select(scope); session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, scope);
            scenes.Remove(scope);
            Check(selection.IsWorld && session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.RemovedSceneFallback,
                "empty-removal-falls-back-inspection-and-retains-viewport-intent");
            authoring.Undo();
            Check(selection.Scope == scope && session.ViewportEnvironmentSelection.SceneId == scope &&
                session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld,
                "undo-removal-restores-scene-target-and-explicit-environment-identity");
            authoring.Redo(); Check(selection.IsWorld && session.SceneScopes().Length == 0, "redo-removal-restores-world-inspection-fallback");
            authoring.Undo();
            selection.PreserveThrough(session, session.Play);
            Check(selection.Scope == scope && Refuses(() => scenes.Create("during Play")), "play-preserves-scope-selection-and-refuses-authoring");
            selection.PreserveThrough(session, session.Stop);
            Check(selection.Scope == scope, "stop-preserves-scope-selection");
            session.Save();
            Check(File.ReadAllBytes(path + ".v1-backup").SequenceEqual(original) && !File.ReadAllBytes(path).SequenceEqual(disk),
                "v2-save-persists-scene-catalog-with-original-v1-backup-untouched");
            using (var reopened = EngineSession.OpenProject(640, 480, folder))
                Check(reopened.SceneScopes().Single().Id == scope, "saved-scene-catalog-reopens");
            scenes.Rename(scope, "unsaved scope");
            var handoff = CodeReopen.Capture(session, selection.Items, null, selection.Anchor, selection.IsWorld, selection.Scope);
            using (var rebuilt = EngineSession.OpenProject(640, 480, folder))
            {
                var state = CodeReopen.Restore(rebuilt, handoff);
                Check(state.Scope == scope && rebuilt.Unsaved && rebuilt.SceneScopes().Single().Name == "unsaved scope", "unsaved-scope-and-inspection-target-survive-code-handoff");
            }
            session.Edits.GestureActive = () => true;
            var pendingCount = history.UndoCount;
            scenes.Queue("create", name: "queued Scene"); scenes.ProcessPending();
            Check(scenes.HasPending && history.UndoCount == pendingCount, "queued-ui-intent-waits-for-active-gesture");
            session.Edits.GestureActive = () => false;
            scenes.ProcessPending();
            Check(!scenes.HasPending && session.SceneScopes().Length == 2 && history.UndoCount == pendingCount + 1, "queued-ui-intent-publishes-once-after-gesture");
            scenes.Queue("rename", scope, "expired"); session.SetName(session.Find(firstId)!.Value, "intervening edit"); scenes.ProcessPending();
            Check(session.SceneScopes().First(s => s.Id == scope).Name == "unsaved scope" && Status.IsError, "queued-intent-expires-after-intervening-authored-edit");
            scenes.Queue("create", name: "expired history"); history.Record(new FieldStep("history-only test boundary", [])); scenes.ProcessPending();
            Check(session.SceneScopes().Length == 2 && Status.IsError, "queued-intent-expires-after-history-only-change");
            failRebind = true; scenes.Create("committed with recovery");
            Check(editor.RecoveryRequired && Refuses(() => scenes.Create("blocked")), "committed-scene-authoring-enters-shared-recovery");
            failRebind = false; Check(editor.RecoverBindings(), "scene-inspector-bindings-recover-without-nested-render");
            Frame(); Frame("scenes-complete");
            session.Edits.GestureActive = () => recorder.HasPendingEdits || inspector.HasActiveAuthoringGesture ||
                viewport.HasActiveAuthoringGesture || ImGui.IsAnyItemActive();
            var inspectedScope = selection.Scope!.Value;
            var beforeTyping = history.UndoCount;
            Click(new Vector2(1120, 121));
            Frame(input: () => io.AddInputCharactersUTF8(" via"));
            Frame(input: () => io.AddInputCharactersUTF8(" UI"));
            Check(scenes.HasPending && history.UndoCount == beforeTyping, "real-scene-name-input-coalesces-without-publishing-during-typing");
            Click(new Vector2(80, 77)); // The rendered World-properties row, outside the Inspector input.
            Check(selection.IsWorld && session.SceneScopes().First(s => s.Id == inspectedScope).Name.EndsWith(" via UI") &&
                history.UndoCount == beforeTyping + 1, "real-target-navigation-commits-typed-scene-name-once-by-stable-id");
            scenes.Select(inspectedScope); Frame(); Frame();
            Click(new Vector2(1120, 121)); Frame(input: () => io.AddInputCharactersUTF8(" collapsed"));
            Click(new Vector2(870, 30));
            Check(!scenes.HasPending && !inspector.HasActiveAuthoringGesture &&
                session.SceneScopes().First(s => s.Id == inspectedScope).Name.EndsWith(" collapsed"),
                "collapsing-inspector-finishes-name-gesture-without-an-admission-latch");
            Click(new Vector2(870, 30));
            var cancelName = session.SceneScopes().First(s => s.Id == inspectedScope).Name;
            var cancelHistory = history.UndoCount;
            Click(new Vector2(1120, 121)); Frame(input: () => io.AddInputCharactersUTF8(" cancelled"));
            Frame(input: () => io.AddKeyEvent(ImGuiKey.Escape, true)); Frame(input: () => io.AddKeyEvent(ImGuiKey.Escape, false)); Frame(); Frame();
            Check(session.SceneScopes().First(s => s.Id == inspectedScope).Name == cancelName && history.UndoCount == cancelHistory,
                "escape-cancels-scene-name-without-a-noop-history-entry");
            var priorUiCount = session.SceneScopes().Length; var priorEnvironment = session.ViewportEnvironmentSelection;
            Click(new Vector2(33, 170));
            Check(session.SceneScopes().Length == priorUiCount + 1 && selection.Scope != inspectedScope,
                "actual-scene-create-button-queues-and-publishes-a-scene");
            Click(new Vector2(940, 143));
            Check(session.SceneScopes().Length == priorUiCount && selection.IsWorld && session.ViewportEnvironmentSelection == priorEnvironment,
                "actual-empty-scene-remove-button-preserves-entities-and-viewport-policy");
            scenes.Select(inspectedScope); Frame(); Frame("scenes-input-qualified");
            var pipeName = "scene-authoring-" + Guid.NewGuid().ToString("N");
            using (var channel = new EditorChannel(pipeName, Path.Combine(work, "captures"), session, selection, inspector, history,
                command => { var at = command.IndexOf('='); if (command == "undo") { authoring.Undo(); return true; }
                    if (command.StartsWith("select=", StringComparison.Ordinal)) { selection.Set(session.Find(ulong.Parse(command[7..], System.Globalization.NumberStyles.HexNumber))!.Value); return true; }
                    return scenes.Run(at < 0 ? command : command[..at], at < 0 ? "" : command[(at + 1)..]); }))
            using (var client = new NamedPipeClientStream(".", "pixie-" + pipeName, PipeDirection.InOut, PipeOptions.Asynchronous))
            {
                client.Connect(5000);
                using var writer = new StreamWriter(client, leaveOpen: true) { AutoFlush = true };
                using var reader = new StreamReader(client, leaveOpen: true);
                JsonNode Request(string method, JsonObject? p = null)
                {
                    writer.WriteLine(new JsonObject { ["jsonrpc"] = "2.0", ["id"] = 1, ["method"] = method, ["params"] = p ?? new() }.ToJsonString());
                    var read = reader.ReadLineAsync().WaitAsync(TimeSpan.FromSeconds(5));
                    var timer = Stopwatch.StartNew(); while (!read.IsCompleted && timer.Elapsed < TimeSpan.FromSeconds(5)) { channel.Drain(); Thread.Sleep(1); }
                    return JsonNode.Parse(read.GetAwaiter().GetResult()!)!;
                }
                Check(Request("world.hello")["result"] != null, "scene-channel-handshake");
                var liveBefore = session.ProjectSnapshot(); var liveRevision = session.ProjectRevision();
                var savedToken = Request("world.snapshot", new() { ["ids"] = new JsonArray(firstId.ToString("x16")) })["result"]!["value"]!["token"]!.GetValue<string>();
                Check(Request("world.restore", new() { ["token"] = savedToken })["error"] != null &&
                    session.ProjectSnapshot() == liveBefore && session.ProjectRevision().Equals(liveRevision) && session.Find(firstId) != null,
                    "v2-raw-channel-restore-refuses-before-destroying-live-entities");
                var tree = Request("inspector.tree");
                Check(tree["result"]!["value"]!["target"]!.GetValue<string>() == "scene", "channel-observes-scene-target-without-rebinding-as-empty-entities");
                var generation = inspector.ChannelGeneration;
                Check(Request("inspector.set", new() { ["generation"] = generation, ["path"] = "Transform2D/positionX", ["value"] = 99 })["error"] != null,
                    "scene-target-refuses-entity-inspector-write");
                var listed = Request("editor.scenes");
                Check(listed["result"]!["value"]!["scopes"]!.AsArray().Count == 3, "channel-enumerates-stable-scene-catalog");
                var receipt = Request("editor.do", new() { ["generation"] = generation, ["command"] = "scenecreate=from channel" });
                Check(receipt["result"] != null && session.SceneScopes().Length == 4 && inspector.ChannelGeneration > generation,
                    "real-channel-scene-command-uses-coordinator-and-invalidates-generation");
                Check(Request("editor.do", new() { ["generation"] = generation, ["command"] = "scenecreate=stale" })["error"] != null && session.SceneScopes().Length == 4,
                    "stale-channel-scene-command-refuses-before-another-publication");
                Check(Request("editor.do", new() { ["generation"] = inspector.ChannelGeneration, ["command"] = "undo" })["result"] != null && session.SceneScopes().Length == 3,
                    "channel-undo-routes-scene-history-from-scope-target");
                Check(Request("editor.do", new() { ["generation"] = inspector.ChannelGeneration, ["command"] = "select=" + firstId.ToString("x16") })["result"] != null && selection.Items.Count == 1,
                    "channel-navigation-leaves-scope-target-for-fresh-entity-bindings");
                Check(Request("editor.do", new() { ["generation"] = inspector.ChannelGeneration, ["command"] = "scenecreate=from entity target" })["result"] != null &&
                    selection.Scope != null && session.SceneScopes().Length == 4, "channel-scene-publication-retires-bound-entity-inspectors");
                Check(Request("editor.do", new() { ["generation"] = inspector.ChannelGeneration, ["command"] = "undo" })["result"] != null && session.SceneScopes().Length == 3,
                    "channel-candidate-undo-restores-entity-selection");
            }
            var finalDocument = session.ProjectSnapshot();
            var undoCount = history.UndoCount;
            for (var i = 0; i < undoCount; ++i) authoring.Undo();
            Check(history.UndoCount == 0 && session.SceneScopes().Length == 0 && session.SceneFormatVersion == 2,
                "complete-mixed-history-can-return-to-converted-world-without-undoing-format");
            for (var i = 0; i < undoCount; ++i) authoring.Redo();
            Check(history.UndoCount == undoCount && session.ProjectSnapshot() == finalDocument,
                "complete-mixed-history-redo-restores-authorship-exactly");
            var preserved = JsonNode.Parse(session.ProjectSnapshot())!;
            Check(preserved["qualificationRoot"]!["retained"]!.GetValue<bool>() &&
                preserved["sceneScopes"]!.AsArray().Any(s => s!["futureScope"]?["retained"]?.GetValue<int>() == 23) &&
                preserved["entities"]!.AsArray().Single(e => e!["id"]!.GetValue<string>() == firstId.ToString("x16"))!["components"]!["FutureReference"]!["v"]!.GetValue<int>() == 7,
                "scene-operations-and-history-preserve-unknown-root-scope-and-entity-payloads");
            // Clean labels for independent owner presentation review; no appearance acceptance implied.
            var reviewScopes = session.SceneScopes();
            for (var i = 0; i < reviewScopes.Length; ++i) scenes.Rename(reviewScopes[i].Id, "Room " + (i + 1));
            scenes.Select(reviewScopes[0].Id); Frame(); Frame("review-scene-properties");
            inspectorWidth = 320; Frame(); Frame("review-scene-properties-320");
            inspectorWidth = 410; selection.Set(session.Find(firstId)!.Value); Frame(); Frame("review-membership");
            var beforeDuplicate = session.ProjectSnapshot();
            var duplicateHistory = history.UndoCount;
            Check(authoring.Duplicate([session.Find(firstId)!.Value]), "v2-entity-duplicate-publishes-one-complete-candidate");
            var copiedId = session.SceneId(selection.Items.Single())!.Value;
            var afterDuplicate = session.ProjectSnapshot();
            Check(copiedId != firstId && history.UndoCount == duplicateHistory + 1,
                "v2-duplicate-has-new-identity-and-one-history-entry");
            authoring.Undo();
            Check(session.ProjectSnapshot() == beforeDuplicate && session.Find(copiedId) == null,
                "v2-duplicate-undo-restores-complete-document");
            authoring.Redo();
            Check(session.ProjectSnapshot() == afterDuplicate && session.Find(copiedId) != null,
                "v2-duplicate-redo-keeps-the-same-authored-identity");
            var referenceFixture = JsonNode.Parse(session.ProjectSnapshot())!;
            var target = referenceFixture["entities"]!.AsArray().Single(e => e!["id"]!.GetValue<string>() == firstId.ToString("x16"))!;
            target["components"]!["ParticleSystem"] = new JsonObject { ["v"] = 1 };
            referenceFixture["entities"]!.AsArray().Add(new JsonObject
            {
                ["id"] = "ffffffffffffffe1", ["components"] = new JsonObject
                {
                    ["ParticleSpawner"] = new JsonObject { ["v"] = 1, ["system"] = firstId.ToString("x16") }
                }
            });
            editor.Apply("outside-reference fixture", referenceFixture.ToJsonString());
            var referenceBefore = session.ProjectSnapshot();
            authoring.Delete([session.Find(firstId)!.Value]);
            Check(session.Find(firstId) == null, "v2-deletion-publishes-complete-document");
            authoring.Undo();
            Check(session.ProjectSnapshot() == referenceBefore,
                "v2-deletion-undo-restores-references-from-outside-the-deleted-set");
            var refusedBefore = session.ProjectSnapshot(); var refusedHistory = history.UndoCount; var refusedToken = session.ProjectRevision();
            Check(Refuses(() => editor.EditFragment("bad import", "{broken", [], false)) &&
                session.ProjectSnapshot() == refusedBefore && history.UndoCount == refusedHistory && session.ProjectRevision().Equals(refusedToken),
                "v2-bad-fragment-retains-world-history-and-content-token");
            File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks,
                limitations = "Hidden real GL/ImGui frames and real pipe; presentation acceptance remains with the owner. No worker or code-project candidate activation." }, new JsonSerializerOptions { WriteIndented = true }));
            return 0;
        }
        finally
        {
            ImGuiImplOpenGL3.Shutdown(); ImGuiImplGLFW.Shutdown(); ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            ImGui.DestroyContext(context); GLFW.DestroyWindow(window); GLFW.Terminate();
        }
    }
}
