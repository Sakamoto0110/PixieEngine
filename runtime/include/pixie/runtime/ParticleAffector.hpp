#pragma once

#include "pixie/FrameTime.hpp"
#include "pixie/Particle.hpp"
#include "pixie/runtime/Entity.hpp"

#include <algorithm>
#include <span>

namespace pixie {

class World;

// Marks a particle for removal: the particle system removes it right after the affectors run, before
// the particles move, so it is not drawn again.
inline void KillParticle(Particle& particle) {
    particle.lifetime = std::max(particle.age, 1e-6f);
    particle.age = particle.lifetime;
}

// User logic over a whole particle pool at once: the particles' counterpart of a Behaviour, a class
// in the EC style that receives the pool instead of one particle, so the work stays a tight loop.
// Affectors belong to a ParticleSystem and run in the order added, once per step, after the pool
// has aged and the spawners have emitted and before the particles move. They should change velocities (or the
// particles' own fields) and read positions, which no affector changes, so their result does not
// depend on particle order.
class ParticleAffector {
public:
    virtual ~ParticleAffector() = default;

    virtual void Apply(World& world, Entity system, std::span<Particle> particles,
                       const FrameTime& time) = 0;

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    bool enabled_ = true;
};

} // namespace pixie
