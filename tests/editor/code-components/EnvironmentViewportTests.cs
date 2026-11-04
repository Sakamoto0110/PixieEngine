using System.Numerics;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Editing;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieEditor.Ui;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

// Real GL/ImGui input and framebuffer evidence; the owner separately accepts presentation.
internal static class EnvironmentViewportTests
{
    internal static unsafe int Run(string native, string work)
    {
        Directory.CreateDirectory(work);
        var checks = new List<string>();
        void Check(bool ok, string name)
        {
            if (!ok) throw new InvalidOperationException(name);
            checks.Add(name); Console.WriteLine("PASS " + name);
        }
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        var window = GLFW.CreateWindow(1280, 900, "Environment qualification", default, default);
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
            var folder = Path.Combine(work, "project-" + Guid.NewGuid().ToString("N"));
            EngineSession.CreateProject(folder);
            string fixture;
            using (var seed = EngineSession.OpenProject(900, 650, folder)) fixture = seed.ProjectSnapshot();
            var document = JsonNode.Parse(fixture)!.AsObject();
            document["version"] = 2;
            document["sceneScopes"] = new JsonArray(new JsonObject { ["id"] = "aa", ["name"] = "Laboratory" });
            // This is a private already-v2 disk fixture, not a product migration or editor Save.
            var scenePath = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single();
            File.WriteAllText(scenePath, document.ToJsonString());
            var disk = File.ReadAllBytes(scenePath);
            using var session = EngineSession.OpenProject(900, 650, folder);
            var entityId = session.SceneId(session.Entities()[0])!.Value;
            var selection = new Selection(); selection.Set(session.Find(entityId)!.Value);
            var history = new History(); var authoring = new Authoring(session, selection, history);
            var viewport = new ViewportPanel(session, selection, authoring, new AssetChooser(folder)).Started();
            viewport.Mode = ViewportMode.Select;
            var toolbar = new Toolbar(session, viewport, authoring, selection);
            Vector2 environmentOrigin = default;
            viewport.DrawControls = () => { toolbar.DrawViewportControls(); environmentOrigin = ImGui.GetCursorScreenPos(); };
            var viewportSize = new Vector2(940, 820);
            var showWorldInspector = false;
            var recorder = new EditRecorder(session, history, () => authoring.Recording);
            using var inspector = new InspectorPanel(session, authoring, recorder, new AssetChooser(folder));
            var editor = new SceneDocumentEditor(session, selection, history, recorder,
                () => inspector.HasActiveAuthoringGesture || viewport.HasActiveAuthoringGesture,
                () => inspector.Rebind(selection));
            authoring.SceneEditor = editor;
            authoring.Scenes = new SceneAuthoring(session, selection, history, editor);
            var sessionPanel = new SessionPanel(session, "pixie.dll")
            {
                InspectWorld = () => { selection.SetWorld(); showWorldInspector = true; viewportSize.X = 620; }
            };
            var entitiesPanel = new EntitiesPanel(session, selection, authoring);
            var choiceMethod = typeof(ViewportPanel).GetMethod("DrawEnvironmentChoice", BindingFlags.Instance | BindingFlags.NonPublic)!;
            var scopeRead = typeof(ViewportPanel).GetField("NextEnvironmentScopeRead", BindingFlags.Instance | BindingFlags.NonPublic)!;
            var probe = false;
            Vector2 comboMin = default, comboMax = default, probeOrigin = default;
            float contentRight = 0, popupPaddingY = 0;
            bool PopupOpen() => ImGui.IsPopupOpen((byte*)null, ImGuiPopupFlags.AnyPopupId | ImGuiPopupFlags.AnyPopupLevel);
            void Frame(Action? input = null)
            {
                GLFW.PollEvents(); ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame();
                input?.Invoke(); ImGui.NewFrame(); toolbar.Draw();
                ImGui.SetNextWindowPos(new Vector2(0, 22)); ImGui.SetNextWindowSize(new Vector2(270, 390)); sessionPanel.Draw();
                ImGui.SetNextWindowPos(new Vector2(0, 415)); ImGui.SetNextWindowSize(new Vector2(270, 420)); entitiesPanel.Draw();
                ImGui.SetNextWindowPos(new Vector2(280, 22)); ImGui.SetNextWindowSize(viewportSize);
                if (probe)
                {
                    // Measure and drive the actual production widget in isolation. World has no
                    // trailing diagnostic, so its last-item rectangle is the combo itself.
                    ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, Vector2.Zero);
                    ImGui.Begin("Environment input fixture", ImGuiWindowFlags.NoScrollbar | ImGuiWindowFlags.NoScrollWithMouse);
                    popupPaddingY = ImGui.GetStyle().WindowPadding.Y;
                    probeOrigin = ImGui.GetCursorScreenPos();
                    choiceMethod.Invoke(viewport, null);
                    if (session.ViewportEnvironmentSelection.Kind == PixieEnvironmentSelectionKind.LegacyWorld && !PopupOpen())
                    {
                        comboMin = ImGui.GetItemRectMin(); comboMax = ImGui.GetItemRectMax();
                        contentRight = ImGui.GetWindowPos().X + ImGui.GetWindowSize().X;
                    }
                    ImGui.End(); ImGui.PopStyleVar();
                }
                else viewport.Draw(0.016);
                if (showWorldInspector)
                {
                    ImGui.SetNextWindowPos(new Vector2(920, 22)); ImGui.SetNextWindowSize(new Vector2(350, 820));
                    inspector.Draw(selection);
                }
                ImGui.Render(); clear(0x00004000); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
            }
            void Settle() { Frame(); Frame(); Frame(); }
            void MeasureCombo()
            {
                probe = true; Settle();
                var relativeMin = comboMin - probeOrigin; var relativeMax = comboMax - probeOrigin;
                probe = false; Settle();
                comboMin = environmentOrigin + relativeMin; comboMax = environmentOrigin + relativeMax;
            }
            (double X, double Y) CameraCenter()
            {
                int Size(string name) => (int)typeof(ViewportPanel).GetField(name, BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(viewport)!;
                return session.ViewToWorld(Size("Width") / 2.0, Size("Height") / 2.0);
            }
            void Click(Vector2 point)
            {
                Frame(() => io.AddMousePosEvent(point.X, point.Y));
                Frame(() => io.AddMouseButtonEvent(0, true));
                Frame(() => io.AddMouseButtonEvent(0, false)); Frame();
            }
            void Capture(string name)
            {
                probe = false;
                Frame(() => io.AddMousePosEvent(1250, 880)); Settle();
                Screenshot.Save(Path.Combine(work, name + ".png"), 1280, 900);
            }
            void OpenCombo()
            {
                Click(new Vector2(comboMax.X - 8, (comboMin.Y + comboMax.Y) / 2));
                Check(PopupOpen(), "combo-opens-at-visible-arrow-" + (int)viewportSize.X);
            }
            void ChooseRow(int row)
            {
                var y = comboMax.Y + popupPaddingY +
                    ImGui.GetTextLineHeight() / 2 + row * ImGui.GetTextLineHeightWithSpacing();
                Console.WriteLine($"Combo {comboMin}..{comboMax}; row {row} at {y}");
                Screenshot.Save(Path.Combine(work, $"popup-{checks.Count}.png"), 1280, 900);
                Click(new Vector2(comboMin.X + 25, y));
            }
            Settle();
            var authored = session.ProjectSnapshot();
            var revision = session.BindingRevision;
            Check(!session.Unsaved && session.ViewportEnvironment.Lighting == PixieEnvironmentLightingState.Absent,
                "already-v2-fixture-starts-clean-with-absent-lighting");
            Capture("world");
            MeasureCombo();
            var camera = CameraCenter();
            OpenCombo(); ChooseRow(1);
            Check(session.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL) &&
                session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld,
                "actual-scene-menu-row-selects-inherited-environment");
            Check(selection.Items.SequenceEqual(new[] { session.Find(entityId)!.Value }) && selection.Anchor == session.Find(entityId) &&
                CameraCenter() == camera && session.ProjectSnapshot() == authored && !session.Unsaved &&
                session.BindingRevision == revision && history.UndoCount == 0 && history.RedoCount == 0,
                "environment-click-preserves-selection-anchor-camera-scene-bindings-and-history");
            selection.Set([]); Frame(); selection.Set(session.Find(entityId)!.Value); Frame();
            Check(session.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL),
                "entity-selection-does-not-reselect-environment");
            Capture("scene-inherits-world");
            Settle(); OpenCombo(); ChooseRow(0);
            Check(session.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.LegacyWorld, 0UL),
                "actual-world-menu-row-restores-world-environment");
            viewportSize.X = 320; MeasureCombo();
            Check(comboMax.X <= contentRight && comboMax.X > comboMin.X + ImGui.GetFrameHeight(),
                "narrow-environment-combo-fits-with-visible-clickable-arrow");
            OpenCombo(); ChooseRow(1);
            Check(session.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL),
                "narrow-scene-menu-row-remains-clickable");
            Capture("narrow-scene");
            var removed = JsonNode.Parse(authored)!.AsObject(); removed["sceneScopes"] = new JsonArray();
            session.ProjectRestore(removed.ToJsonString());
            selection.Set(session.Find(entityId)!.Value); scopeRead.SetValue(viewport, 0L);
            var removedSnapshot = session.ProjectSnapshot();
            Settle();
            Check(session.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL) &&
                session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.RemovedSceneFallback &&
                session.ViewportEnvironment.Diagnostic.Contains("using World", StringComparison.Ordinal),
                "removed-scene-retains-intent-and-shows-world-fallback");
            Capture("narrow-removed-scene");
            viewportSize.X = 940; Capture("removed-scene");
            Check(session.ViewportEnvironment.Lighting == PixieEnvironmentLightingState.Absent &&
                session.ProjectSnapshot() == removedSnapshot && session.Unsaved &&
                disk.SequenceEqual(File.ReadAllBytes(scenePath)),
                "fallback-render-does-not-install-lighting-change-authored-data-or-save");
            session.ProjectRestore(authored); selection.Set(session.Find(entityId)!.Value);
            scopeRead.SetValue(viewport, 0L); Settle();
            Check(session.ViewportEnvironment.Resolution == PixieEnvironmentResolutionKind.SceneInheritedWorld,
                "restoring-scene-definition-recovers-selected-environment-intent");
            Capture("restored-scene");
            var beforeWorld = session.ProjectSnapshot(); var beforeWorldDirty = session.Unsaved;
            Click(new Vector2(65, 105));
            Check(selection.IsWorld && selection.Items.Count == 0 && selection.Anchor == null,
                "session-world-properties-button-selects-distinct-world-target");
            Check(session.ProjectSnapshot() == beforeWorld && session.Unsaved == beforeWorldDirty &&
                session.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL) &&
                !inspector.Set("PhysicsSettings2D", "gravityY", "1"),
                "world-navigation-preserves-environment-and-does-not-install-absent-settings");
            Capture("world-properties-absent");
            var physics = session.TypeIndex("PhysicsSettings2D"); var lighting = session.TypeIndex("LightingSettings2D");
            var physicsOwner = session.CreateEntity("World physics"); session.AddComponent(physicsOwner, physics);
            var lightingOwner = session.CreateEntity("World Environment"); session.AddComponent(lightingOwner, lighting);
            var physicsId = session.SceneId(physicsOwner)!.Value;
            var lightingId = session.SceneId(lightingOwner)!.Value;
            session.AddComponent(session.Find(entityId)!.Value, session.TypeIndex("Collider2D"));
            session.AddComponent(session.Find(entityId)!.Value, session.TypeIndex("RigidBody2D"));
            Settle();
            Check(inspector.Set("PhysicsSettings2D", "gravityX", "42"), "world-physics-field-is-editable-through-existing-authority");
            recorder.Close();
            Check(inspector.Set("LightingSettings2D", "ambientIntensity", "0.25"), "world-environment-field-is-editable-through-existing-authority");
            recorder.Close();
            Check(history.UndoCount == 2 && session.Get(physicsOwner, physics, session.FieldIndex(physics, "gravity")).X == 42 &&
                session.Get(lightingOwner, lighting, session.FieldIndex(lighting, "ambientIntensity")).X == 0.25 &&
                !inspector.Set("LightingSettings2D", "exposure", "2"),
                "separate-world-settings-owners-and-view-ownership-remain-distinct");
            authoring.Undo(); authoring.Undo(); Settle();
            physicsOwner = session.Find(physicsId)!.Value;
            lightingOwner = session.Find(lightingId)!.Value;
            Check(selection.IsWorld && session.Get(physicsOwner, physics, session.FieldIndex(physics, "gravity")).X == 0 &&
                session.Get(lightingOwner, lighting, session.FieldIndex(lighting, "ambientIntensity")).X == 1,
                "world-field-undo-retains-world-target-and-restores-both-authorities");
            authoring.Redo(); authoring.Redo(); Settle();
            physicsOwner = session.Find(physicsId)!.Value;
            lightingOwner = session.Find(lightingId)!.Value;
            Check(selection.IsWorld && history.UndoCount == 2 && history.RedoCount == 0,
                "world-field-redo-retains-world-target");
            var oldHandle = new ComponentHandle(session, physicsOwner, physics);
            selection.PreserveThrough(session, session.Play); session.Paused = true; session.Step();
            bool Refused(Action action) { try { action(); return false; } catch (EngineException) { return true; } }
            var playingPhysics = session.Entities().Single(entity => session.EntityName(entity) == "World physics");
            Check(Refused(() => oldHandle.Get(session.FieldIndex(physics, "gravity"))) &&
                Refused(() => oldHandle.Set(session.FieldIndex(physics, "gravity"), new PixieValue { X = 7 })),
                "replaced-world-component-handles-refuse-read-and-write");
            Check(Refused(() => session.Set(playingPhysics, physics, session.FieldIndex(physics, "unitsPerMeter"), new PixieValue { X = 64 })) &&
                session.Get(playingPhysics, physics, session.FieldIndex(physics, "unitsPerMeter")).X == 32,
                "post-step-world-unit-edit-is-refused-before-changing-storage");
            selection.PreserveThrough(session, session.Stop); Settle();
            Check(selection.IsWorld && selection.Items.Count == 0 && inspector.Set("PhysicsSettings2D", "gravityX", "42"),
                "world-target-and-bindings-recover-after-play-stop");
            recorder.Close();
            Capture("world-properties");
#if PIXIE_DEVELOPMENT
            inspector.SynchronizeChannel(selection.Items, session.BindingRevision, selection.IsWorld);
            Check(inspector.ChannelTree()["target"]?.GetValue<string>() == "world" &&
                inspector.ChannelTree()["rows"]!.AsArray().Count == 0,
                "world-target-is-explicitly-outside-entity-inspector-channel-surface");
            var channelRefused = false;
            try { inspector.ChannelSet("PhysicsSettings2D.gravityX", JsonValue.Create(8), inspector.ChannelGeneration); }
            catch (InvalidOperationException) { channelRefused = true; }
            Check(channelRefused, "world-target-channel-write-is-refused-explicitly");
            var invokedWorldGesture = false; var gestureRefused = false;
            try { inspector.ChannelGesture(() => { invokedWorldGesture = true; return true; }); }
            catch (InvalidOperationException) { gestureRefused = true; }
            Check(gestureRefused && !invokedWorldGesture, "unsupported-world-channel-gesture-is-refused-before-handler");
#endif
            var handoff = CodeReopen.Capture(session, selection.Items, null, selection.Anchor, selection.IsWorld);
            using (var rebuilt = EngineSession.OpenProject(320, 240, folder))
            {
                var restored = CodeReopen.Restore(rebuilt, handoff);
                Check(restored.World && restored.Selection.Length == 0 && restored.Anchor == null &&
                    rebuilt.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL),
                    "world-inspection-target-and-environment-survive-code-handoff");
            }
            selection.Set(session.Find(entityId)!.Value); Settle();
#if PIXIE_DEVELOPMENT
            selection.SetWorld(); Settle();
            inspector.SynchronizeChannel(selection.Items, session.BindingRevision, true);
            var worldGeneration = inspector.ChannelGeneration;
            var navigation = inspector.ChannelGesture(() => { selection.Set(session.Find(entityId)!.Value); return true; }, entityNavigation: true);
            inspector.SynchronizeChannel(selection.Items, session.BindingRevision, selection.IsWorld);
            Check(navigation["handler_returned"]!.GetValue<bool>() && !selection.IsWorld &&
                inspector.ChannelGeneration > worldGeneration && inspector.ChannelTree()["target"] == null,
                "entity-channel-navigation-recovers-from-world-target-and-invalidates-generation");
            Settle();
#endif
            Check(!selection.IsWorld && selection.Items.Count == 1 &&
                session.ViewportEnvironmentSelection == (PixieEnvironmentSelectionKind.Scene, 0xaaUL) &&
                disk.SequenceEqual(File.ReadAllBytes(scenePath)),
                "entity-selection-exits-world-inspection-without-changing-environment-or-file");
            Check(inspector.Set("Transform2D", "positionX", "0"), "entity-inspector-fields-recover-after-world-target");
            recorder.Close();
            // A clean v1 file may contain duplicated legacy settings. The World UI must
            // diagnose that state without selecting one editable owner or dirtying the file.
            var legacyFolder = Path.Combine(work, "legacy-duplicates"); EngineSession.CreateProject(legacyFolder);
            var legacyPath = Directory.GetFiles(Path.Combine(legacyFolder, "scenes"), "*.scene").Single();
            const string duplicated = "{\"format\":\"pixie-scene\",\"version\":1,\"seed\":1,\"entities\":[" +
                "{\"id\":\"10\",\"components\":{\"PhysicsSettings2D\":{},\"LightingSettings2D\":{}}}," +
                "{\"id\":\"11\",\"components\":{\"PhysicsSettings2D\":{},\"LightingSettings2D\":{}}}]}";
            File.WriteAllText(legacyPath, duplicated);
            using (var legacy = EngineSession.OpenProject(320, 240, legacyFolder))
            {
                var world = new Selection(); world.SetWorld(); var legacyHistory = new History();
                var legacyAuthoring = new Authoring(legacy, world, legacyHistory);
                var legacyRecorder = new EditRecorder(legacy, legacyHistory, () => legacyAuthoring.Recording);
                using var legacyInspector = new InspectorPanel(legacy, legacyAuthoring, legacyRecorder, new AssetChooser(legacyFolder));
                void LegacyFrame()
                {
                    GLFW.PollEvents(); ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); ImGui.NewFrame();
                    ImGui.SetNextWindowPos(new Vector2(920, 22)); ImGui.SetNextWindowSize(new Vector2(350, 820));
                    legacyInspector.Draw(world); ImGui.Render(); clear(0x00004000); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
                }
                LegacyFrame(); LegacyFrame();
                var before = legacy.ProjectSnapshot(); var first = legacy.Entities()[0];
                Check(!legacyInspector.Set("PhysicsSettings2D", "gravityY", "1") &&
                    !legacyInspector.Set("LightingSettings2D", "ambientIntensity", "0.1") &&
                    Refused(() => legacy.Set(first, physics, legacy.FieldIndex(physics, "gravity"), new PixieValue { X = 1 })) &&
                    legacy.ProjectSnapshot() == before && !legacy.Unsaved && legacyHistory.UndoCount == 0 &&
                    File.ReadAllText(legacyPath) == duplicated,
                    "clean-v1-duplicate-world-authorities-are-readable-and-refuse-edit-without-dirty-or-history");
                Screenshot.Save(Path.Combine(work, "world-properties-conflict.png"), 1280, 900);
            }
            File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new
            {
                passed = checks.Count, checks,
                limitations = new[] { "Owner visual acceptance pending; no Scene overrides or temporary-unload operation qualified.",
                    "An isolated widget measures geometry; all selection clicks and PNGs use the full production viewport." }
            }, new JsonSerializerOptions { WriteIndented = true }));
            return 0;
        }
        finally
        {
            ImGuiImplOpenGL3.Shutdown(); ImGuiImplGLFW.Shutdown();
            ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            ImGui.DestroyContext(context); GLFW.DestroyWindow(window); GLFW.Terminate();
        }
    }
}
