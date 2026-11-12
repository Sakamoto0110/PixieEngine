#include "pixie/runtime/ProjectSession.hpp"
#include "SceneSave.hpp"

#include "pixie/runtime/Culling.hpp"
#include "pixie/runtime/Picking.hpp"

#include "pixie/render/RenderTarget.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/core/Log.hpp"
#include "pixie/runtime/LightingDrawing.hpp"
#include "pixie/runtime/ParticleStep.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneProject.hpp"
#include "pixie/runtime/PathDrawing.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/ShapeDrawing.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace pixie {
namespace { std::atomic<std::uint64_t> nextSessionIdentity{1}; }

ProjectSession::ProjectSession(std::unique_ptr<Project> project, SessionSettings settings, ProjectFactory candidateFactory)
    : identity_(nextSessionIdentity.fetch_add(1)), candidateFactory_(std::move(candidateFactory)),
      project_(std::move(project)), settings_(settings), stepper_(settings.updateRate) {
    DescribeRuntimeComponents(types_);
    project_->Describe(types_);
}

ProjectSession::~ProjectSession() {
    // The World may hold no GL objects, but the renderers do: Stop must have run.
    world_.reset();
}

bool ProjectSession::OpenProject(const std::filesystem::path& folder) {
    OpenedProject opened = OpenProjectFolder(folder);
    if (!opened.ok) {
        error_ = opened.error;
        return false;
    }
    const std::filesystem::path scenePath = opened.StartScenePath();
    std::ifstream in(scenePath, std::ios::binary);
    if (!in) {
        error_ = std::format("the start scene, {}, could not be opened", opened.file.startScene);
        return false;
    }
    std::ostringstream text;
    text << in.rdbuf();
    // Read once into a World of its own, so a scene that cannot be read is refused here.
    World check;
    if (const SceneReport report = ReadScene(check, types_, text.str()); !report.ok) {
        error_ = std::format("{}: {}", opened.file.startScene, report.error);
        return false;
    }
    std::error_code identityError;
    const bool sameProject = opened_ && std::filesystem::equivalent(opened_->folder, opened.folder, identityError)
        && !identityError;
    if (started_ && !Build(true, text.str())) return false;
    images_.SetFolder(opened.folder);
    outlines_.SetFolder(opened.folder);
    text_.Destroy();
    fonts_.SetFolder(opened.folder);
    opened_ = std::move(opened);
    sceneText_ = text.str();
    playedFrom_.clear();
    mode_ = SessionMode::Edit;
    unsaved_ = false;
    if (!sameProject) { SetViewportEnvironment({}); SetViewportCamera({}); }
    error_.clear();
    return true;
}

std::string_view ProjectSession::ProjectName() const {
    return opened_ ? std::string_view(opened_->file.name) : project_->Name();
}

bool ProjectSession::Start(FramebufferSize size) {
    error_.clear();
    if (!shapes_.Create() || !particles_.Create() || !sprites_.Create() || !paths_.Create() || !lighting_.Create()) {
        return false;
    }
    const Camera2D previousCamera = camera_;
    camera_.SetViewport(size);
    if (!Build(true, sceneText_)) {
        camera_ = previousCamera;
        lighting_.Destroy();
        paths_.Destroy();
        sprites_.Destroy();
        particles_.Destroy();
        shapes_.Destroy();
        return false;
    }
    started_ = true;
    return true;
}

void ProjectSession::Stop() {
    ++worldRevision_;
    world_.reset();
    target_.Destroy();
    images_.Destroy();
    text_.Destroy();
    lighting_.Destroy();
    paths_.Destroy();
    sprites_.Destroy();
    particles_.Destroy();
    shapes_.Destroy();
    started_ = false;
}

bool ProjectSession::Build(bool resetCamera, std::string_view sceneText, SceneBuildTimings* timings, bool validateMovement) {
    const auto preparing = std::chrono::steady_clock::now();
    // Declaration order also governs failed-candidate teardown: World, types, Project.
    std::unique_ptr<Project> candidateProject;
    ComponentTypes candidateTypes;
    std::unique_ptr<World> candidate;
    Camera2D candidateCamera = camera_;
    std::vector<std::string> warnings;
    if (resetCamera) {
        candidateCamera.SetCenter(pxPoint(0.0, 0.0));
        candidateCamera.SetZoom(1.0);
    }
    try {
        if (candidateFactory_) {
            if (!sceneText.empty()) {
                if (const auto problem = SceneImportAdmissionError(types_, sceneText); !problem.empty()) {
                    error_ = problem;
                    return false;
                }
            }
            candidateProject = candidateFactory_();
            if (candidateProject.get() == project_.get()) {
                candidateProject.release(); // A broken factory must not destroy the live Project.
                throw std::runtime_error("candidate factory returned the published Project");
            }
            if (!candidateProject) throw std::runtime_error("candidate factory returned no Project");
            DescribeRuntimeComponents(candidateTypes);
            candidateProject->Describe(candidateTypes);
            if (!types_.SameSchema(candidateTypes)) throw std::runtime_error("candidate component schema changed");
        }
        candidate = std::make_unique<World>(settings_.seed);
        if (!sceneText.empty()) {
            const SceneReport report = ReadScene(*candidate, candidateProject ? candidateTypes : types_, sceneText);
            if (!report.ok) {
                error_ = std::format("{}: the scene could not be read: {}", ProjectName(), report.error);
                return false;
            }
            warnings = report.warnings;
        }
        const int authoredFormat = SceneFormatVersion(*candidate);
        (candidateProject ? *candidateProject : *project_).Build(*candidate, candidateCamera);
        // After the project's Build, so a project that adds the step itself keeps its place.
        AddParticleStep(*candidate);
        InstallSceneMovement2D(*candidate);
        SyncSceneMovement2D(*candidate);
        if (candidateProject) {
            if (SceneFormatVersion(*candidate) != authoredFormat) throw std::runtime_error("candidate Build changed the authored scene format");
            if (!types_.SameSchema(candidateTypes)) throw std::runtime_error("candidate component schema changed during Build");
            if (const auto problem = ValidateV2SceneSnapshot(*candidate, candidateTypes); !problem.empty()) {
                error_ = problem;
                return false;
            }
        }
        if (validateMovement) {
            const auto problems = SceneMovementErrors2D(*candidate);
            if (!problems.empty()) {
                error_ = std::format("candidate movement is invalid: {}", problems.front());
                return false;
            }
        }
    } catch (const std::exception& failure) {
        error_ = std::format("{}: World build failed: {}", ProjectName(), failure.what());
        return false;
    } catch (...) {
        error_ = std::format("{}: World build failed: unknown exception", ProjectName());
        return false;
    }
    const auto lights = candidate->Registry().view<const Light2D>();
    if (lights.begin() != lights.end() && ReadPresentation2D(*candidate).lighting == LightingState2D::Absent) {
        LogWarning(std::format("{}: the scene has lights but no LightingSettings2D, so it is drawn unlit", ProjectName()));
    }
    // Formatting/logging can fail: finish it before the irreversible storage publication.
    for (const std::string& warning : warnings) {
        LogWarning(std::format("{}: {}", ProjectName(), warning));
    }
    const auto publishing = std::chrono::steady_clock::now();
    // No handle into the old World remains valid after this point. A failed read never reaches it.
    // Publish only after the read, project Build and requested movement admission succeed.
    input_.EndUpdate();
    world_.swap(candidate);
    authoringPreviewDirty_ = false;
    if (candidateProject) {
        types_.Swap(candidateTypes);
        project_.swap(candidateProject);
    }
    camera_ = candidateCamera;
    ++worldRevision_;
    candidate.reset();
    // Retired callbacks/descriptors must be gone before their matching Project.
    { ComponentTypes retiredTypes; candidateTypes.Swap(retiredTypes); }
    candidateProject.reset();
    error_.clear();
    if (timings != nullptr) {
        timings->preparationMilliseconds = std::chrono::duration<double, std::milli>(publishing - preparing).count();
        timings->publicationMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - publishing).count();
    }
    return true;
}

void ProjectSession::Reset() {
    error_.clear();
    std::string nextSceneText = sceneText_;
    if (world_) {
        if (!candidateFactory_) project_->Capture(*world_);
        if (opened_) {
            try {
                nextSceneText = WriteScene(*world_, types_);
            } catch (const std::runtime_error& error) {
                error_ = error.what();
                return;
            }
        }
    }
    if (!Build(!opened_, nextSceneText)) return;
    sceneText_.swap(nextSceneText);
    stepper_.Drop();
}

bool ProjectSession::RestoreAuthoredScene(std::string_view snapshot) {
    return RestoreAuthoredScene(snapshot, nullptr);
}

bool ProjectSession::RestoreAuthoredScene(std::string_view snapshot, SceneBuildTimings* timings, bool validateMovement,
                                         bool preserveRandom) {
    if (!opened_ || mode_ != SessionMode::Edit || !world_) {
        error_ = "an open project in Edit with an active World is required to restore an authored scene";
        return false;
    }
    std::string nextSceneText(snapshot);
    const Random previousRandom = world_->GetRandom();
    if (!Build(false, nextSceneText, timings, validateMovement)) return false;
    if (preserveRandom && world_->GetRandom().Seed() == previousRandom.Seed()) world_->GetRandom() = previousRandom;
    sceneText_.swap(nextSceneText);
    unsaved_ = true;
    stepper_.Drop();
    return true;
}

bool ProjectSession::Play() {
    if (!opened_) {
        error_ = "no project is open, so there is no scene to play";
        return false;
    }
    if (mode_ == SessionMode::Play) {
        return true;
    }
    std::string nextPlayedFrom;
    try {
        RefreshAuthoringPreview();
        nextPlayedFrom = WriteScene(*world_, types_);
    } catch (const std::runtime_error& error) {
        error_ = error.what();
        return false;
    }
    std::string nextSceneText = nextPlayedFrom;
    if (!Build(false, nextSceneText)) return false;
    playedFrom_.swap(nextPlayedFrom);
    sceneText_.swap(nextSceneText);
    mode_ = SessionMode::Play;
    paused_ = false;
    stepper_.Drop();
    return true;
}

bool ProjectSession::StopPlaying() {
    if (!opened_) {
        error_ = "no project is open, so there is no scene to go back to";
        return false;
    }
    if (mode_ == SessionMode::Edit) {
        return true;
    }
    std::string nextSceneText = playedFrom_;
    if (!Build(false, nextSceneText)) return false;
    sceneText_.swap(nextSceneText);
    playedFrom_.clear();
    mode_ = SessionMode::Edit;
    stepper_.Drop();
    return true;
}

bool ProjectSession::Save() {
    if (!opened_) {
        error_ = "no project is open, so there is nowhere to save";
        return false;
    }
    std::vector<std::string> warnings;
    std::string text;
    try {
        RefreshAuthoringPreview();
        text = mode_ == SessionMode::Play ? playedFrom_ : WriteScene(*world_, types_, &warnings);
    } catch (const std::runtime_error& error) {
        error_ = error.what();
        return false;
    }
    for (const std::string& warning : warnings) {
        LogWarning(std::format("{}: {}", ProjectName(), warning));
    }
    if (const SceneReport saved = detail::SaveSceneText(text, opened_->StartScenePath()); !saved.ok) {
        error_ = saved.error;
        return false;
    }
    unsaved_ = false;
    return true;
}

void ProjectSession::MarkChanged() {
    if (opened_ && mode_ == SessionMode::Edit) {
        unsaved_ = true;
        authoringPreviewDirty_ = true;
    }
}

void ProjectSession::RefreshAuthoringPreview() {
    if (!authoringPreviewDirty_ || mode_ != SessionMode::Edit) return;
    project_->RefreshAuthoringPreview(*world_);
    authoringPreviewDirty_ = false;
}

bool ProjectSession::ConvertStartSceneToV2() {
    if (!opened_ || mode_ != SessionMode::Edit || !world_) {
        error_ = "conversion requires an open project in Edit";
        return false;
    }
    std::string converted;
    const auto report = ConvertSceneFileV1ToV2(*world_, types_, opened_->StartScenePath(), &converted);
    if (!report.ok) { error_ = report.error; return false; }
    // The file is committed: only nonallocating state changes remain.
    sceneText_.swap(converted);
    unsaved_ = false;
    ++authoredRevision_;
    error_.clear();
    return true;
}

void ProjectSession::BeginAuthoredMutation() {
    if (opened_ && mode_ == SessionMode::Edit) ++authoredRevision_;
}

void ProjectSession::Resize(FramebufferSize size) {
    camera_.SetViewport(size);
}

std::optional<ViewportEnvironment2D> ProjectSession::ViewportEnvironment() const {
    if (!world_) return std::nullopt;
    return ResolveViewportEnvironment2D(*world_, environmentSelection_);
}

void ProjectSession::Update(double delta, const Input& input) {
    if (mode_ == SessionMode::Edit) {
        return;
    }
    project_->BeforeStep(*world_, camera_, input);
    const auto start = std::chrono::steady_clock::now();
    world_->Step(delta, input);
    lastStepMilliseconds_ =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

void ProjectSession::Advance(double frameDelta) {
    if (mode_ == SessionMode::Edit) {
        PanAndZoom(camera_, input_);
        input_.EndUpdate();
        return;
    }
    if (paused_) {
        return;
    }
    stepper_.Advance(frameDelta * timeScale_, [this](const FrameTime& time) {
        Update(time.delta, input_);
        input_.EndUpdate();
    });
}

void ProjectSession::StepOnce() {
    if (mode_ == SessionMode::Edit) {
        return;
    }
    Update(stepper_.StepSeconds(), input_);
    input_.EndUpdate();
}

void ProjectSession::SetTimeScale(double scale) {
    timeScale_ = std::clamp(scale, kMinTimeScale, kMaxTimeScale);
}

void ProjectSession::SetPaused(bool paused) {
    paused_ = paused;
    stepper_.Drop();
}

double ProjectSession::Alpha() const {
    return paused_ || mode_ == SessionMode::Edit ? 1.0 : stepper_.RenderTime(0.0, 0).alpha;
}

namespace {

// How far beyond the screen's edges, in view pixels, a session still draws.
constexpr double kCullPadding = 32.0;

} // namespace

void ProjectSession::Render(double alpha) {
    RefreshAuthoringPreview();
    const Camera2D camera = ResolvedCamera().camera;
    const pxColorRgba background = project_->Background();
    const ViewportEnvironment2D environment = ResolveViewportEnvironment2D(*world_, environmentSelection_);
    const bool fallback = environment.kind == EnvironmentResolutionKind2D::RemovedSceneFallback;
    if (fallback && !environmentFallbackReported_) LogWarning(environment.diagnostic);
    environmentFallbackReported_ = fallback;
    // Absent or disabled lighting draws exactly as an unlit scene. A disabled
    // component retains its authored values for later re-enabling.
    LightingSettings2D resolvedLighting = ResolveLightingSettings2D(*world_,
        environmentSelection_.kind == EnvironmentSelectionKind2D::Scene ? environmentSelection_.scene.value : 0);
    if (viewOverride_) {
        resolvedLighting.exposure = viewOverride_->exposure;
        resolvedLighting.toneMapping = static_cast<LightToneMapping2D>(viewOverride_->toneMapping);
        resolvedLighting.bloom = viewOverride_->bloom; resolvedLighting.bloomThreshold = viewOverride_->bloomThreshold;
        resolvedLighting.bloomIntensity = viewOverride_->bloomIntensity;
    }
    const LightingSettings2D* lighting = environment.lighting.state == LightingState2D::Configured ? &resolvedLighting : nullptr;
    if (lighting != nullptr) {
        lighting_.BeginWorld(camera.Viewport(), background);
    } else {
        pixie::ClearColor(background.r / 255.0f, background.g / 255.0f, background.b / 255.0f, 1.0f);
    }
    if (const auto now = std::chrono::steady_clock::now(); now >= nextImageCheck_) {
        images_.CheckForChanges();
        nextImageCheck_ = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                    std::chrono::duration<double>(SpriteImages::kCheckInterval));
    }
    const TextSources texts{fonts_, text_, camera.Zoom()};
    // Only what may show on screen is submitted.
    const pxRect view = PaddedView(camera, kCullPadding);
    SubmitSprites(*world_, sprites_, images_, alpha, &texts, &view);
    sprites_.Flush(camera.MakeView(), 0);
    SubmitPaths(*world_, paths_, alpha, &view);
    paths_.Flush(camera.MakeView());
    sprites_.Flush(camera.MakeView());
    if (lighting != nullptr) {
        // Emissive pools glow in the emission pass instead of being lit.
        DrawParticlePools(*world_, particles_, camera, alpha, false);
    } else {
        DrawParticles(*world_, particles_, camera, alpha);
    }
    SubmitShapes(*world_, shapes_, alpha, &view);
    std::size_t sceneLights = 0;
    if (lighting != nullptr) {
        Telemetry& telemetry = world_->GetTelemetry();
        if (lightingMetrics_.world != worldRevision_) {
            lightingMetrics_ = {telemetry.RegisterTiming("lighting.cpu"), telemetry.RegisterTiming("lighting.gpu.lights"),
                                telemetry.RegisterTiming("lighting.gpu.shadows"), telemetry.RegisterTiming("lighting.gpu.combine"),
                                telemetry.RegisterGauge("lighting.lights"), telemetry.RegisterGauge("lighting.shadowed_lights"),
                                telemetry.RegisterGauge("lighting.shadowed_lights_dropped"),
                                telemetry.RegisterGauge("lighting.occluder_edges"), telemetry.RegisterGauge("lighting.particle_lights"),
                                telemetry.RegisterGauge("lighting.particle_lights_dropped"), worldRevision_,
                                lightingMetrics_.gpuSample};
        }
        int shadowedDropped = 0;
        int particleDropped = 0;
        {
            const TelemetryScope cpu(telemetry, lightingMetrics_.cpu);
            // World shapes are part of the lit image; gizmos and overlays are drawn over it, unlit.
            shapes_.Flush(camera.MakeView());
            if (HasLightReceivers(*world_)) {
                lighting_.BeginReceivers();
                DrawLightingPass(alpha, texts, view, LightingPass::Receivers, camera);
            }
            if (HasEmission(*world_)) {
                lighting_.BeginEmission();
                DrawLightingPass(alpha, texts, view, LightingPass::Emission, camera);
                particles_.SetEmissionPass(true);
                DrawParticlePools(*world_, particles_, camera, alpha, true);
                particles_.SetEmissionPass(false);
            }
            if (lighting->normalMaps && HasNormalMaps(*world_)) {
                lighting_.BeginNormals();
                sprites_.SetLightingPass(LightingPass::Normals);
                SubmitSpriteNormals(*world_, sprites_, images_, alpha, &texts, &view);
                sprites_.Flush(camera.MakeView());
                sprites_.SetLightingPass(LightingPass::Color);
            }
            GatherLights(*world_, alpha, &view, lights_);
            shadowedDropped = CapShadowedLights(lights_, ShadowedLightCap(lighting->quality), view);
            sceneLights = lights_.size();
            particleDropped = GatherParticleLights(*world_, alpha, &view, lighting->maxParticleLights, lights_);
            GatherOccluders(*world_, alpha, occluders_, &outlines_);
            LightingParameters parameters = MakeLightingParameters(*lighting);
            parameters.view = LightingDebugView(LightingDebug());
            lighting_.Finish(camera.MakeView(), parameters, lights_, occluders_);
        }
        telemetry.SetGauge(lightingMetrics_.lights, static_cast<double>(lighting_.LastLightCount()));
        telemetry.SetGauge(lightingMetrics_.shadowed, static_cast<double>(lighting_.LastShadowedCount()));
        telemetry.SetGauge(lightingMetrics_.shadowedDropped, shadowedDropped);
        telemetry.SetGauge(lightingMetrics_.edges, static_cast<double>(occluders_.size()));
        telemetry.SetGauge(lightingMetrics_.particleLights, static_cast<double>(lights_.size() - sceneLights));
        telemetry.SetGauge(lightingMetrics_.particleDropped, particleDropped);
        if (const LightingGpuTimes& gpu = lighting_.GpuTimes(); gpu.sample != lightingMetrics_.gpuSample) {
            lightingMetrics_.gpuSample = gpu.sample;
            telemetry.RecordTiming(lightingMetrics_.gpuLights, gpu.lights);
            telemetry.RecordTiming(lightingMetrics_.gpuShadows, gpu.shadows);
            telemetry.RecordTiming(lightingMetrics_.gpuCombine, gpu.combine);
        }
    }
    if (mode_ == SessionMode::Edit) {
        SyncSceneMovement2D(*world_);
        if (!physicsDebugExplicit_) DrawMovementGizmos2D(*world_, shapes_, camera);
    }
    if (physicsDebugExplicit_) DrawPhysicsDebug2D(*world_, shapes_, camera, physicsDebug_);
    if (lighting != nullptr && LightingDebug() != 0) {
        DrawLightingDebug2D(std::span<const LightInstance>(lights_.data(), sceneLights), occluders_, shapes_, camera,
                            LightingDebug());
    }
    project_->DrawOverlay(*world_, shapes_, camera);
    shapes_.Flush(camera.MakeView());

    ScreenCanvas screen(camera, shapes_, paths_, sprites_, text_, images_, fonts_);
    project_->DrawScreen(*world_, screen);
    const Camera2D pixels = ScreenCanvas::PixelCamera(camera.Viewport());
    shapes_.Flush(pixels.MakeView());
    paths_.Flush(pixels.MakeView());
    sprites_.Flush(pixels.MakeView());
}

void ProjectSession::DrawLightingPass(double alpha, const TextSources& texts, const pxRect& view, LightingPass pass, const Camera2D& camera) {
    sprites_.SetLightingPass(pass);
    if (pass == LightingPass::Emission) {
        SubmitSpriteEmission(*world_, sprites_, images_, alpha, &texts, &view);
    } else {
        SubmitSpriteReceivers(*world_, sprites_, images_, alpha, &texts, &view);
    }
    sprites_.Flush(camera.MakeView());
    sprites_.SetLightingPass(LightingPass::Color);
    shapes_.SetLightingPass(pass);
    if (pass == LightingPass::Emission) {
        SubmitShapeEmission(*world_, shapes_, alpha, &view);
    } else {
        SubmitShapeReceivers(*world_, shapes_, alpha, &view);
    }
    shapes_.Flush(camera.MakeView());
    shapes_.SetLightingPass(LightingPass::Color);
}

ViewportCamera2D ProjectSession::ResolveCameraForViewport(CameraSelection2D selection, FramebufferSize size) const {
    auto fallback = camera_; fallback.SetViewport(size);
    return ResolveViewportCamera2D(*world_, fallback, selection);
}
Entity ProjectSession::PickViewport(CameraSelection2D selection, FramebufferSize size, pxPoint point, double tolerance) {
    const auto camera = ResolveCameraForViewport(selection, size).camera;
    const auto entity = PickAt(*world_, camera.ScreenToWorld(point), tolerance / camera.Zoom(), &images_, &fonts_);
    return entity == entt::null ? Entity{} : world_->Wrap(entity);
}

unsigned ProjectSession::RenderViewportToTarget(RenderTarget& target, FramebufferSize size,
    CameraSelection2D camera, EnvironmentSelection2D environment, double alpha, std::optional<ViewLighting2D> view) {
    const auto previousCamera = cameraSelection_;
    const auto previousEnvironment = environmentSelection_;
    const auto previousSize = camera_.Viewport();
    const auto previousView = viewOverride_;
    const auto previousFallbackReported = environmentFallbackReported_;
    GLint previousViewport[4]{};
    GLint previousDraw = 0, previousRead = 0;
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDraw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousRead);
    if (view) { auto proposed = ReadPresentation2D(*world_); proposed.view = *view; if (auto problem = ValidatePresentation2D(proposed); !problem.empty()) throw std::invalid_argument(problem); }
    struct Restore {
        ProjectSession& owner; CameraSelection2D camera; EnvironmentSelection2D environment; FramebufferSize size;
        std::optional<ViewLighting2D>& targetView; std::optional<ViewLighting2D> previousView; bool& report; bool previousReport;
        GLint viewport[4]; GLint draw, read;
        ~Restore() {
            owner.SetViewportCamera(camera); owner.SetViewportEnvironment(environment); owner.GetCamera().SetViewport(size);
            targetView = previousView; report = previousReport;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
            glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        }
    } restore{*this, previousCamera, previousEnvironment, previousSize, viewOverride_, previousView,
        environmentFallbackReported_, previousFallbackReported,
        {previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]}, previousDraw, previousRead};
    viewOverride_ = view;
    SetViewportCamera(camera); SetViewportEnvironment(environment); camera_.SetViewport(size);
    if (size.width <= 0 || size.height <= 0 || !target.Resize(size)) return 0;
    target.Bind();
    try { Render(alpha); } catch (...) { RenderTarget::BindDefault(); throw; }
    RenderTarget::BindDefault(); return target.ColorTexture();
}

unsigned ProjectSession::RenderToTarget() {
    return RenderToTarget(Alpha());
}

unsigned ProjectSession::RenderToTarget(double alpha) {
    const FramebufferSize size = camera_.Viewport();
    if (size.width <= 0 || size.height <= 0 || !target_.Resize(size)) {
        return 0;
    }
    target_.Bind();
    Render(alpha);
    RenderTarget::BindDefault();
    return target_.ColorTexture();
}

} // namespace pixie
