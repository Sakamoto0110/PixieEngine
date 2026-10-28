#pragma once

#include "pixie/render/2d/Particle.hpp"
#include "pixie/render/2d/ParticleRenderer.hpp"
#include "pixie/runtime/ParticleAffector.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace pixie {

// A component that owns a pool of particles, its look, and its affectors. Particles are not
// entities: they live in one contiguous vector here, and the particle step (AddParticleStep) updates
// the whole pool each step. Particles come from ParticleSpawner entities that point at this one
// (several may feed it), or straight from Emit.
struct ParticleSystem {
    // Move-only, since it owns its affectors; said explicitly, because a vector of unique_ptr
    // claims to be copyable and EnTT would try.
    ParticleSystem() = default;
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    ParticleSystem(ParticleSystem&&) = default;
    ParticleSystem& operator=(ParticleSystem&&) = default;
    ~ParticleSystem() = default;

    // The pool never grows beyond this; emission stops while it is full.
    int capacity = 10000;
    ParticleStyle style;

    std::vector<Particle> particles;
    std::vector<std::unique_ptr<ParticleAffector>> affectors;

    // Adds one particle now, with the system's look (its style's color, size and shape); returns
    // false, adding nothing, when the pool is full.
    bool Emit(pxPointf position, pxVec2f velocity, float particleLifetime) {
        return Emit(position, velocity, particleLifetime, style.color, style.size, style.shape);
    }

    bool EmitDynamic(pxPointf position, pxVec2f velocity, float particleLifetime) {
        return Emit(position, velocity, particleLifetime, style.color, style.size, style.shape, true);
    }

    // The same, with a look of its own instead of the system's.
    bool Emit(pxPointf position, pxVec2f velocity, float particleLifetime, pxColorRgba color, float size,
              ParticleShape shape, bool dynamic = false) {
        if (particles.size() >= static_cast<std::size_t>(capacity)) {
            return false;
        }
        particles.push_back(MakeParticle(position, velocity, particleLifetime, color, size, shape, dynamic));
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
