#include "ShapesApp.hpp"

#include "pixie/render/RenderTarget.hpp"

#include <cmath>
#include <numbers>
#include <random>

namespace {

// The shapes move inside [-kHalfWorld, kHalfWorld] on both axes.
constexpr double kHalfWorld = 2000.0;
constexpr double kZoomStep = 1.15;
constexpr double kMarkerPixels = 14.0;

constexpr pxColorRgba kBackground{20, 23, 31};
constexpr pxColorRgba kBorder{70, 78, 98};
constexpr pxColorRgba kMarker{255, 255, 255};

constexpr pxColorRgba kPalette[] = {
    {245, 92, 107}, {250, 186, 69}, {107, 214, 130}, {77, 171, 245}, {181, 125, 245}, {250, 140, 209},
};

pxPointf ToFloat(pxPoint p) {
    return pxPointf(static_cast<float>(p.x), static_cast<float>(p.y));
}

} // namespace

bool ShapesApp::Start(pixie::Window& window) {
    window_ = &window;
    if (!renderer_.Create()) {
        return false;
    }

    std::mt19937 random(seed_);
    std::uniform_real_distribution<double> coordinate(-kHalfWorld, kHalfWorld);
    std::uniform_real_distribution<double> speed(20.0, 160.0);
    std::uniform_real_distribution<double> angle(0.0, 2.0 * std::numbers::pi);
    std::uniform_real_distribution<double> spin(-2.0, 2.0);
    std::uniform_real_distribution<float> size(4.0f, 14.0f);
    std::uniform_int_distribution<int> kind(0, pixie::kShapeKindCount - 1);
    std::uniform_int_distribution<std::size_t> color(0, std::size(kPalette) - 1);

    shapes_.resize(static_cast<std::size_t>(count_));
    for (MovingShape& shape : shapes_) {
        shape.kind = static_cast<pixie::ShapeKind>(kind(random));
        shape.position = pxPoint(coordinate(random), coordinate(random));
        shape.previousPosition = shape.position;
        const double direction = angle(random);
        shape.velocity = pxVec2(std::cos(direction), std::sin(direction)) * speed(random);
        shape.rotation = angle(random);
        shape.previousRotation = shape.rotation;
        shape.spin = spin(random);
        const float s = size(random);
        shape.scale = shape.kind == pixie::ShapeKind::Triangle ? pxVec2f(s * 1.4f, s) : pxVec2f(s, s);
        shape.color = kPalette[color(random)];
    }

    TrackViewport();
    ResetCamera();
    return true;
}

void ShapesApp::ResetCamera() {
    camera_.SetCenter(pxPoint(0.0, 0.0));
    const pixie::FramebufferSize viewport = camera_.Viewport();
    const double fit = std::min(viewport.width, viewport.height) / (2.0 * kHalfWorld);
    camera_.SetZoom(fit > 0.0 ? fit * 0.95 : 1.0);
}

void ShapesApp::TrackViewport() {
    camera_.SetViewport(window_->GetFramebufferSize());
}

void ShapesApp::Update(const pixie::FrameTime& time, const pixie::Input& input) {
    TrackViewport();
    if (input.IsDown(pixie::MouseButton::Left) || input.IsDown(pixie::MouseButton::Right)) {
        camera_.Pan(input.CursorMoved());
    }
    if (input.Scroll().y != 0.0) {
        camera_.ZoomAt(input.Cursor(), std::pow(kZoomStep, input.Scroll().y));
    }
    if (input.WasPressed(pixie::Key::Space)) {
        paused_ = !paused_;
    }
    if (input.WasPressed(pixie::Key::Home)) {
        ResetCamera();
    }

    for (MovingShape& shape : shapes_) {
        shape.previousPosition = shape.position;
        shape.previousRotation = shape.rotation;
        if (paused_) {
            continue;
        }
        shape.position = shape.position + shape.velocity * time.delta;
        shape.rotation += shape.spin * time.delta;
        if (std::abs(shape.position.x) > kHalfWorld) {
            shape.velocity.x = -shape.velocity.x;
            shape.position.x = std::copysign(kHalfWorld, shape.position.x);
        }
        if (std::abs(shape.position.y) > kHalfWorld) {
            shape.velocity.y = -shape.velocity.y;
            shape.position.y = std::copysign(kHalfWorld, shape.position.y);
        }
    }
}

void ShapesApp::Render(const pixie::FrameTime& time, const pixie::Input& input) {
    TrackViewport();
    pixie::ClearColor(kBackground.r / 255.0f, kBackground.g / 255.0f, kBackground.b / 255.0f, 1.0f);

    // The world's border, four thin quads.
    const auto edge = static_cast<float>(kHalfWorld);
    const float thickness = static_cast<float>(1.5 / camera_.Zoom());
    renderer_.Submit(pixie::ShapeKind::Quad, {{0.0f, -edge}, 0.0f, {edge, thickness}, kBorder});
    renderer_.Submit(pixie::ShapeKind::Quad, {{0.0f, edge}, 0.0f, {edge, thickness}, kBorder});
    renderer_.Submit(pixie::ShapeKind::Quad, {{-edge, 0.0f}, 0.0f, {thickness, edge}, kBorder});
    renderer_.Submit(pixie::ShapeKind::Quad, {{edge, 0.0f}, 0.0f, {thickness, edge}, kBorder});

    for (const MovingShape& shape : shapes_) {
        pixie::ShapeInstance instance;
        instance.position = ToFloat(pxPoint::Lerp(shape.previousPosition, shape.position, time.alpha));
        instance.rotation = static_cast<float>(std::lerp(shape.previousRotation, shape.rotation, time.alpha));
        instance.scale = shape.scale;
        instance.color = shape.color;
        renderer_.Submit(shape.kind, instance);
    }
    renderer_.Flush(camera_.MakeView());

    // The marker is drawn last, so it stays on top; it keeps its size on screen at every zoom.
    const pxPoint cursor = camera_.ScreenToWorld(input.Cursor());
    const auto marker = static_cast<float>(kMarkerPixels / camera_.Zoom());
    renderer_.Submit(pixie::ShapeKind::Ring, {ToFloat(cursor), 0.0f, {marker, marker}, kMarker});
    renderer_.Submit(pixie::ShapeKind::Circle,
                     {ToFloat(cursor), 0.0f, {marker * 0.15f, marker * 0.15f}, kMarker});
    renderer_.Flush(camera_.MakeView());
}

void ShapesApp::Stop() {
    renderer_.Destroy();
}
