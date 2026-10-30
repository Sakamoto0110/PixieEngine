#include "ParticlesProject.hpp"

#include "ParticlesScene.hpp"

#include "pixie/core/Log.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/ScreenCanvas.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <string_view>

namespace {

constexpr std::array kPresets = {1, 10, 100, 10000, 100000};
constexpr pixie::Key kPresetKeys[] = {pixie::Key::Num1, pixie::Key::Num2, pixie::Key::Num3,
                                      pixie::Key::Num4, pixie::Key::Num5};
// Interaction above this many particles is left to the user to turn on.
constexpr int kInteractionLimit = 20000;
constexpr double kPickPixels = 18.0;
constexpr double kZoomStep = 1.15;

// The screen layer's font, copied into the project from the shared resources (vdr cp inter-regular).
constexpr std::string_view kFont = "fonts/inter-regular.ttf";

} // namespace

void ParticlesProject::Describe(pixie::ComponentTypes& types) const {
    types.Add<ParticlesScenario>("ParticlesScenario")
        .Scenario()
        .Field<&ParticlesScenario::count>("count")
        .Tooltip("How many particles the scene starts with. Rebuild the scene to apply.")
        .Range(0.0, 200000.0, 1.0)
        .Scrub(10.0)
        .Field<&ParticlesScenario::interaction>("interaction")
        .Tooltip("Whether the scene starts with particle-to-particle interaction.");
}

void ParticlesProject::Build(pixie::World& world, pixie::Camera2D&) {
    // Without a scene, the World starts with what the sample's scene holds: the spawner on the left
    // and one attractor in the middle.
    const auto scenarios = world.Registry().view<ParticlesScenario>();
    const bool defaults = scenarios.begin() == scenarios.end();
    scenarioEntity_ = pixie::SceneEntityWith<ParticlesScenario>(world, "scenario", scenario_);
    const ParticlesScenario scenario = scenarioEntity_.Get<ParticlesScenario>();
    emitter_ = pixie::SceneEntityWith<pixie::ParticleSystem>(world, "particles");
    SetUpParticles(world, emitter_, defaults);
    if (defaults) {
        AddAttractor(world, pxPoint(0.0, 0.0));
    }
    dragged_ = {};
    // The scenario says whether the scene starts with the interaction.
    SetInteraction(emitter_, scenario.interaction);
    Respawn(world, scenario.count);
    pixie::LogInfo("particles: 1-5 for 1, 10, 100, 10000, 100000 particles; I interaction; "
                   "left click adds or drags an attractor, Alt + left click adds a spawner, "
                   "a left drag moves either, right click removes it; "
                   "middle drag pans, the wheel zooms");
}

void ParticlesProject::Capture(pixie::World&) {
    if (const auto* edited = scenarioEntity_.TryGet<ParticlesScenario>()) {
        scenario_ = *edited;
    }
}

void ParticlesProject::Respawn(pixie::World& world, int count) {
    count_ = count;
    SpawnOrbits(world, emitter_, count);
    pixie::LogInfo(std::format("particles: {} particles, {} attractors, interaction {}", count,
                               AttractorCount(world), InteractionOn(emitter_) ? "on" : "off"));
}

pixie::Entity ParticlesProject::HandleAt(pixie::World& world, const pixie::Camera2D& camera, pxPoint screen) {
    pixie::Entity found;
    double best = kPickPixels;
    const auto consider = [&](pixie::Entity entity, const pixie::Transform2D& transform) {
        const double distance = glm::length(camera.WorldToScreen(transform.position) - screen);
        if (distance <= best) {
            best = distance;
            found = entity;
        }
    };
    world.Each<const pixie::Transform2D, const pixie::Attractor>(
        [&](pixie::Entity entity, const pixie::Transform2D& transform, const pixie::Attractor&) {
            consider(entity, transform);
        });
    world.Each<const pixie::Transform2D, const pixie::ParticleSpawner>(
        [&](pixie::Entity entity, const pixie::Transform2D& transform, const pixie::ParticleSpawner&) {
            consider(entity, transform);
        });
    return found;
}

void ParticlesProject::HandleMouse(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) {
    const pxPoint cursor = input.Cursor();
    if (input.IsDown(pixie::MouseButton::Middle)) {
        camera.Pan(input.CursorMoved());
    }
    if (input.Scroll().y != 0.0) {
        camera.ZoomAt(cursor, std::pow(kZoomStep, input.Scroll().y));
    }
    if (input.WasPressed(pixie::MouseButton::Left)) {
        dragged_ = HandleAt(world, camera, cursor);
        // On empty space: an attractor, or with Alt a spawner, which starts at rate 0.
        const bool alt = input.IsDown(pixie::Key::LeftAlt) || input.IsDown(pixie::Key::RightAlt);
        if (!dragged_ && alt && SpawnerCount(world) < kMaxSpawners) {
            dragged_ = AddSpawner(world, emitter_, camera.ScreenToWorld(cursor));
            pixie::LogInfo(std::format("particles: {} spawners", SpawnerCount(world)));
        } else if (!dragged_ && !alt && AttractorCount(world) < kMaxAttractors) {
            dragged_ = AddAttractor(world, camera.ScreenToWorld(cursor));
            pixie::LogInfo(std::format("particles: {} attractors", AttractorCount(world)));
        }
    }
    if (!input.IsDown(pixie::MouseButton::Left)) {
        dragged_ = {};
    }
    if (dragged_.IsAlive()) {
        dragged_.Get<pixie::Transform2D>().position = camera.ScreenToWorld(cursor);
        world.ResetInterpolation(dragged_);
    }
    if (input.WasPressed(pixie::MouseButton::Right)) {
        if (pixie::Entity target = HandleAt(world, camera, cursor)) {
            const bool spawner = target.Has<pixie::ParticleSpawner>();
            target.Destroy();
            pixie::LogInfo(spawner ? std::format("particles: {} spawners", SpawnerCount(world))
                                   : std::format("particles: {} attractors", AttractorCount(world)));
        }
    }
}

void ParticlesProject::HandleKeys(pixie::World& world, const pixie::Input& input) {
    for (std::size_t i = 0; i < kPresets.size(); ++i) {
        if (input.WasPressed(kPresetKeys[i])) {
            if (kPresets[i] > kInteractionLimit) {
                SetInteraction(emitter_, false);
            }
            Respawn(world, kPresets[i]);
        }
    }
    if (input.WasPressed(pixie::Key::I)) {
        SetInteraction(emitter_, !InteractionOn(emitter_));
        pixie::LogInfo(std::format("particles: interaction {}", InteractionOn(emitter_) ? "on" : "off"));
    }
    if (input.WasPressed(pixie::Key::R)) {
        Respawn(world, count_);
    }
}

bool ParticlesProject::SpawnAt(pixie::World&, pxPoint at) {
    // A particle at rest that lives until removed; the attractors take it from there.
    return emitter_.Get<pixie::ParticleSystem>().Emit(
        pxPointf(static_cast<float>(at.x), static_cast<float>(at.y)), pxVec2f(0.0f), 0.0f);
}

void ParticlesProject::BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) {
    HandleMouse(world, camera, input);
    HandleKeys(world, input);
    // An attractor's ring is its body.
    world.Each<pixie::Shape, const pixie::Attractor>(
        [](pixie::Entity, pixie::Shape& shape, const pixie::Attractor& attractor) {
            shape.size = pxVec2(attractor.size, attractor.size);
        });

}

std::unique_ptr<pixie::Project> pixie::CreateProject() {
    return std::make_unique<ParticlesProject>();
}

void ParticlesProject::DrawScreen(pixie::World& world, pixie::ScreenCanvas& screen) {
    const pixie::TextStyle title{kFont, 18.0, pxColorRgba(236, 238, 245)};
    const pixie::TextStyle body{kFont, 14.0, pxColorRgba(170, 178, 196)};
    const int particles = static_cast<int>(emitter_.Get<pixie::ParticleSystem>().particles.size());
    const std::string counts = std::format("{} particles\n{} attractors, {} spawners\ninteraction {}", particles,
                                           AttractorCount(world), SpawnerCount(world),
                                           InteractionOn(emitter_) ? "on" : "off");

    // A panel sized to what it holds, in the top-left corner.
    constexpr double kMargin = 12.0;
    constexpr double kPadding = 10.0;
    const pxVec2 titleSize = screen.MeasureText("particles", title);
    const pxVec2 bodySize = screen.MeasureText(counts, body);
    const pxVec2 panel(std::max(titleSize.x, bodySize.x) + 2.0 * kPadding, titleSize.y + bodySize.y + 2.0 * kPadding + 4.0);
    screen.Rect(pxPoint(kMargin, kMargin), panel, pxColorRgba(16, 20, 32, 210));
    screen.Text("particles", pxPoint(kMargin + kPadding, kMargin + kPadding), title);
    screen.Text(counts, pxPoint(kMargin + kPadding, kMargin + kPadding + titleSize.y + 4.0), body);

    // Each spawner's rate, centered below it wherever the camera puts it.
    world.Each<const pixie::Transform2D, const pixie::ParticleSpawner>(
        [&](pixie::Entity, const pixie::Transform2D& transform, const pixie::ParticleSpawner& spawner) {
            const pxPoint at = screen.WorldToScreen(transform.position);
            const std::string rate = std::format("{:g}/s", spawner.rate);
            const pxVec2 size = screen.MeasureText(rate, body);
            screen.Text(rate, pxPoint(at.x - size.x * 0.5, at.y + 28.0), body);
        });
}
