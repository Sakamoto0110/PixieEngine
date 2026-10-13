#include "ParticlesApp.hpp"

#include "ParticlesScene.hpp"

#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/ParticleEmitter.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/ShapeDrawing.hpp"
#include "pixie/runtime/Transform2D.hpp"

#include <array>
#include <cmath>
#include <format>
#include <numbers>

namespace {

constexpr std::array kPresets = {1, 10, 100, 10000, 100000};
constexpr pixie::Key kPresetKeys[] = {pixie::Key::Num1, pixie::Key::Num2, pixie::Key::Num3,
                                      pixie::Key::Num4, pixie::Key::Num5};
// Interaction above this many particles is left to the user to turn on.
constexpr int kInteractionLimit = 20000;
constexpr double kPickPixels = 18.0;
constexpr double kZoomStep = 1.15;
constexpr double kAttractorRing = 250.0;
constexpr pxColorRgba kBackground{6, 8, 14};

} // namespace

bool ParticlesApp::Start(pixie::Window& window) {
    window_ = &window;
    if (!shapes_.Create() || !particles_.Create()) {
        return false;
    }
    camera_.SetViewport(window.GetFramebufferSize());
    camera_.SetCenter(pxPoint(0.0, 0.0));
    world_ = std::make_unique<pixie::World>(options_.seed);
    emitter_ = SetUpParticles(*world_);
    // One attractor at the middle; more on a ring around it.
    for (int i = 0; i < options_.attractors; ++i) {
        const double angle = 2.0 * std::numbers::pi * i / options_.attractors;
        const double ring = options_.attractors == 1 ? 0.0 : kAttractorRing;
        AddAttractor(*world_, pxPoint(ring * std::cos(angle), ring * std::sin(angle)));
    }
    SetInteraction(emitter_, options_.interaction);
    Respawn(options_.count);
    pixie::LogInfo("particles: 1-5 for 1, 10, 100, 10000, 100000 particles; I interaction; "
                   "left click adds or drags an attractor, right click removes it; "
                   "middle drag pans, the wheel zooms");
    return true;
}

void ParticlesApp::Respawn(int count) {
    count_ = count;
    SpawnOrbits(*world_, emitter_, count);
    pixie::LogInfo(std::format("particles: {} particles, {} attractors, interaction {}", count,
                               AttractorCount(*world_), InteractionOn(emitter_) ? "on" : "off"));
}

pixie::Entity ParticlesApp::AttractorAt(pxPoint screen) {
    pixie::Entity found;
    double best = kPickPixels;
    world_->Each<const pixie::Transform2D, const pixie::Attractor>(
        [&](pixie::Entity entity, const pixie::Transform2D& transform, const pixie::Attractor&) {
            const double distance = glm::length(camera_.WorldToScreen(transform.position) - screen);
            if (distance <= best) {
                best = distance;
                found = entity;
            }
        });
    return found;
}

void ParticlesApp::HandleMouse(const pixie::Input& input) {
    const pxPoint cursor = input.Cursor();
    if (input.IsDown(pixie::MouseButton::Middle)) {
        camera_.Pan(input.CursorMoved());
    }
    if (input.Scroll().y != 0.0) {
        camera_.ZoomAt(cursor, std::pow(kZoomStep, input.Scroll().y));
    }
    if (input.WasPressed(pixie::MouseButton::Left)) {
        dragged_ = AttractorAt(cursor);
        if (!dragged_ && AttractorCount(*world_) < kMaxAttractors) {
            dragged_ = AddAttractor(*world_, camera_.ScreenToWorld(cursor));
            pixie::LogInfo(std::format("particles: {} attractors", AttractorCount(*world_)));
        }
    }
    if (!input.IsDown(pixie::MouseButton::Left)) {
        dragged_ = {};
    }
    if (dragged_.IsAlive()) {
        dragged_.Get<pixie::Transform2D>().position = camera_.ScreenToWorld(cursor);
        world_->ResetInterpolation(dragged_);
    }
    if (input.WasPressed(pixie::MouseButton::Right)) {
        if (pixie::Entity target = AttractorAt(cursor)) {
            target.Destroy();
            pixie::LogInfo(std::format("particles: {} attractors", AttractorCount(*world_)));
        }
    }
}

void ParticlesApp::HandleKeys(const pixie::Input& input) {
    for (std::size_t i = 0; i < kPresets.size(); ++i) {
        if (input.WasPressed(kPresetKeys[i])) {
            if (kPresets[i] > kInteractionLimit && InteractionOn(emitter_)) {
                SetInteraction(emitter_, false);
            }
            Respawn(kPresets[i]);
        }
    }
    if (input.WasPressed(pixie::Key::I)) {
        SetInteraction(emitter_, !InteractionOn(emitter_));
        pixie::LogInfo(std::format("particles: interaction {}", InteractionOn(emitter_) ? "on" : "off"));
    }
    if (input.WasPressed(pixie::Key::R)) {
        Respawn(count_);
    }
}

void ParticlesApp::Update(const pixie::FrameTime& time, const pixie::Input& input) {
    camera_.SetViewport(window_->GetFramebufferSize());
    HandleMouse(input);
    HandleKeys(input);
    stepTimer_.Begin();
    world_->Step(time.delta, input);
    stepTimer_.End();
}

void ParticlesApp::Render(const pixie::FrameTime& time, const pixie::Input&) {
    camera_.SetViewport(window_->GetFramebufferSize());
    glClearColor(kBackground.r / 255.0f, kBackground.g / 255.0f, kBackground.b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    pixie::DrawParticles(*world_, particles_, camera_, time.alpha);
    pixie::SubmitShapes(*world_, shapes_, time.alpha);
    shapes_.Flush(camera_);
}

void ParticlesApp::Stop() {
    world_.reset();
    particles_.Destroy();
    shapes_.Destroy();
}
