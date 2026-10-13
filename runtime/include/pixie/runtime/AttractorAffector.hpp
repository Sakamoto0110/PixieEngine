#pragma once

#include "pixie/runtime/ParticleAffector.hpp"

#include <vector>

namespace pixie {

// Pulls every particle of the pool toward every entity with a Transform2D and an Attractor:
// acceleration = strength * offset / (distance^2 + softening^2)^(3/2). An attractor with
// killOnContact removes the particles that come within its body.
class AttractorAffector final : public ParticleAffector {
public:
    void Apply(World& world, Entity emitter, std::span<Particle> particles,
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

} // namespace pixie
