#pragma once

#include "pixie/runtime/ParticleAffector.hpp"

#include <vector>

namespace pixie {

// Pulls every particle of the pool toward every entity with a Transform2D and an Attractor:
// acceleration = strength * offset / (distance^2 + softening^2)^(3/2). An attractor with
// killOnContact removes the particles that come within its body.
class AttractorAffector final : public ParticleAffector {
public:
    // Multiplies every attractor's strength for this pool: 0 pulls nothing, negative pushes away.
    // Killing on contact does not depend on it.
    double scale = 1.0;

    void Apply(World& world, Entity system, std::span<Particle> particles,
               const FrameTime& time) override;

private:
    struct Source {
        float x;
        float y;
        float strength;
        float softeningSquared;
        float reachSquared;
        // Squared body radius when the attractor kills on contact, else -1.
        float killSquared;
    };
    std::vector<Source> sources_;
};

// The attraction of a particle system, as a scene component on the system's entity: while
// enabled, the particle step pulls the system's particles toward every attractor, as an
// AttractorAffector with this scale would, after the affectors added in code.
struct ParticleAttraction {
    bool enabled = true;
    double scale = 1.0;
};

} // namespace pixie
