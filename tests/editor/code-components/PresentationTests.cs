using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using PixieEditor.Editing;
using PixieEditor.Native;
using PixieEditor.Ui;

internal static class PresentationTests
{
    internal static unsafe int Run(string native, string work)
    {
        Directory.CreateDirectory(work); var checks = new List<string>();
        void Check(bool value, string name) { if (!value) throw new InvalidOperationException(name); checks.Add(name); Console.WriteLine("PASS " + name); }
        bool Refuses(Action action) { try { action(); return false; } catch (EngineException) { return true; } }
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(640, 480, "Presentation qualification", default, default);
        if (window.Handle == null) return 4;
        GLFW.MakeContextCurrent(window); Gl.Load(); GameLibrary.Use(Path.GetFullPath(native));
        var context = ImGui.CreateContext(); ImGui.SetCurrentContext(context);
        try
        {
            var folder = Path.Combine(Path.GetFullPath(work), "project"); EngineSession.CreateProject(folder);
            var path = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single();
            var initial = JsonNode.Parse(File.ReadAllText(path))!;
            initial["entities"]![0]!["components"]!["LightingSettings2D"] = new JsonObject
                { ["v"] = 1, ["ambientIntensity"] = 0.3, ["futureLegacy"] = 17, ["bloom"] = true };
            File.WriteAllText(path, initial.ToJsonString()); var original = File.ReadAllBytes(path);
            using var session = EngineSession.OpenProject(640, 480, folder);
            var selection = new Selection(); selection.SetWorld(); var history = new History();
            var authoring = new Authoring(session, selection, history);
            var recorder = new EditRecorder(session, history, () => authoring.Recording);
            using var inspector = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
            var active = false; session.Edits.GestureActive = () => active || recorder.HasPendingEdits;
            var coordinator = new SceneDocumentEditor(session, selection, history, recorder, () => active, () => inspector.Rebind(selection));
            var scenes = new SceneAuthoring(session, selection, history, coordinator);
            authoring.SceneEditor = coordinator; authoring.Scenes = scenes;
            scenes.Convert(); scenes.MigratePresentation();
            Check(session.PresentationSnapshot()["canonical"]!.GetValue<bool>() && selection.IsWorld,
                "explicit-migration-publishes-canonical-authority-and-retains-world-target");
            var after = JsonNode.Parse(session.ProjectSnapshot())!;
            Check(after["presentation"]!["legacySource"]!["futureLegacy"]!.GetValue<int>() == 17 &&
                after["presentation"]!["environment"]!["ambientIntensity"]!.GetValue<double>() == 0.3 &&
                after["entities"]!.AsArray().All(e => e!["components"]?["LightingSettings2D"] == null),
                "migration-retains-legacy-payload-without-a-second-editable-component");
            var entity = session.Entities().First();
            Check(Refuses(() => session.AddComponent(entity, session.TypeIndex("LightingSettings2D"))), "canonical-authority-refuses-legacy-component-addition");
            authoring.Undo(); Check(!session.PresentationSnapshot()["canonical"]!.GetValue<bool>(), "migration-undo-restores-legacy-authority");
            authoring.Redo(); Check(session.PresentationSnapshot()["canonical"]!.GetValue<bool>(), "migration-redo-restores-canonical-authority");
            var model = scenes.PresentationModel(0);
            model["view"]!["exposure"] = 2.0; model["renderer"]!["maxParticleLights"] = 37;
            model["futurePresentation"] = 23; scenes.ApplyPresentation(0, model.ToJsonString());
            var scope = scenes.Create("room"); var local = scenes.PresentationModel(scope);
            local["environment"]!["mode"] = "override"; local["environment"]!["value"]!["ambientIntensity"] = 0.15;
            local["environment"]!["value"]!["futureValue"] = 31; scenes.ApplyPresentation(scope, local.ToJsonString());
            Check(session.ViewportEnvironmentSelection.Kind == PixieEnvironmentSelectionKind.LegacyWorld, "scene-setting-edit-does-not-select-render-environment");
            session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, scope);
            Check(session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneOverride, "explicit-selected-scene-resolves-its-environment-override");
            var camera = authoring.CreateEntity()!.Value; var cameraId = session.SceneId(camera)!.Value;
            session.AddComponent(camera, session.TypeIndex("Camera2D"));
            var transform = session.TypeIndex("Transform2D");
            session.Set(camera, transform, session.FieldIndex(transform, "position"), new PixieValue { X = 20, Y = 30 });
            session.Set(camera, session.TypeIndex("Camera2D"), 0, new PixieValue { X = 2 });
            Check(Refuses(() => session.Set(camera, session.TypeIndex("Camera2D"), 0, new PixieValue { X = 0 })), "invalid-camera-zoom-refuses-without-clamping");
            local = scenes.PresentationModel(scope); local["camera"] = cameraId.ToString("x16"); scenes.ApplyPresentation(scope, local.ToJsonString());
            session.SelectViewportCamera(1, scene: scope);
            Check(session.WorldToView(20, 30) == (320.0, 240.0) && session.ViewToWorld(320, 240) == (20.0, 30.0)
                && session.Pick(320, 240, 0) == session.Find(cameraId), "authored-camera-render-coordinates-and-picking-share-resolution");
            session.FreeCamera = (71, 93, 1.5);
            selection.SetScope(scope);
            var snapshot = session.ProjectSnapshot(); var undo = history.UndoCount;
            active = true;
            Check(Refuses(() => scenes.ApplyPresentation(scope, local.ToJsonString())) && session.ProjectSnapshot() == snapshot && history.UndoCount == undo,
                "presentation-candidate-admission-refuses-active-gesture");
            active = false;
            selection.PreserveThrough(session, session.Play); selection.PreserveThrough(session, session.Stop);
            Check(selection.Scope == scope && session.ViewportCameraSelection == (1, 0UL, scope) && session.FreeCamera == (71.0, 93.0, 1.5),
                "camera-intent-free-navigation-and-scope-survive-play-stop");
            var handoff = CodeReopen.Capture(session, selection.Items, null, selection.Anchor, selection.IsWorld, selection.Scope);
            using (var restored = EngineSession.OpenProject(640, 480, folder))
            {
                var state = CodeReopen.Restore(restored, handoff);
                Check(state.Scope == scope && restored.ViewportCameraSelection == (1, 0UL, scope) && restored.FreeCamera == (71.0, 93.0, 1.5),
                    "full-handoff-restores-scope-camera-choice-and-free-camera");
                var roundTrip = JsonNode.Parse(restored.ProjectSnapshot())!;
                Check(roundTrip["presentation"]!["futurePresentation"]!.GetValue<int>() == 23 &&
                    roundTrip["sceneScopes"]![0]!["environment"]!["value"]!["futureValue"]!.GetValue<int>() == 31 && restored.Unsaved,
                    "full-handoff-restores-view-renderer-environment-and-unknown-authorship");
            }
            Check(File.ReadAllBytes(path + ".v1-backup").SequenceEqual(original), "presentation-history-and-reopen-retain-original-v1-backup");
            session.Save();
            using (var reopened = EngineSession.OpenProject(640, 480, folder))
                Check(reopened.PresentationSnapshot()["canonical"]!.GetValue<bool>() && reopened.SceneScopes().Single().Id == scope,
                    "canonical-presentation-and-scene-settings-save-reopen");
            File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks,
                limitations = "Hidden GL, production candidate/rebind/history/CAPI paths. Owner visual acceptance and actual input execution are separate. AO/normal compatibility remains read-only; no body-gravity activation." }, new JsonSerializerOptions { WriteIndented = true }));
            return 0;
        }
        finally { ImGui.DestroyContext(context); GLFW.DestroyWindow(window); GLFW.Terminate(); }
    }
}
