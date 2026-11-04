using System.Numerics;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Nodes;
using Hexa.NET.GLFW;
using Hexa.NET.ImGui;
using Hexa.NET.ImGui.Backends.GLFW;
using Hexa.NET.ImGui.Backends.OpenGL3;
using PixieEditor.Editing;
using PixieEditor.Native;
using PixieEditor.Ui;
using BackendWindow = Hexa.NET.ImGui.Backends.GLFW.GLFWwindowPtr;

// Geometry probes have NoInputs. Every tested interaction is subsequently delivered
// to complete production panels, and assertions read native authored storage.
internal static class PresentationVisualTests
{
    internal static unsafe int Run(string native, string work)
    {
        Directory.CreateDirectory(work); var checks = new List<string>();
        void Check(bool value, string name) { if (!value) throw new InvalidOperationException(name); checks.Add(name); Console.WriteLine("PASS " + name); }
        if (GLFW.Init() == 0) return 3;
        GLFW.WindowHint(GLFW.GLFW_VISIBLE, GLFW.GLFW_FALSE);
        GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MAJOR, 4); GLFW.WindowHint(GLFW.GLFW_CONTEXT_VERSION_MINOR, 5);
        GLFW.WindowHint(GLFW.GLFW_OPENGL_PROFILE, GLFW.GLFW_OPENGL_CORE_PROFILE);
        const int width = 1280, height = 1000;
        var window = GLFW.CreateWindow(width, height, "Presentation input qualification", default, default);
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
            const ulong scope = 0xaa, cameraId = 2;
            var folder = Path.Combine(Path.GetFullPath(work), "project"); EngineSession.CreateProject(folder);
            var path = Directory.GetFiles(Path.Combine(folder, "scenes"), "*.scene").Single();
            var fixture = JsonNode.Parse(File.ReadAllText(path))!;
            fixture["version"] = 2;
            fixture["sceneScopes"] = new JsonArray(new JsonObject { ["id"] = "aa", ["name"] = "Review room", ["camera"] = "2" });
            fixture["qualificationOpaque"] = new JsonObject { ["retained"] = true };
            fixture["entities"] = new JsonArray(
                new JsonObject { ["id"] = "1", ["name"] = "Review shape", ["components"] = new JsonObject
                {
                    ["Transform2D"] = new JsonObject { ["position"] = new JsonArray(100, 50) },
                    ["Shape"] = new JsonObject { ["kind"] = 3, ["size"] = new JsonArray(25, 25) },
                    ["LightingSettings2D"] = new JsonObject { ["v"] = 1, ["ambientIntensity"] = 1.0, ["futureLegacy"] = 17 }
                } },
                new JsonObject { ["id"] = "2", ["name"] = "Review camera", ["components"] = new JsonObject
                {
                    ["Transform2D"] = new JsonObject { ["position"] = new JsonArray(100, 50) },
                    ["Camera2D"] = new JsonObject { ["zoom"] = 2.0, ["enabled"] = true }
                } });
            File.WriteAllText(path, fixture.ToJsonString()); var saved = File.ReadAllBytes(path);
            using var session = EngineSession.OpenProject(640, 480, folder);
            var selection = new Selection(); selection.SetWorld(); var history = new History();
            var authoring = new Authoring(session, selection, history);
            var recorder = new EditRecorder(session, history, () => authoring.Recording);
            var files = new AssetChooser(folder);
            using var inspector = new InspectorPanel(session, authoring, recorder, files);
            var viewport = new ViewportPanel(session, selection, authoring, files).Started(); viewport.Mode = ViewportMode.Select;
            var toolbar = new Toolbar(session, viewport, authoring, selection); viewport.DrawControls = toolbar.DrawViewportControls;
            var coordinator = new SceneDocumentEditor(session, selection, history, recorder, () => false, () => inspector.Rebind(selection));
            var scenes = new SceneAuthoring(session, selection, history, coordinator);
            authoring.SceneEditor = coordinator; authoring.Scenes = scenes;
            var entities = new EntitiesPanel(session, selection, authoring);
            session.Edits.GestureActive = () => recorder.HasPendingEdits || inspector.HasActiveAuthoringGesture ||
                viewport.HasActiveAuthoringGesture || ImGui.IsAnyItemActive();
            var controls = new Dictionary<string, (Vector2 Min, Vector2 Max)>();
            var inspectorWidth = 410f; var viewportWidth = 575f;
            var inspectorPosition = new Vector2(width - inspectorWidth - 10, 20);
            var viewportPosition = new Vector2(275, 20);
            bool measureInspector = false, measureCamera = false;
            var helperFlags = BindingFlags.NonPublic | BindingFlags.Static;
            object? Helper(string name, params object?[] args) => typeof(InspectorPanel).GetMethod(name, helperFlags)!.Invoke(null, args);
            void Remember(string name) => controls[name] = (ImGui.GetItemRectMin(), ImGui.GetItemRectMax());
            void ProbeInspector()
            {
                ImGui.SetNextWindowPos(inspectorPosition); ImGui.SetNextWindowSize(new Vector2(inspectorWidth, 960));
                ImGui.PushStyleVar(ImGuiStyleVar.Alpha, 0.01f);
                ImGui.Begin("Presentation geometry only", ImGuiWindowFlags.NoInputs | ImGuiWindowFlags.NoSavedSettings);
                if (selection.IsWorld)
                {
                    ImGui.TextUnformatted("World properties");
                    ImGui.TextWrapped("World defaults. Inspecting this target does not select the viewport Environment.");
                    ImGui.Separator();
                    if (!session.PresentationSnapshot()["canonical"]!.GetValue<bool>())
                    {
                        typeof(InspectorPanel).GetMethod("DrawPresentation", BindingFlags.Instance | BindingFlags.NonPublic)!.Invoke(inspector, [0UL]);
                        Remember("migration");
                    }
                    else
                    {
                        var model = scenes.PresentationModel(0);
                        ImGui.CollapsingHeader("Environment", ImGuiTreeNodeFlags.DefaultOpen);
                        Helper("Choice", "Lighting capability", model, "lightingState", new[] { "absent", "disabled", "configured" });
                        ImGui.TextWrapped("Configuring lighting is explicit. Disabled and absent capabilities are not enabled by a Scene selection.");
                        Helper("Environment", model["environment"]!.AsObject());
                        var last = (Min: ImGui.GetItemRectMin(), Max: ImGui.GetItemRectMax());
                        var delta = new Vector2(0, 5 * ImGui.GetFrameHeightWithSpacing());
                        controls["ambient"] = (last.Min - delta, last.Max - delta);
                        ImGui.CollapsingHeader("Primary view", ImGuiTreeNodeFlags.DefaultOpen);
                        var view = model["view"]!.AsObject();
                        Helper("Number", "Exposure", view, "exposure"); Remember("exposure");
                        Helper("Choice", "Tone mapping", view, "toneMapping", new[] { "none", "aces" });
                        Helper("Flag", "Bloom", view, "bloom");
                        Helper("Number", "Bloom threshold", view, "bloomThreshold");
                        Helper("Number", "Bloom intensity", view, "bloomIntensity");
                        ImGui.CollapsingHeader("Renderer", ImGuiTreeNodeFlags.DefaultOpen);
                        var renderer = model["renderer"]!.AsObject();
                        Helper("Choice", "Quality", renderer, "quality", new[] { "low", "medium", "high" });
                        var budget = renderer["maxParticleLights"]!.GetValue<int>(); ImGui.InputInt("Particle light budget", ref budget); Remember("budget");
                    }
                }
                else
                {
                    var current = session.SceneScopes().Single(s => s.Id == scope);
                    ImGui.TextUnformatted("Scene properties"); ImGui.TextDisabled($"ID {scope:x16}");
                    ImGui.TextWrapped("Logical membership within World. Inspecting this Scene does not change the viewport Environment.");
                    var name = current.Name; ImGui.InputText("Name", ref name, 257); ImGui.Button("Remove empty Scene");
                    ImGui.TextWrapped("Reassign members before removal. Environment and camera selection remain independent of this Inspector target.");
                    var model = scenes.PresentationModel(scope); var environment = model["environment"]!.AsObject();
                    ImGui.CollapsingHeader("Environment override", ImGuiTreeNodeFlags.DefaultOpen);
                    Helper("Choice", "Mode", environment, "mode", new[] { "inherit", "override" }); Remember("override");
                    ImGui.BeginDisabled(environment["mode"]!.GetValue<string>() == "inherit");
                    Helper("Environment", environment["value"]!.AsObject()); ImGui.EndDisabled();
                    ImGui.TextWrapped(session.ViewportEnvironmentSelection.Kind == PixieEnvironmentSelectionKind.Scene
                        ? "This Scene is explicitly selected as the viewport Environment."
                        : "Inspecting or editing this Scene does not select its Environment for the viewport.");
                    ImGui.CollapsingHeader("Scene gravity", ImGuiTreeNodeFlags.DefaultOpen);
                    var gravity = model["gravity"]!.AsObject();
                    Helper("Choice", "Gravity mode", gravity, "mode", new[] { "inherit", "multiply", "override" }); Remember("gravity-mode");
                    var x = gravity["value"]![0]!.GetValue<double>(); var y = gravity["value"]![1]!.GetValue<double>();
                    ImGui.InputDouble("Gravity X", ref x); ImGui.InputDouble("Gravity Y", ref y);
                    Helper("Number", "Gravity multiplier", gravity, "multiplier"); Remember("gravity-multiplier");
                }
                ImGui.End(); ImGui.PopStyleVar();
            }
            void ProbeCamera()
            {
                ImGui.SetNextWindowPos(viewportPosition); ImGui.SetNextWindowSize(new Vector2(viewportWidth, 960));
                ImGui.PushStyleVar(ImGuiStyleVar.Alpha, 0.01f); ImGui.PushStyleVar(ImGuiStyleVar.WindowPadding, Vector2.Zero);
                ImGui.Begin("Camera geometry only", ImGuiWindowFlags.NoInputs | ImGuiWindowFlags.NoSavedSettings);
                toolbar.DrawViewportControls();
                var flags = BindingFlags.Instance | BindingFlags.NonPublic;
                typeof(ViewportPanel).GetMethod("DrawEnvironmentChoice", flags)!.Invoke(viewport, null);
                typeof(ViewportPanel).GetMethod("DrawCameraChoice", flags)!.Invoke(viewport, null);
                Remember("viewport-camera");
                ImGui.End(); ImGui.PopStyleVar(2);
            }
            void Frame(string? capture = null, Action? input = null)
            {
                GLFW.PollEvents(); ImGuiImplOpenGL3.NewFrame(); ImGuiImplGLFW.NewFrame(); input?.Invoke(); ImGui.NewFrame();
                scenes.ProcessPending();
                ImGui.SetNextWindowPos(new Vector2(0, 20)); ImGui.SetNextWindowSize(new Vector2(270, 960)); entities.Draw();
                ImGui.SetNextWindowPos(viewportPosition); ImGui.SetNextWindowSize(new Vector2(viewportWidth, 960)); viewport.Draw(0.016);
                inspectorPosition = new Vector2(width - inspectorWidth - 10, 20);
                ImGui.SetNextWindowPos(inspectorPosition); ImGui.SetNextWindowSize(new Vector2(inspectorWidth, 960)); inspector.Draw(selection);
                if (measureInspector) ProbeInspector(); if (measureCamera) ProbeCamera();
                if (!ImGui.IsAnyItemActive()) recorder.Close();
                ImGui.Render(); clear(0x00004000); ImGuiImplOpenGL3.RenderDrawData(ImGui.GetDrawData());
                if (capture != null) Screenshot.Save(Path.Combine(work, capture + ".png"), width, height);
            }
            void Settle() { Frame(); Frame(); Frame(); }
            void Measure(bool camera = false) { measureInspector = !camera; measureCamera = camera; Settle(); measureInspector = measureCamera = false; Settle(); }
            Vector2 Point(string name) { var rect = controls[name]; return new Vector2(rect.Min.X + Math.Min(55, (rect.Max.X - rect.Min.X) / 2), (rect.Min.Y + rect.Max.Y) / 2); }
            void Click(Vector2 point)
            {
                Frame(input: () => io.AddMousePosEvent(point.X, point.Y)); Frame(input: () => io.AddMouseButtonEvent(0, true));
                Frame(input: () => io.AddMouseButtonEvent(0, false)); Settle();
            }
            void Number(string name, string value)
            {
                Click(Point(name));
                Frame(input: () => io.AddKeyEvent(ImGuiKey.ModCtrl, true)); Frame(input: () => io.AddKeyEvent(ImGuiKey.A, true));
                Frame(input: () => { io.AddKeyEvent(ImGuiKey.A, false); io.AddKeyEvent(ImGuiKey.ModCtrl, false); });
                Frame(input: () => io.AddInputCharactersUTF8(value));
                Frame(input: () => io.AddKeyEvent(ImGuiKey.Enter, true)); Frame(input: () => io.AddKeyEvent(ImGuiKey.Enter, false));
                Click(new Vector2(inspectorPosition.X + 15, 48)); Settle();
            }
            void Choose(string name, int row)
            {
                var rect = controls[name]; Console.WriteLine($"Choosing {name} at {rect.Min}..{rect.Max}"); Frame("before-" + name); Click(Point(name));
                Check(ImGui.IsPopupOpen((byte*)null, ImGuiPopupFlags.AnyPopupId | ImGuiPopupFlags.AnyPopupLevel), name + "-popup-opens");
                Frame("popup-" + name);
                var padding = name == "viewport-camera" ? 0 : ImGui.GetStyle().WindowPadding.Y;
                Click(new Vector2(rect.Min.X + 30, rect.Max.Y + padding + ImGui.GetTextLineHeight() / 2 + row * ImGui.GetTextLineHeightWithSpacing()));
            }
            JsonNode Authored() => JsonNode.Parse(session.ProjectSnapshot())!;
            Settle(); Frame("world-before-migration"); Measure();
            var originalHistory = history.UndoCount;
            Click(Point("migration"));
            Check(session.PresentationSnapshot()["canonical"]!.GetValue<bool>() && history.UndoCount == originalHistory + 1,
                "actual-migration-button-uses-one-candidate-history-entry");
            Check(Authored()["presentation"]!["legacySource"]!["futureLegacy"]!.GetValue<int>() == 17,
                "actual-migration-retains-unknown-legacy-payload");
            Frame("world-canonical"); Measure();
            Number("ambient", "0.75");
            Check(Authored()["presentation"]!["environment"]!["ambientIntensity"]!.GetValue<double>() == 0.75, "actual-world-environment-number-commits");
            Measure(); Number("exposure", "1.5");
            Check(Authored()["presentation"]!["view"]!["exposure"]!.GetValue<double>() == 1.5, "actual-primary-view-exposure-commits");
            var finiteBefore = session.ProjectSnapshot(); var finiteHistory = history.Revision;
            Measure(); Number("exposure", "1e999");
            Check(session.ProjectSnapshot() == finiteBefore && history.Revision == finiteHistory,
                "actual-nonfinite-number-entry-keeps-world-and-history");
            Measure(); Number("budget", "37");
            Check(Authored()["presentation"]!["renderer"]!["maxParticleLights"]!.GetValue<int>() == 37, "actual-renderer-budget-commits");
            Frame("world-settings-edited");
            var environmentChoice = session.ViewportEnvironmentSelection; scenes.Select(scope); Settle(); Measure();
            Choose("override", 1);
            Check(Authored()["sceneScopes"]![0]!["environment"]!["mode"]!.GetValue<string>() == "override" &&
                session.ViewportEnvironmentSelection == environmentChoice, "actual-scene-environment-mode-commits-without-selecting-viewport");
            Measure(); Choose("gravity-mode", 1); Measure(); Number("gravity-multiplier", "0.5");
            Check(Authored()["sceneScopes"]![0]!["gravity"]!["mode"]!.GetValue<string>() == "multiply" &&
                Authored()["sceneScopes"]![0]!["gravity"]!["multiplier"]!.GetValue<double>() == 0.5,
                "actual-scene-gravity-mode-and-multiplier-commit");
            Frame("scene-environment-gravity");
            session.SelectViewportEnvironment(PixieEnvironmentSelectionKind.Scene, scope); Settle(); Measure(camera: true);
            var independentEnvironment = session.ViewportEnvironmentSelection; Choose("viewport-camera", 3);
            Check(session.ViewportCameraSelection.Entity == cameraId && session.ViewportCameraSelection.Kind == 2 &&
                session.ViewportEnvironmentSelection == independentEnvironment && selection.Scope == scope,
                "actual-viewport-camera-selector-preserves-environment-and-inspector-target");
            Frame("camera-wide");
            viewportWidth = 320; inspectorWidth = 320; Settle(); Measure(camera: true);
            Check(controls["viewport-camera"].Max.X <= viewportPosition.X + viewportWidth + 0.5f,
                "narrow-camera-combo-arrow-stays-inside-viewport");
            Frame("camera-and-scene-narrow"); Choose("viewport-camera", 0);
            Check(session.ViewportCameraSelection.Kind == 0 && session.ViewportEnvironmentSelection == independentEnvironment,
                "actual-narrow-camera-selector-returns-to-free-without-environment-change");
            Frame("camera-free-narrow");
            Check(File.ReadAllBytes(path).SequenceEqual(saved) && Authored()["qualificationOpaque"]!["retained"]!.GetValue<bool>(),
                "actual-ui-authoring-preserves-saved-file-and-unknown-root-data");
            File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks,
                controls = "Migration, World ambient, view exposure, renderer budget, Scene Environment mode, gravity mode/multiplier and viewport camera selection.",
                limitations = "Hidden real GL/ImGui; geometry-only NoInputs probes use production widget helpers and matching prefix layout. Every input runs complete production panels. Fixed fixtures, no owner visual acceptance, no body-gravity activation." }, new JsonSerializerOptions { WriteIndented = true }));
            return 0;
        }
        finally
        {
            ImGuiImplOpenGL3.Shutdown(); ImGuiImplGLFW.Shutdown(); ImGuiImplOpenGL3.SetCurrentContext(default); ImGuiImplGLFW.SetCurrentContext(default);
            ImGui.DestroyContext(context); GLFW.DestroyWindow(window); GLFW.Terminate();
        }
    }
}
