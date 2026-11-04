using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using PixieEditor.Editing;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieEditor.Ui;

internal static class ModuleSceneTests
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int ProbeState(int query, int value);

    internal static unsafe int Run(string module, string work)
    {
        Directory.CreateDirectory(work);
        var checks = new List<string>();
        void Check(bool ok, string name) { if (!ok) throw new InvalidOperationException(name); checks.Add(name); Console.WriteLine("PASS " + name); }
        bool Refuses(Action operation) { try { operation(); return false; } catch (EngineException) { return true; } }
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(640, 480, "Contracted module qualification", default, default);
        if (window.Handle == null) return 4;
        GLFW.MakeContextCurrent(window); Gl.Load(); GameLibrary.Use(Path.GetFullPath(module));
        var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        var io = ImGui.GetIO(); io.IniFilename = (byte*)null;
        nint auditLease = NativeLibrary.Load(Path.GetFullPath(module));
        var probe = Marshal.GetDelegateForFunctionPointer<ProbeState>(NativeLibrary.GetExport(auditLease, "pixie_probe_state"));
        try
        {
            var folder = Path.Combine(Path.GetFullPath(work), "project"); EngineSession.CreateProject(folder);
            var path = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single();
            var document = JsonNode.Parse(File.ReadAllText(path))!;
            document["version"] = 1;
            document["entities"]![0]!["components"]!["ProbeData"] = new JsonObject { ["v"] = 1, ["value"] = 12.5, ["future"] = 42 };
            File.WriteAllText(path, document.ToJsonString());
            var legacyBytes = File.ReadAllBytes(path);
            using var session = EngineSession.OpenProject(640, 480, folder);
            _ = session.CandidateEditRevision();
            var selection = new Selection(); var history = new History();
            var authoring = new Authoring(session, selection, history);
            var recorder = new EditRecorder(session, history, () => authoring.Recording);
            using var inspector = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
            var failBinding = false; var gesture = false;
            session.Edits.GestureActive = () => gesture;
            var editor = new SceneDocumentEditor(session, selection, history, recorder, () => gesture,
                () => { if (failBinding) throw new InvalidOperationException("injected module binding fault"); inspector.Rebind(selection); });
            var scenes = new SceneAuthoring(session, selection, history, editor);
            authoring.SceneEditor = editor; authoring.Scenes = scenes;
            var unsupportedType = session.TypeIndex("UnsupportedProbe");
            var firstAuthored = session.Entities().First(e => session.SceneId(e) != null);
            Check(session.ComponentAdditionError(firstAuthored, unsupportedType).Length == 0,
                "v1-legacy-component-add-admission-is-preserved");
            var conversionToken = session.ProjectRevision();
            Check(scenes.NeedsConversion && Refuses(() => scenes.Create("before conversion"))
                && session.ProjectRevision().Equals(conversionToken), "contracted-module-requires-explicit-conversion");
            scenes.Convert();
            Check(session.SceneFormatVersion == 2 && session.ProjectRevision().WorldRevision == conversionToken.WorldRevision
                && File.ReadAllBytes(path + ".v1-backup").SequenceEqual(legacyBytes), "contracted-conversion-keeps-original-backup-and-world");
            var id = session.Entities().Select(session.SceneId).OfType<ulong>().First();
            uint Entity() => session.Find(id) ?? throw new InvalidOperationException("Authored identity lost.");
            var addToken = session.ProjectRevision(); var constructors = probe(6, 0);
            Check(Refuses(() => session.AddComponent(Entity(), unsupportedType))
                && session.ProjectRevision().Equals(addToken) && probe(6, 0) == constructors
                && !session.Has(Entity(), unsupportedType), "v2-component-add-refuses-before-construction-or-authored-mutation");
            Check(session.ComponentAdditionError(Entity(), unsupportedType).Contains("UnsupportedProbe"),
                "v2-component-menu-reports-the-unsupported-type");
            var type = session.TypeIndex("ProbeData");
            Check(session.Get(Entity(), type, 0).X == 12.5, "contracted-module-reads-declarative-data");
            foreach (var (field, invalid) in new[] { (1, 1e100), (1, 1.5), (2, -1.0), (3, 1e100), (0, double.NaN) })
            {
                var retained = session.Get(Entity(), type, field);
                Check(Refuses(() => session.Set(Entity(), type, field, new PixieValue { X = invalid }))
                    && session.Get(Entity(), type, field).X == retained.X, $"declarative-native-setter-rejects-{field}-{invalid}");
            }
            var scope = scenes.Create("module room"); scenes.Transfer([id], scope); scenes.Rename(scope, "renamed room");
            Check(session.EntityScope(Entity()) == scope, "contracted-module-creates-renames-and-transfers");
            Check(session.ProjectSnapshot().Contains("future"), "contracted-data-keeps-unknown-fields");
            Check(Refuses(() => scenes.Transfer([0], scope)), "generated-entity-has-no-authored-transfer-identity");
            var before = session.Get(Entity(), type, 0);
            session.Set(Entity(), type, 0, new PixieValue { X = 31 });
            history.Record(new FieldStep("custom data", [new(id, type, 0, before, new PixieValue { X = 31 })]));
            scenes.Transfer([id], 0);
            authoring.Undo(); authoring.Undo();
            Check(session.EntityScope(Entity()) == scope && session.Get(Entity(), type, 0).X == 12.5, "custom-scalar-and-candidate-history-undo");
            authoring.Redo(); authoring.Redo();
            Check(session.EntityScope(Entity()) == 0 && session.Get(Entity(), type, 0).X == 31, "custom-scalar-and-candidate-history-redo");
            selection.PreserveThrough(session, session.Reset); inspector.Rebind(selection);
            Check(session.Get(Entity(), type, 0).X == 31 && probe(3, 0) == 1, "contracted-reset-reconstructs-without-live-capture");
            var gestureToken = session.ProjectRevision(); var gestureHistory = history.Revision; var gestureCalls = probe(5, 0);
            gesture = true;
            Check(Refuses(() => scenes.Create("gesture refusal")) && Refuses(editor.Undo)
                && session.ProjectRevision().Equals(gestureToken) && history.Revision == gestureHistory && probe(5, 0) == gestureCalls,
                "contracted-module-refuses-edit-and-history-during-gesture");
            gesture = false;
            selection.Set(Entity()); inspector.Rebind(selection);
            var survivingBinding = new ComponentHandle(session, Entity(), type, recorder);
            var oldProject = probe(1, 0); var oldState = probe(2, oldProject);
            foreach (var fault in new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9 })
            {
                var snapshot = session.ProjectSnapshot(); var token = session.ProjectRevision();
                var revision = history.Revision; var disk = File.ReadAllBytes(path); var dirty = session.Unsaved;
                probe(0, fault);
                try { Check(Refuses(() => editor.Apply("injected module failure", snapshot)), $"module-fault-{fault}-refused"); }
                finally { probe(0, 0); }
                probe(0, fault);
                try { Check(Refuses(() => editor.EditFragment("failed fragment", session.Snapshot([Entity()]), [], true)),
                    $"module-fault-{fault}-fragment-candidate-refused"); }
                finally { probe(0, 0); }
                Check(session.ProjectRevision().Equals(token) && history.Revision == revision && session.Unsaved == dirty,
                    $"module-fault-{fault}-retains-token-history-and-dirty");
                Check(session.ProjectSnapshot() == snapshot && File.ReadAllBytes(path).SequenceEqual(disk)
                    && probe(2, oldProject) == oldState && probe(3, 0) == 1 && survivingBinding.Get(0).X == 31
                    && selection.Items.SequenceEqual([Entity()]), $"module-fault-{fault}-retains-world-project-inspector-selection-and-file");
            }
            var unsupported = JsonNode.Parse(session.ProjectSnapshot())!;
            unsupported["entities"]![0]!["components"]!["UnsupportedProbe"] = new JsonObject { ["v"] = 1 };
            var calls = probe(5, 0);
            Check(Refuses(() => editor.Apply("unsupported", unsupported.ToJsonString())) && probe(5, 0) == calls,
                "unsupported-component-refused-before-candidate-factory");
            var retainedSnapshot = session.ProjectSnapshot(); var retainedToken = session.ProjectRevision();
            var badScalar = JsonNode.Parse(retainedSnapshot)!;
            badScalar["entities"]![0]!["components"]!["ProbeData"]!["count"] = 1e100;
            Check(Refuses(() => editor.Apply("invalid scalar", badScalar.ToJsonString()))
                && session.ProjectSnapshot() == retainedSnapshot && session.ProjectRevision().Equals(retainedToken)
                && probe(2, oldProject) == oldState && probe(3, 0) == 1, "invalid-declarative-input-retains-published-bundle");
            selection.SetScope(scope);
            session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, 0xdead);
            selection.PreserveThrough(session, session.Play);
            Check(Refuses(() => survivingBinding.Get(0)), "module-publication-retires-old-entity-inspector-binding");
            var playProject = probe(1, 0); var playState = probe(2, playProject);
            session.Step();
            Check(probe(2, playProject) > playState, "contracted-play-executes-project-owned-system");
            var playToken = session.ProjectRevision(); var playHistory = history.Revision; var playCalls = probe(5, 0);
            Check(Refuses(() => scenes.Create("play refusal")) && Refuses(editor.Undo)
                && session.ProjectRevision().Equals(playToken) && history.Revision == playHistory && probe(5, 0) == playCalls,
                "contracted-module-refuses-scene-edit-and-history-in-play");
            selection.PreserveThrough(session, session.Stop); inspector.Rebind(selection);
            Check(session.Get(Entity(), type, 0).X == 31 && selection.Scope == scope
                && session.ViewportEnvironmentSelection.SceneId == 0xdead, "custom-data-scope-target-and-environment-survive-play-stop");
            failBinding = true; var tokenBefore = session.ProjectRevision();
            scenes.Create("committed recovery");
            Check(editor.RecoveryRequired && session.ProjectRevision().WorldRevision != tokenBefore.WorldRevision, "module-publication-enters-binding-recovery");
            var recoveryCalls = probe(5, 0);
            Check(Refuses(() => scenes.Create("recovery refusal")) && probe(5, 0) == recoveryCalls,
                "module-recovery-blocks-further-candidate-construction");
            var committed = session.ProjectRevision(); var committedProject = probe(1, 0);
            failBinding = false;
            Check(editor.RecoverBindings() && session.ProjectRevision().Equals(committed) && probe(1, 0) == committedProject,
                "module-recovery-does-not-replay-project-or-world");
            Check(probe(4, 0) == 0 && probe(3, 0) == 1, "old-world-cleanup-precedes-project-destruction");
            EngineSession.Check(PixieCapi.ModuleLoad(null), "clear global module selection");
            NativeLibrary.Free(auditLease); auditLease = 0;
            scenes.Create("pinned session factory");
            Check(probe(4, 0) == 0, "session-factory-pins-module-after-global-and-audit-release");
            session.Save();
            Check(!session.Unsaved && File.ReadAllText(path).Contains("ProbeData"), "contracted-module-saves-custom-data");
            EngineSession.Check(PixieCapi.ModuleLoad(module), "select the saved module for reopening");
            using (var reopened = EngineSession.OpenProject(640, 480, folder))
            {
                var entity = reopened.Find(id) ?? throw new InvalidOperationException("Saved identity lost.");
                Check(reopened.Get(entity, reopened.TypeIndex("ProbeData"), 0).X == 31
                    && reopened.SceneScopes().Any(s => s.Id == scope), "saved-module-reopens-data-and-scene-catalog");
            }
            EngineSession.Check(PixieCapi.ModuleLoad(null), "release saved module selection");
            File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks }));
            return 0;
        }
        finally
        {
            if (auditLease != 0) NativeLibrary.Free(auditLease);
            ImGui.DestroyContext(context);
            GLFW.DestroyWindow(window); GLFW.Terminate();
        }
    }
}
