#pragma once

#include "pixie/Particle.hpp"
#include "pixie/ParticleRenderer.hpp"
#include "pixie/runtime/ParticleAffector.hpp"

#include <cstddef>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

namespace pixie {

// A component that owns a pool of particles. Particles are not entities: they live in one
// contiguous vector here, and the particle system (AddParticleSystem) updates the whole pool each
// step. The entity's Transform2D, if any, is where rate emission happens, and its rotation the
// direction of the spread.
struct ParticleEmitter {
    // Move-only, since it owns its affectors; said explicitly, because a vector of unique_ptr
    // claims to be copyable and EnTT would try.
    ParticleEmitter() = default;
    ParticleEmitter(const ParticleEmitter&) = delete;
    ParticleEmitter& operator=(const ParticleEmitter&) = delete;
    ParticleEmitter(ParticleEmitter&&) = default;
    ParticleEmitter& operator=(ParticleEmitter&&) = default;
    ~ParticleEmitter() = default;

    // Particles emitted per second at the entity's position; 0 emits nothing by itself.
    double rate = 0.0;
    // Lifetime of emitted particles, in seconds; 0 means forever.
    float lifetime = 0.0f;
    // The pool never grows beyond this; emission stops while it is full.
    std::size_t capacity = 10000;
    // Emission speed range, and the angle of the cone around the entity's rotation, in radians.
    double speedMin = 0.0;
    double speedMax = 0.0;
    double spread = 2.0 * std::numbers::pi;
    ParticleStyle style;

    std::vector<Particle> particles;
    std::vector<std::unique_ptr<ParticleAffector>> affectors;
    // Fraction of a particle carried to the next step by rate emission.
    double pending = 0.0;

    // Adds one particle now; returns false, adding nothing, when the pool is full.
    bool Emit(pxPointf position, pxVec2f velocity, float particleLifetime) {
        if (particles.size() >= capacity) {
            return false;
        }
        Particle particle;
        particle.position = position;
        particle.previous = position;
        particle.velocity = velocity;
        particle.lifetime = particleLifetime;
        particles.push_back(particle);
        return true;
    }

    template<typename A, typename... Args>
    A& AddAffector(Args&&... args) {
        auto affector = std::make_unique<A>(std::forward<Args>(args)...);
        A& added = *affector;
        affectors.push_back(std::move(affector));
        return added;
    }

    // The first affector of type A, or null.
    template<typename A>
    A* FindAffector() const {
        for (const auto& affector : affectors) {
            if (auto* found = dynamic_cast<A*>(affector.get())) {
                return found;
            }
        }
        return nullptr;
    }
};

} // namespace pixie
