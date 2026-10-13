#include "pixie/runtime/ProjectSession.hpp"

#include "pixie/Gl.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/ShapeDrawing.hpp"

#include <algorithm>
#include <chrono>

namespace pixie {

ProjectSession::ProjectSession(std::unique_ptr<Project> project, SessionSettings settings)
    : project_(std::move(project)), settings_(settings), stepper_(settings.updateRate) {
    DescribeRuntimeComponents(types_);
    project_->Describe(types_);
}

ProjectSession::~ProjectSession() {
    // The World may hold no GL objects, but the renderers do: Stop must have run.
    world_.reset();
}

bool ProjectSession::Start(FramebufferSize size) {
    if (!shapes_.Create() || !particles_.Create()) {
        return false;
    }
    camera_.SetViewport(size);
    started_ = true;
    Build();
    return true;
}

void ProjectSession::Stop() {
    world_.reset();
    target_.Destroy();
    particles_.Destroy();
    shapes_.Destroy();
    started_ = false;
}

void ProjectSession::Build() {
    world_.reset();
    world_ = std::make_unique<World>(settings_.seed);
    camera_.SetCenter(pxPoint(0.0, 0.0));
    camera_.SetZoom(1.0);
    project_->Build(*world_, camera_);
}

void ProjectSession::Reset() {
    if (world_) {
        project_->Capture(*world_);
    }
    Build();
    stepper_.Drop();
}

void ProjectSession::Resize(FramebufferSize size) {
    camera_.SetViewport(size);
}

void ProjectSession::Update(double delta, const Input& input) {
    project_->BeforeStep(*world_, camera_, input);
    const auto start = std::chrono::steady_clock::now();
    world_->Step(delta, input);
    lastStepMilliseconds_ =
        std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

void ProjectSession::Advance(double frameDelta) {
    if (paused_) {
        return;
    }
    stepper_.Advance(frameDelta * timeScale_, [this](const FrameTime& time) {
        Update(time.delta, input_);
        input_.EndUpdate();
    });
}

void ProjectSession::StepOnce() {
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
    return paused_ ? 1.0 : stepper_.RenderTime(0.0, 0).alpha;
}

void ProjectSession::Render(double alpha) {
    const pxColorRgba background = project_->Background();
    glClearColor(background.r / 255.0f, background.g / 255.0f, background.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    DrawParticles(*world_, particles_, camera_, alpha);
    SubmitShapes(*world_, shapes_, alpha);
    project_->DrawOverlay(*world_, shapes_, camera_);
    shapes_.Flush(camera_);
}

unsigned ProjectSession::RenderToTarget() {
    const FramebufferSize size = camera_.Viewport();
    if (size.width <= 0 || size.height <= 0 || !target_.Resize(size)) {
        return 0;
    }
    target_.Bind();
    Render(Alpha());
    RenderTarget::BindDefault();
    return target_.ColorTexture();
}

} // namespace pixie
