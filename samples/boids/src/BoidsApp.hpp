#pragma once

#include "StepTimer.hpp"

#include "pixie/App.hpp"
#include "pixie/Camera2D.hpp"
#include "pixie/ShapeRenderer.hpp"
#include "pixie/runtime/World.hpp"

#include <cstdint>
#include <memory>

// Anchor 1: boids that steer toward the mouse, or away from it, within a radius.
class BoidsApp final : public pixie::App {
public:
    BoidsApp(int count, std::uint64_t seed) : initialCount_(count), seed_(seed) {}

    bool Start(pixie::Window& window) override;
    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Render(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Stop() override;

private:
    void TrackView(const pixie::Input& input);
    void HandleKeys(const pixie::Input& input);

    int initialCount_;
    std::uint64_t seed_;
    pixie::Window* window_ = nullptr;
    pixie::Camera2D camera_;
    pixie::ShapeRenderer renderer_;
    std::unique_ptr<pixie::World> world_;
    pixie::Entity controls_;
    sample::StepTimer stepTimer_{"boids step"};
};
