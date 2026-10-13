#include "ParticlesProject.hpp"

#include "ParticlesScene.hpp"

#include "pixie/Log.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/InteractionAffector.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

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

} // namespace

void ParticlesProject::Describe(pixie::ComponentTypes& types) const {
    types.Add<ParticlesScenario>("ParticlesScenario")
        .Scenario()
        .Field<&ParticlesScenario::count>("count")
        .Tooltip("How many particles the scene starts with. Rebuild the scene to apply.")
        .Range(0.0, 200000.0, 1.0)
        .Scrub(10.0)
        .Field<&ParticlesScenario::attractors>("attractors")
        .Tooltip("How many attractors the scene starts with: one in the middle, or a ring of them.")
        .Range(0.0, 16.0, 1.0)
        .Scrub(0.1)
        .Field<&ParticlesScenario::interaction>("interaction")
        .Tooltip("Whether the scene starts with particle-to-particle interaction.");
    types.Add<ParticleControls>("ParticleControls")
        .Field<&ParticleControls::interaction>("interaction")
        .Tooltip("Whether particles closer than the interaction radius push each other apart.")
        .Field<&ParticleControls::interactionRadius>("interactionRadius")
        .Label("interaction radius")
        .Tooltip("Contact distance between two particles' centers, in world units.")
        .Range(0.5, 50.0)
        .Scrub(0.05)
        .Field<&ParticleControls::stiffness>("stiffness")
        .Tooltip("Push at full overlap, in world units per second squared.")
        .Range(0.0, 20000.0)
        .Scrub(10.0)
        .Field<&ParticleControls::damping>("damping")
        .Tooltip("How much of the relative velocity contact removes per second.")
        .Range(0.0, 50.0)
        .Scrub(0.05);
}

void ParticlesProject::Build(pixie::World& world, pixie::Camera2D&) {
    scenarioEntity_ = world.Create("scenario");
    scenarioEntity_.Add<ParticlesScenario>(scenario_);
    emitter_ = SetUpParticles(world);
    dragged_ = {};
    ParticleControls& controls = emitter_.Add<ParticleControls>();
    controls.interaction = scenario_.interaction;
    // One attractor at the middle; more on a ring around it.
    for (int i = 0; i < scenario_.attractors; ++i) {
        const double angle = 2.0 * std::numbers::pi * i / scenario_.attractors;
        const double ring = scenario_.attractors == 1 ? 0.0 : kAttractorRing;
        AddAttractor(world, pxPoint(ring * std::cos(angle), ring * std::sin(angle)));
    }
    SetInteraction(emitter_, controls.interaction);
    Respawn(world, scenario_.count);
    pixie::LogInfo("particles: 1-5 for 1, 10, 100, 10000, 100000 particles; I interaction; "
                   "left click adds or drags an attractor, right click removes it; "
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

pixie::Entity ParticlesProject::AttractorAt(pixie::World& world, const pixie::Camera2D& camera,
                                            pxPoint screen) {
    pixie::Entity found;
    double best = kPickPixels;
    world.Each<const pixie::Transform2D, const pixie::Attractor>(
        [&](pixie::Entity entity, const pixie::Transform2D& transform, const pixie::Attractor&) {
            const double distance = glm::length(camera.WorldToScreen(transform.position) - screen);
            if (distance <= best) {
                best = distance;
                found = entity;
            }
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
        dragged_ = AttractorAt(world, camera, cursor);
        if (!dragged_ && AttractorCount(world) < kMaxAttractors) {
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
        if (pixie::Entity target = AttractorAt(world, camera, cursor)) {
            target.Destroy();
            pixie::LogInfo(std::format("particles: {} attractors", AttractorCount(world)));
        }
    }
}

void ParticlesProject::HandleKeys(pixie::World& world, const pixie::Input& input) {
    ParticleControls& controls = emitter_.Get<ParticleControls>();
    for (std::size_t i = 0; i < kPresets.size(); ++i) {
        if (input.WasPressed(kPresetKeys[i])) {
            if (kPresets[i] > kInteractionLimit) {
                controls.interaction = false;
                SetInteraction(emitter_, false);
            }
            Respawn(world, kPresets[i]);
        }
    }
    if (input.WasPressed(pixie::Key::I)) {
        controls.interaction = !controls.interaction;
        pixie::LogInfo(std::format("particles: interaction {}", controls.interaction ? "on" : "off"));
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

    // The controls are the source of truth; the affector follows them.
    const ParticleControls& controls = emitter_.Get<ParticleControls>();
    auto* interaction = emitter_.Get<pixie::ParticleSystem>().FindAffector<pixie::InteractionAffector>();
    if (interaction != nullptr) {
        interaction->SetEnabled(controls.interaction);
        interaction->radius = controls.interactionRadius;
        interaction->stiffness = controls.stiffness;
        interaction->damping = controls.damping;
    }
}

std::unique_ptr<pixie::Project> pixie::CreateProject() {
    return std::make_unique<ParticlesProject>();
}
