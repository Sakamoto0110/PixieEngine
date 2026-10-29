#pragma once

#include "pixie/runtime/Export.hpp"

#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/core/FixedStepper.hpp"
#include "pixie/platform/Input.hpp"
#include "pixie/render/2d/ParticleRenderer.hpp"
#include "pixie/render/2d/PathRenderer.hpp"
#include "pixie/render/RenderTarget.hpp"
#include "pixie/render/2d/ShapeRenderer.hpp"
#include "pixie/render/2d/SpriteRenderer.hpp"
#include "pixie/render/2d/TextRenderer.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Fonts.hpp"
#include "pixie/runtime/Project.hpp"
#include "pixie/runtime/ProjectFile.hpp"
#include "pixie/runtime/SpriteImages.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/PhysicsDebug2D.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace pixie {

struct SessionSettings {
    // The World's seed; a scene's own seed replaces it.
    std::uint64_t seed = 1;
    double updateRate = 60.0;
};

// What a session does with time. In Play the World steps; in Edit it stays as built while the scene
// is edited. A session without a project always plays.
enum class SessionMode { Play, Edit };

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
    PIXIE_RUNTIME_API ProjectSession(std::unique_ptr<Project> project, SessionSettings settings);
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
    // entities as they are now.
    PIXIE_RUNTIME_API void Reset();

    SessionMode Mode() const { return mode_; }
    // From Edit to Play; needs a project.
    PIXIE_RUNTIME_API bool Play();
    // From Play back to Edit, with the scene as it was at Play; needs a project.
    PIXIE_RUNTIME_API bool StopPlaying();

    // Writes the scene's entities to the start scene's file; in Play, the ones Play kept.
    PIXIE_RUNTIME_API bool Save();
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
    // The interpolation of the last Advance, for Render.
    PIXIE_RUNTIME_API double Alpha() const;

    World& GetWorld() { return *world_; }
    // Changes whenever World storage is discarded/rebuilt; invalidates host caches/tokens
    // even when an allocator reuses the same address and entity handle numbers.
    std::uint64_t WorldRevision() const { return worldRevision_; }
    // Optional view, including before Start and after Stop. Reacquire after rebuilding the World.
    Telemetry* GetTelemetry() { return world_ ? &world_->GetTelemetry() : nullptr; }
    Camera2D& GetCamera() { return camera_; }
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
    PIXIE_RUNTIME_API void Build(bool resetCamera);

    std::unique_ptr<Project> project_;
    SessionSettings settings_;
    ComponentTypes types_;
    std::unique_ptr<World> world_;
    std::uint64_t worldRevision_ = 0;
    Camera2D camera_;
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
