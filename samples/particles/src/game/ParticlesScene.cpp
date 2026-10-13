#include "ParticlesScene.hpp"

#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/AttractorAffector.hpp"
#include "pixie/runtime/InteractionAffector.hpp"
#include "pixie/runtime/ParticleEmitter.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <vector>

namespace {

constexpr pxColorRgba kAttractorColor{255, 214, 120};
constexpr pxColorRgba kParticleColor{120, 175, 255, 200};
constexpr double kInnerOrbit = 40.0;
constexpr double kOuterOrbit = 260.0;

} // namespace

pixie::Entity SetUpParticles(pixie::World& world) {
    pixie::AddParticleSystem(world);
    pixie::Entity emitter = world.Create("particles");
    emitter.Add<pixie::Transform2D>();
    pixie::ParticleEmitter& pool = emitter.Add<pixie::ParticleEmitter>();
    pool.capacity = 200000;
    pool.style.color = kParticleColor;
    pool.style.size = 2.0f;
    pool.style.minPixels = 1.2f;
    pool.style.additive = true;
    pool.AddAffector<pixie::AttractorAffector>();
    pool.AddAffector<pixie::InteractionAffector>();
    return emitter;
}

pixie::Entity AddAttractor(pixie::World& world, pxPoint position) {
    pixie::Entity attractor = world.Create("attractor");
    attractor.Add<pixie::Transform2D>(pixie::Transform2D{position});
    attractor.Add<pixie::Attractor>();
    attractor.Add<pixie::Shape>(pixie::Shape{pixie::ShapeKind::Ring, pxVec2(10.0, 10.0), kAttractorColor});
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

    pixie::ParticleEmitter& pool = emitter.Get<pixie::ParticleEmitter>();
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
    if (auto* interaction = emitter.Get<pixie::ParticleEmitter>().FindAffector<pixie::InteractionAffector>()) {
        interaction->SetEnabled(on);
    }
}

bool InteractionOn(pixie::Entity emitter) {
    const auto* interaction = emitter.Get<pixie::ParticleEmitter>().FindAffector<pixie::InteractionAffector>();
    return interaction != nullptr && interaction->IsEnabled();
}
