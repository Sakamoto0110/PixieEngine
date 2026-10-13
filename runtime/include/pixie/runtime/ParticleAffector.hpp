#pragma once

#include "pixie/FrameTime.hpp"
#include "pixie/Particle.hpp"
#include "pixie/runtime/Entity.hpp"

#include <span>

namespace pixie {

class World;

// User logic over a whole particle pool at once: the particles' counterpart of a Behaviour, a class
// in the EC style that receives the pool instead of one particle, so the work stays a tight loop.
// Affectors belong to a ParticleEmitter and run in the order added, once per step, after the pool
// has aged and emitted and before the particles move. They should change velocities (or the
// particles' own fields) and read positions, which no affector changes, so their result does not
// depend on particle order.
class ParticleAffector {
public:
    virtual ~ParticleAffector() = default;

    virtual void Apply(World& world, Entity emitter, std::span<Particle> particles,
                       const FrameTime& time) = 0;

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    bool enabled_ = true;
};

} // namespace pixie
