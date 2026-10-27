#pragma once

#include "pixie/Particle.hpp"
#include "pixie/runtime/CollisionWorld2D.hpp"

namespace pixie {
class World;
struct ParticleSystem;

// Shared settings for the dynamic subset of a pool. All distances use world units.
struct ParticlePhysics2D {
    double gravityScale = 1;
    double collisionRadius = 1;
    double restitution = 0.6;
    double friction = 0.2;
    double bounceThreshold = 35;
    int category = 1;
    int mask = 0x7fffffff;
};
struct ParticlePhysicsStats2D {
    std::size_t dynamic = 0, queries = 0, contacts = 0, expired = 0, exhausted = 0;
};

// Integration after the caller ages particles and snapshots previous (as AddParticleStep does).
// Never steps the backend or creates particle bodies. Newborn age supplies the partial tick.
void MoveDynamicParticles2D(ParticleSystem& system, const ParticlePhysics2D& settings,
                            const CollisionWorld2D* collision, pxVec2 gravity, double delta,
                            ParticlePhysicsStats2D& stats);
}
