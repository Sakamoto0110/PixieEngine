#pragma once

#include "StepTimer.hpp"

#include "pixie/App.hpp"
#include "pixie/Camera2D.hpp"
#include "pixie/ParticleRenderer.hpp"
#include "pixie/ShapeRenderer.hpp"
#include "pixie/runtime/World.hpp"

#include <cstdint>
#include <memory>

struct ParticlesOptions {
    int count = 10000;
    int attractors = 1;
    bool interaction = true;
    std::uint64_t seed = 1;
};

// Anchor 3: particles orbiting attractors that the user places, drags, and removes.
class ParticlesApp final : public pixie::App {
public:
    explicit ParticlesApp(const ParticlesOptions& options) : options_(options) {}

    bool Start(pixie::Window& window) override;
    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Render(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Stop() override;

private:
    void HandleMouse(const pixie::Input& input);
    void HandleKeys(const pixie::Input& input);
    void Respawn(int count);
    // The attractor whose ring is under the screen point, if any.
    pixie::Entity AttractorAt(pxPoint screen);

    ParticlesOptions options_;
    int count_ = 0;
    pixie::Window* window_ = nullptr;
    pixie::Camera2D camera_;
    pixie::ShapeRenderer shapes_;
    pixie::ParticleRenderer particles_;
    std::unique_ptr<pixie::World> world_;
    pixie::Entity emitter_;
    pixie::Entity dragged_;
    sample::StepTimer stepTimer_{"particles step"};
};
