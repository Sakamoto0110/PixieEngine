#include "ParticlesScene.hpp"

#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/AttractorAffector.hpp"
#include "pixie/runtime/InteractionAffector.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/ParticleStep.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <vector>

namespace {

constexpr pxColorRgba kAttractorColor{255, 214, 120};
constexpr pxColorRgba kSpawnerColor{140, 200, 255};
constexpr pxColorRgba kParticleColor{120, 175, 255, 200};
constexpr double kInnerOrbit = 40.0;
constexpr double kOuterOrbit = 260.0;

} // namespace

void SetUpParticles(pixie::World& world, pixie::Entity system, bool defaults) {
    pixie::AddParticleStep(world);
    if (!system.Has<pixie::Transform2D>()) {
        system.Add<pixie::Transform2D>();
    }
    pixie::ParticleSystem& pool = system.Get<pixie::ParticleSystem>();
    // Not described, so not in the scene: far particles stay visible.
    pool.style.minPixels = 1.2f;
    if (defaults) {
        // What the scene holds: the look, the pull of the attractors, and the interaction.
        pool.capacity = 200000;
        pool.style.color = kParticleColor;
        pool.style.size = 2.0f;
        pool.style.additive = true;
        system.Add<pixie::ParticleAttraction>();
        system.Add<pixie::ParticleInteraction>();
    }

    if (defaults) {
        // A spawner on the left: particles leave a small ring outward and fall toward the attractors.
        AddSpawner(world, system, pxPoint(-420.0, 0.0)).Get<pixie::ParticleSpawner>().rate = 150.0;
    }
}

pixie::Entity AddSpawner(pixie::World& world, pixie::Entity system, pxPoint position) {
    pixie::Entity spawner = world.Create("spawner");
    spawner.Add<pixie::Transform2D>(pixie::Transform2D{position});
    pixie::ParticleSpawner& feed = spawner.Add<pixie::ParticleSpawner>();
    feed.system = system.Id();
    feed.rate = 0.0;
    feed.lifetime = 8.0f;
    feed.speedMin = 60.0;
    feed.speedMax = 110.0;
    feed.shape = pixie::EmissionShape::Ring;
    feed.radius = 16.0;
    feed.directionMode = pixie::DirectionMode::ShapeNormal;
    feed.spread = 0.6;
    spawner.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Ring, pxVec2(16.0, 16.0), kSpawnerColor});
    pixie::AddToScene(spawner);
    return spawner;
}

int SpawnerCount(pixie::World& world) {
    return static_cast<int>(world.Registry().storage<pixie::ParticleSpawner>().size());
}

pixie::Entity AddAttractor(pixie::World& world, pxPoint position) {
    pixie::Entity attractor = world.Create("attractor");
    attractor.Add<pixie::Transform2D>(pixie::Transform2D{position});
    attractor.Add<pixie::Attractor>();
    attractor.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Ring, pxVec2(10.0, 10.0), kAttractorColor});
    pixie::AddToScene(attractor);
    return attractor;
}

int AttractorCount(pixie::World& world) {
    return static_cast<int>(world.Registry().storage<pixie::Attractor>().size());
}

void SpawnOrbits(pixie::World& world, pixie::Entity emitter, int count) {
    struct Center {
        pxPoint position;
        double strength;
    };
    std::vector<Center> centers;
    for (auto [id, transform, attractor] :
         world.Registry().view<const pixie::Transform2D, const pixie::Attractor>().each()) {
        centers.push_back({transform.position, attractor.strength});
    }
    if (centers.empty()) {
        centers.push_back({pxPoint(0.0, 0.0), pixie::Attractor{}.strength});
    }

    pixie::ParticleSystem& pool = emitter.Get<pixie::ParticleSystem>();
    pool.particles.clear();
    pixie::Random& random = world.GetRandom();
    for (int i = 0; i < count; ++i) {
        const Center& center = centers[static_cast<std::size_t>(i) % centers.size()];
        // Uniform over the annulus' area, so the inner orbits are not crowded.
        const double inner = kInnerOrbit * kInnerOrbit;
        const double outer = kOuterOrbit * kOuterOrbit;
        const double radius = std::sqrt(random.Range(inner, outer));
        const pxVec2 out = random.Direction();
        const pxVec2 along(-out.y, out.x); // counterclockwise on screen
        // A little off the circular speed, so orbits are slightly elliptical and the rings shimmer.
        const double speed = std::sqrt(std::abs(center.strength) / radius) * random.Range(0.92, 1.08);
        const pxPoint at = center.position + out * radius;
        pool.Emit(pxPointf(static_cast<float>(at.x), static_cast<float>(at.y)),
                  pxVec2f(along * speed), 0.0f);
    }
}

void SetInteraction(pixie::Entity emitter, bool on) {
    if (auto* interaction = emitter.TryGet<pixie::ParticleInteraction>()) {
        interaction->enabled = on;
    } else if (on) {
        emitter.Add<pixie::ParticleInteraction>();
    }
}

bool InteractionOn(pixie::Entity emitter) {
    const auto* interaction = emitter.TryGet<pixie::ParticleInteraction>();
    return interaction != nullptr && interaction->enabled;
}
