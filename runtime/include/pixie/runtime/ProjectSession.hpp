#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/FixedStepper.hpp"
#include "pixie/Input.hpp"
#include "pixie/ParticleRenderer.hpp"
#include "pixie/RenderTarget.hpp"
#include "pixie/ShapeRenderer.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Project.hpp"
#include "pixie/runtime/World.hpp"

#include <cstdint>
#include <memory>

namespace pixie {

struct SessionSettings {
    std::uint64_t seed = 1;
    double updateRate = 60.0;
};

// One running Project: its World, camera, renderers, input, and time. It is the engine's loop for a
// host that drives it, such as an editor: the host feeds input events, calls Advance with the wall
// time of each frame, and draws with Render or RenderToTarget. ProjectApp drives the same session
// from pixie::Run, through Update and Render.
//
// Start and every later call need the GL context current; Stop releases the GL objects.
class ProjectSession {
public:
    ProjectSession(std::unique_ptr<Project> project, SessionSettings settings);
    ~ProjectSession();
    ProjectSession(const ProjectSession&) = delete;
    ProjectSession& operator=(const ProjectSession&) = delete;

    // Creates the renderers and builds the World for a viewport of `size` pixels.
    bool Start(FramebufferSize size);
    void Stop();

    // Throws the World away and builds it again from the project, with the same seed.
    void Reset();

    // Sets the viewport the camera maps the World into.
    void Resize(FramebufferSize size);

    // The host's input. Events go here; Advance forgets presses after each step.
    Input& GetInput() { return input_; }

    // Adds a frame of wall time and runs the fixed steps it makes due, unless paused.
    void Advance(double frameDelta);
    // Runs one fixed step now, paused or not.
    void StepOnce();
    void SetPaused(bool paused);
    bool IsPaused() const { return paused_; }

    // One step with the caller's time and input: the project's BeforeStep, then World::Step.
    void Update(double delta, const Input& input);

    // Draws the World into the bound framebuffer, interpolated `alpha` of the way between steps.
    void Render(double alpha);
    // Draws into the session's own render target, sized to the viewport, and returns its texture.
    unsigned RenderToTarget();
    // The interpolation of the last Advance, for Render.
    double Alpha() const;

    World& GetWorld() { return *world_; }
    Camera2D& GetCamera() { return camera_; }
    Project& GetProject() { return *project_; }
    const ComponentTypes& Types() const { return types_; }

    // Duration of the last step, in milliseconds.
    double LastStepMilliseconds() const { return lastStepMilliseconds_; }

private:
    void Build();

    std::unique_ptr<Project> project_;
    SessionSettings settings_;
    ComponentTypes types_;
    std::unique_ptr<World> world_;
    Camera2D camera_;
    Input input_;
    FixedStepper stepper_;
    ShapeRenderer shapes_;
    ParticleRenderer particles_;
    RenderTarget target_;
    bool started_ = false;
    bool paused_ = false;
    double lastStepMilliseconds_ = 0.0;
};

} // namespace pixie
