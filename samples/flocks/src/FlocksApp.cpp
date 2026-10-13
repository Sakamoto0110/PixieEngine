#include "FlocksApp.hpp"

#include "FlockControls.hpp"
#include "FlocksScene.hpp"

#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"
#include "pixie/runtime/ShapeDrawing.hpp"

#include <algorithm>
#include <format>

namespace {

constexpr int kCountStep = 500;
constexpr int kMaxCount = 100000;
constexpr double kWeightStep = 0.1;
constexpr pxColorRgba kBackground{12, 14, 22};
constexpr pxColorRgba kAttractColor{110, 230, 140, 110};
constexpr pxColorRgba kRepelColor{250, 110, 110, 110};

pxPointf ToFloat(pxPoint p) {
    return pxPointf(static_cast<float>(p.x), static_cast<float>(p.y));
}

// Raises the weight with one key and lowers it with another; returns true if it changed.
bool Tune(const pixie::Input& input, pixie::Key up, pixie::Key down, double& weight) {
    const double before = weight;
    if (input.WasPressed(up)) {
        weight += kWeightStep;
    }
    if (input.WasPressed(down)) {
        weight = std::max(0.0, weight - kWeightStep);
    }
    return weight != before;
}

} // namespace

bool FlocksApp::Start(pixie::Window& window) {
    window_ = &window;
    if (!renderer_.Create()) {
        return false;
    }
    camera_.SetViewport(window.GetFramebufferSize());
    world_ = std::make_unique<pixie::World>(seed_);
    flock_ = SetUpFlocks(*world_, camera_.VisibleWorld(), initialCount_);
    pixie::LogInfo(std::format(
        "flocks: {} boids; Q/A separation, W/S alignment, E/D cohesion, Space attract or repel, "
        "Up and Down change the count",
        FlockSize(*world_)));
    return true;
}

void FlocksApp::TrackView(const pixie::Input& input) {
    camera_.SetViewport(window_->GetFramebufferSize());
    flock_.Get<Bounds>().area = camera_.VisibleWorld();
    const pixie::FramebufferSize viewport = camera_.Viewport();
    const pxPoint cursor = input.Cursor();
    Pointer& pointer = flock_.Get<Pointer>();
    pointer.position = camera_.ScreenToWorld(cursor);
    pointer.active = cursor.x >= 0.0 && cursor.y >= 0.0 && cursor.x < viewport.width &&
                     cursor.y < viewport.height;
}

void FlocksApp::HandleKeys(const pixie::Input& input) {
    FlockControls& c = flock_.Get<FlockControls>();
    bool changed = Tune(input, pixie::Key::Q, pixie::Key::A, c.separationWeight);
    changed |= Tune(input, pixie::Key::W, pixie::Key::S, c.alignmentWeight);
    changed |= Tune(input, pixie::Key::E, pixie::Key::D, c.cohesionWeight);
    if (changed) {
        pixie::LogInfo(std::format("flocks: separation {:.1f}, alignment {:.1f}, cohesion {:.1f}",
                                   c.separationWeight, c.alignmentWeight, c.cohesionWeight));
    }
    if (input.WasPressed(pixie::Key::Space)) {
        c.pointerMode = c.pointerMode == PointerMode::Attract ? PointerMode::Repel : PointerMode::Attract;
        pixie::LogInfo(c.pointerMode == PointerMode::Attract ? "flocks: attracting" : "flocks: repelling");
    }
    int count = FlockSize(*world_);
    if (input.WasPressed(pixie::Key::Up)) {
        count = std::min(count + kCountStep, kMaxCount);
    }
    if (input.WasPressed(pixie::Key::Down)) {
        count = std::max(count - kCountStep, 0);
    }
    if (count != FlockSize(*world_)) {
        SetFlockSize(*world_, flock_, count);
        pixie::LogInfo(std::format("flocks: {} boids", count));
    }
}

void FlocksApp::Update(const pixie::FrameTime& time, const pixie::Input& input) {
    TrackView(input);
    HandleKeys(input);
    stepTimer_.Begin();
    world_->Step(time.delta, input);
    stepTimer_.End();
}

void FlocksApp::Render(const pixie::FrameTime& time, const pixie::Input&) {
    camera_.SetViewport(window_->GetFramebufferSize());
    glClearColor(kBackground.r / 255.0f, kBackground.g / 255.0f, kBackground.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const Pointer& pointer = flock_.Get<Pointer>();
    const FlockControls& c = flock_.Get<FlockControls>();
    if (pointer.active) {
        const auto radius = static_cast<float>(c.pointerRadius);
        renderer_.Submit(pixie::ShapeKind::Ring,
                         {ToFloat(pointer.position), 0.0f, {radius, radius},
                          c.pointerMode == PointerMode::Attract ? kAttractColor : kRepelColor});
    }
    pixie::SubmitShapes(*world_, renderer_, time.alpha);
    renderer_.Flush(camera_);
}

void FlocksApp::Stop() {
    world_.reset();
    renderer_.Destroy();
}
