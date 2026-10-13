#pragma once

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <type_traits>

namespace pixie {

// One particle, 32 bytes, in float to keep large pools small. The GPU reads pools of these as they
// are, so this layout is also the ParticleRenderer's vertex format.
struct Particle {
    // Where it is, and where it was at the start of the current step, in world units.
    pxPointf position;
    pxPointf previous;
    // World units per second.
    pxVec2f velocity;
    // Seconds since it was emitted, and how long it lives; a lifetime of 0 means forever.
    float age = 0.0f;
    float lifetime = 0.0f;
};

static_assert(std::is_trivially_copyable_v<Particle> && sizeof(Particle) == 32);

} // namespace pixie
