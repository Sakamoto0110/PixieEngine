#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/core/FixedStepper.hpp"
#include "pixie/render/2d/LightRenderer.hpp"
#include "pixie/platform/Input.hpp"
#include "pixie/render/2d/ParticleRenderer.hpp"
#include "pixie/render/2d/PathRenderer.hpp"
#include "pixie/render/RenderTarget.hpp"
#include "pixie/render/2d/ShapeRenderer.hpp"
#include "pixie/render/2d/SpriteRenderer.hpp"
#include "pixie/render/2d/TextRenderer.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Fonts.hpp"
#include "pixie/runtime/LightingDrawing.hpp"
#include "pixie/runtime/LightingEnvironment2D.hpp"
#include "pixie/runtime/Project.hpp"
#include "pixie/runtime/ProjectFile.hpp"
#include "pixie/runtime/SpriteDrawing.hpp"
#include "pixie/runtime/SpriteImages.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/ViewportCamera2D.hpp"
#include "pixie/runtime/PhysicsDebug2D.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pixie {

struct SessionSettings {
    // The World's seed; a scene's own seed replaces it.
    std::uint64_t seed = 1;
    double updateRate = 60.0;
};

// What a session does with time. In Play the World steps; in Edit it stays as built while the scene
// is edited. A session without a project always plays.
enum class SessionMode { Play, Edit };

struct SceneBuildTimings {
    double preparationMilliseconds = 0;
    double publicationMilliseconds = 0; // Includes destruction of the replaced World.
};

// One running Project: its World, camera, renderers, input, and time. It is the engine's loop for a
// host that drives it, such as an editor: the host feeds input events, calls Advance with the wall
// time of each frame, and draws with Render or RenderToTarget. ProjectApp drives the same session
// from pixie::Run, through Update and Render.
//
// With a project folder open, the World is built from the project's start scene: the scene's
// entities first, then the Project's Build for what is not saved. The session starts in Edit; Play
// keeps the scene's entities in memory and builds the World again from them, and StopPlaying
// builds it again from that copy, so nothing done in play stays.
//
// Start and every later call need the GL context current; Stop releases the GL objects.
class ProjectSession {
public:
    // Opted-in development modules supply a pinned factory. Each Build stages a matching
    // fresh Project/types/World. Callbacks must not mutate external or published state.
    using ProjectFactory = std::function<std::unique_ptr<Project>()>;
    PIXIE_RUNTIME_API ProjectSession(std::unique_ptr<Project> project, SessionSettings settings,
                                    ProjectFactory candidateFactory = {});
    PIXIE_RUNTIME_API ~ProjectSession();
    ProjectSession(const ProjectSession&) = delete;
    ProjectSession& operator=(const ProjectSession&) = delete;

    // Opens the project in `folder`, before or after Start, in Edit. Returns false, and sets
    // Error, when the project or its start scene cannot be read.
    PIXIE_RUNTIME_API bool OpenProject(const std::filesystem::path& folder);
    bool HasProject() const { return opened_.has_value(); }
    // The open project; only when HasProject.
    const OpenedProject& OpenedProjectInfo() const { return *opened_; }
    // The project file's name when a project is open, else the Project's.
    PIXIE_RUNTIME_API std::string_view ProjectName() const;
    // Why the last OpenProject, Play, StopPlaying, or Save failed.
    const std::string& Error() const { return error_; }

    // Creates the renderers and builds the World for a viewport of `size` pixels.
    PIXIE_RUNTIME_API bool Start(FramebufferSize size);
    PIXIE_RUNTIME_API void Stop();

    // Throws the World away and builds it again, with the same seed, after the project has captured
    // what it starts from (Project::Capture). With a project open, it is built from the scene's
    // entities as they are now. A contracted candidate factory reconstructs a fresh
    // Project and skips Capture on the published instance.
    PIXIE_RUNTIME_API void Reset();

    // Builds an Edit candidate and publishes it only on success. Failure does not replace the
    // session-owned World or advance its revision. Project callbacks can still mutate retained
    // World references or external state; those effects are not rolled back. See Error.
    PIXIE_RUNTIME_API bool RestoreAuthoredScene(std::string_view snapshot);
    PIXIE_RUNTIME_API bool RestoreAuthoredScene(std::string_view snapshot, SceneBuildTimings* timings, bool validateMovement = false,
                                               bool preserveRandom = false);

    SessionMode Mode() const { return mode_; }
    // From Edit to Play; needs a project.
    PIXIE_RUNTIME_API bool Play();
    // From Play back to Edit, with the scene as it was at Play; needs a project.
    PIXIE_RUNTIME_API bool StopPlaying();

    // Writes the scene's entities to the start scene's file; in Play, the ones Play kept.
    // A recovered v2 snapshot cannot replace a v1 file without explicit backed-up conversion.
    PIXIE_RUNTIME_API bool Save();
    // Explicitly saves current Edit authorship as v2 with an original-file .v1-backup.
    // Does not replace World storage or add a format-undo entry.
    PIXIE_RUNTIME_API bool ConvertStartSceneToV2();
    // Whether the scene was edited since it was opened or saved.
    bool HasUnsavedChanges() const { return unsaved_; }
    // Notes that the scene was edited; ignored outside Edit.
    PIXIE_RUNTIME_API void MarkChanged();

    // Sets the viewport the camera maps the World into.
    PIXIE_RUNTIME_API void Resize(FramebufferSize size);

    // The host's input. Events go here; Advance forgets presses after each step.
    Input& GetInput() { return input_; }

    // Adds a frame of wall time and runs the fixed steps it makes due, unless paused. In Edit it
    // runs no step; the middle button and the wheel move the camera.
    PIXIE_RUNTIME_API void Advance(double frameDelta);
    // Runs one fixed step now, paused or not; nothing in Edit.
    PIXIE_RUNTIME_API void StepOnce();
    PIXIE_RUNTIME_API void SetPaused(bool paused);
    bool IsPaused() const { return paused_; }
    // Session presentation only; retained across World rebuilds and never saved to a scene.
    void SetPhysicsDebug(std::uint32_t flags) { physicsDebug_ = flags & kPhysicsDebugAll2D; physicsDebugExplicit_ = true; }
    std::uint32_t PhysicsDebug() const { return physicsDebugExplicit_ ? physicsDebug_ : mode_ == SessionMode::Edit ? 17u : 0u; }
    // Lighting debug drawing and views in lit scenes (kLightingDebug*2D bits); until set, light bounds
    // in Edit and nothing in Play.
    void SetLightingDebug(std::uint32_t flags) { lightingDebug_ = flags & kLightingDebugAll2D; lightingDebugExplicit_ = true; }
    std::uint32_t LightingDebug() const {
        return lightingDebugExplicit_ ? lightingDebug_ : mode_ == SessionMode::Edit ? kLightingDebugLights2D : 0u;
    }
    // Presentation-only choice for this session's current viewport. It never
    // changes the session camera or follows the editor's entity selection.
    void SetViewportEnvironment(EnvironmentSelection2D selection) {
        if (environmentSelection_ != selection) environmentFallbackReported_ = false;
        environmentSelection_ = selection;
    }
    EnvironmentSelection2D ViewportEnvironmentSelection() const { return environmentSelection_; }
    // Re-resolves against the current World; no pointer survives Play/Stop/Reset.
    PIXIE_RUNTIME_API std::optional<ViewportEnvironment2D> ViewportEnvironment() const;

    // How fast simulated time runs against wall time in Advance: 0.5 is half speed. The fixed step
    // stays the same; a faster scale runs more steps per frame. Clamped to [kMinTimeScale,
    // kMaxTimeScale].
    PIXIE_RUNTIME_API void SetTimeScale(double scale);
    double TimeScale() const { return timeScale_; }
    static constexpr double kMinTimeScale = 0.01;
    static constexpr double kMaxTimeScale = 8.0;

    // One step with the caller's time and input: the project's BeforeStep, then World::Step;
    // nothing in Edit.
    PIXIE_RUNTIME_API void Update(double delta, const Input& input);

    // Draws the World into the bound framebuffer, interpolated `alpha` of the way between steps:
    // sprites of negative layers, then paths, then the other sprites, then particles, then shapes,
    // then the project's overlay, then its screen layer (Project::DrawScreen). Images whose files changed
    // are read again first, checked at most every SpriteImages::kCheckInterval seconds.
    PIXIE_RUNTIME_API void Render(double alpha);
    // Draws into the session's own render target, sized to the viewport, and returns its texture.
    PIXIE_RUNTIME_API unsigned RenderToTarget();
    // Presentation-only override. Does not change the stepper, pause or time scale.
    PIXIE_RUNTIME_API unsigned RenderToTarget(double alpha);
    // The interpolation of the last Advance, for Render.
    PIXIE_RUNTIME_API double Alpha() const;

    World& GetWorld() { return *world_; }
    // Changes whenever World storage is discarded/rebuilt; invalidates host caches/tokens
    // even when an allocator reuses the same address and entity handle numbers.
    std::uint64_t WorldRevision() const { return worldRevision_; }
    std::uint64_t Identity() const { return identity_; }
    std::uint64_t AuthoredRevision() const { return authoredRevision_; }
    // Conservative invalidation before an admitted authored mutation can invoke callbacks.
    // A rejected application/no-op may advance it; raw Registry writes are outside this contract.
    PIXIE_RUNTIME_API void BeginAuthoredMutation();
    // Optional view, including before Start and after Stop. Reacquire after rebuilding the World.
    Telemetry* GetTelemetry() { return world_ ? &world_->GetTelemetry() : nullptr; }
    Camera2D& GetCamera() { return camera_; } // Existing free/session camera, independent of authored choice.
    void SetViewportCamera(CameraSelection2D value) { cameraSelection_ = value; }
    CameraSelection2D ViewportCameraSelection() const { return cameraSelection_; }
    ViewportCamera2D ResolvedCamera() const { return ResolveViewportCamera2D(*world_, camera_, cameraSelection_); }
    PIXIE_RUNTIME_API unsigned RenderViewportToTarget(RenderTarget& target, FramebufferSize size,
        CameraSelection2D camera, EnvironmentSelection2D environment, double alpha,
        std::optional<ViewLighting2D> view = std::nullopt);
    PIXIE_RUNTIME_API ViewportCamera2D ResolveCameraForViewport(CameraSelection2D camera, FramebufferSize size) const;
    PIXIE_RUNTIME_API Entity PickViewport(CameraSelection2D camera, FramebufferSize size, pxPoint point, double tolerance = 0);

    Project& GetProject() { return *project_; }
    const ComponentTypes& Types() const { return types_; }
    // The images of the World's sprites, resolved against the project folder (the working
    // directory without a project).
    SpriteImages& Images() { return images_; }
    // The fonts of the screen layer's text, resolved as the images are.
    Fonts& GetFonts() { return fonts_; }

    // Duration of the last step, in milliseconds.
    double LastStepMilliseconds() const { return lastStepMilliseconds_; }

private:
    PIXIE_RUNTIME_API bool Build(bool resetCamera, std::string_view sceneText, SceneBuildTimings* timings = nullptr,
                                 bool validateMovement = false);
    // A lit frame's receiver or emission pass: the sprites, texts and shapes again, in that pass.
    void DrawLightingPass(double alpha, const TextSources& texts, const pxRect& view, LightingPass pass, const Camera2D& camera);

    const std::uint64_t identity_;
    std::uint64_t authoredRevision_ = 0;
    // May retain module code: destroyed after World, descriptors and Project.
    ProjectFactory candidateFactory_;
    std::unique_ptr<Project> project_;
    SessionSettings settings_;
    ComponentTypes types_;
    std::unique_ptr<World> world_;
    std::uint64_t worldRevision_ = 0;
    Camera2D camera_;
    CameraSelection2D cameraSelection_;
    std::optional<ViewLighting2D> viewOverride_;
    Input input_;
    FixedStepper stepper_;
    ShapeRenderer shapes_;
    SpriteRenderer sprites_;
    PathRenderer paths_;
    SpriteImages images_;
    TextRenderer text_;
    Fonts fonts_;
    std::chrono::steady_clock::time_point nextImageCheck_{};
    ParticleRenderer particles_;
    LightRenderer lighting_;
    std::vector<LightInstance> lights_;
    std::vector<OccluderEdge> occluders_;
    SpriteAlphaOutlines outlines_;
    // Lighting telemetry, registered in a World's collector at its first lit frame.
    struct LightingMetrics {
        MetricHandle cpu, gpuLights, gpuShadows, gpuCombine;
        MetricHandle lights, shadowed, shadowedDropped, edges, particleLights, particleDropped;
        std::uint64_t world = 0;
        std::uint64_t gpuSample = 0;
    };
    LightingMetrics lightingMetrics_;
    std::uint32_t lightingDebug_ = 0;
    bool lightingDebugExplicit_ = false;
    EnvironmentSelection2D environmentSelection_;
    bool environmentFallbackReported_ = false;
    RenderTarget target_;
    bool started_ = false;
    bool paused_ = false;
    double timeScale_ = 1.0;
    double lastStepMilliseconds_ = 0.0;
    std::optional<OpenedProject> opened_;
    // The scene the next Build starts from; empty for none.
    std::string sceneText_;
    // The scene's entities as they were at Play.
    std::string playedFrom_;
    SessionMode mode_ = SessionMode::Play;
    bool unsaved_ = false;
    std::uint32_t physicsDebug_ = 0;
    bool physicsDebugExplicit_ = false;
    std::string error_;
};

} // namespace pixie
