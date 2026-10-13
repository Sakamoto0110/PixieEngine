#include "BoidsApp.hpp"

#include "BoidControls.hpp"
#include "BoidsScene.hpp"

#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"
#include "pixie/runtime/ShapeDrawing.hpp"

#include <algorithm>
#include <format>

namespace {

constexpr int kCountStep = 100;
constexpr int kMaxCount = 200000;
constexpr pxColorRgba kBackground{14, 17, 26};
constexpr pxColorRgba kAttractColor{110, 230, 140, 110};
constexpr pxColorRgba kRepelColor{250, 110, 110, 110};

pxPointf ToFloat(pxPoint p) {
    return pxPointf(static_cast<float>(p.x), static_cast<float>(p.y));
}

} // namespace

bool BoidsApp::Start(pixie::Window& window) {
    window_ = &window;
    if (!renderer_.Create()) {
        return false;
    }
    camera_.SetViewport(window.GetFramebufferSize());
    world_ = std::make_unique<pixie::World>(seed_);
    controls_ = SetUpBoids(*world_, camera_.VisibleWorld(), initialCount_);
    pixie::LogInfo(std::format("boids: {} boids, attracting; Space switches, Up and Down change the count",
                               BoidCount(*world_)));
    return true;
}

void BoidsApp::TrackView(const pixie::Input& input) {
    camera_.SetViewport(window_->GetFramebufferSize());
    controls_.Get<Bounds>().area = camera_.VisibleWorld();
    const pixie::FramebufferSize viewport = camera_.Viewport();
    const pxPoint cursor = input.Cursor();
    Pointer& pointer = controls_.Get<Pointer>();
    pointer.position = camera_.ScreenToWorld(cursor);
    pointer.active = cursor.x >= 0.0 && cursor.y >= 0.0 && cursor.x < viewport.width &&
                     cursor.y < viewport.height;
}

void BoidsApp::HandleKeys(const pixie::Input& input) {
    BoidControls& controls = controls_.Get<BoidControls>();
    if (input.WasPressed(pixie::Key::Space)) {
        controls.mode = controls.mode == SteerMode::Attract ? SteerMode::Repel : SteerMode::Attract;
        pixie::LogInfo(controls.mode == SteerMode::Attract ? "boids: attracting" : "boids: repelling");
    }
    int count = BoidCount(*world_);
    if (input.WasPressed(pixie::Key::Up)) {
        count = std::min(count + kCountStep, kMaxCount);
    }
    if (input.WasPressed(pixie::Key::Down)) {
        count = std::max(count - kCountStep, 0);
    }
    if (count != BoidCount(*world_)) {
        SetBoidCount(*world_, controls_, count);
        pixie::LogInfo(std::format("boids: {} boids", count));
    }
}

void BoidsApp::Update(const pixie::FrameTime& time, const pixie::Input& input) {
    TrackView(input);
    HandleKeys(input);
    stepTimer_.Begin();
    world_->Step(time.delta, input);
    stepTimer_.End();
}

void BoidsApp::Render(const pixie::FrameTime& time, const pixie::Input&) {
    camera_.SetViewport(window_->GetFramebufferSize());
    glClearColor(kBackground.r / 255.0f, kBackground.g / 255.0f, kBackground.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const Pointer& pointer = controls_.Get<Pointer>();
    const BoidControls& controls = controls_.Get<BoidControls>();
    if (pointer.active) {
        const auto radius = static_cast<float>(controls.radius);
        renderer_.Submit(pixie::ShapeKind::Ring,
                         {ToFloat(pointer.position), 0.0f, {radius, radius},
                          controls.mode == SteerMode::Attract ? kAttractColor : kRepelColor});
    }
    pixie::SubmitShapes(*world_, renderer_, time.alpha);
    renderer_.Flush(camera_);
}

void BoidsApp::Stop() {
    world_.reset();
    renderer_.Destroy();
}
