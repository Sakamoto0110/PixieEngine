#pragma once

#include "pixie/App.hpp"
#include "pixie/Camera2D.hpp"
#include "pixie/ShapeRenderer.hpp"

#include <cstdint>
#include <vector>

// Thousands of moving shapes in a square world, seen through a camera that pans by dragging and
// zooms around the cursor; a ring marks the world point under the cursor.
class ShapesApp final : public pixie::App {
public:
    ShapesApp(int count, std::uint32_t seed) : count_(count), seed_(seed) {}

    bool Start(pixie::Window& window) override;
    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Render(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Stop() override;

private:
    struct MovingShape {
        pixie::ShapeKind kind = pixie::ShapeKind::Circle;
        pxPoint position;
        pxPoint previousPosition;
        pxVec2 velocity;
        double rotation = 0.0;
        double previousRotation = 0.0;
        double spin = 0.0;
        pxVec2f scale;
        pxColorRgba color;
    };

    void ResetCamera();
    void TrackViewport();

    int count_;
    std::uint32_t seed_;
    bool paused_ = false;
    pixie::Window* window_ = nullptr;
    pixie::Camera2D camera_;
    pixie::ShapeRenderer renderer_;
    std::vector<MovingShape> shapes_;
};
