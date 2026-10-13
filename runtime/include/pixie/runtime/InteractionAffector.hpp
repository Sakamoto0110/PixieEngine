#pragma once

#include "pixie/runtime/ParticleAffector.hpp"
#include "pixie/runtime/SpatialGrid.hpp"

namespace pixie {

// A short-range interaction between the particles of one pool: two particles closer than `radius`
// push each other apart, harder the more they overlap, like soft discs. Neighbours come from a
// spatial grid built over the pool at each step, so the cost grows with the particles and their
// neighbours, not with the square of the pool.
class InteractionAffector final : public ParticleAffector {
public:
    // Contact distance between two particles' centers, in world units.
    double radius = 4.0;
    // Acceleration at full overlap, in world units per second squared.
    double stiffness = 1200.0;
    // Fraction of the relative velocity removed per second while in contact, so piles settle.
    double damping = 3.0;

    void Apply(World& world, Entity system, std::span<Particle> particles,
               const FrameTime& time) override;

private:
    SpatialGrid grid_;
    std::vector<pxVec2f> change_;
};

} // namespace pixie
