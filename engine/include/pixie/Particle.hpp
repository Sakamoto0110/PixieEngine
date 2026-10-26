#pragma once

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <bit>
#include <cstdint>
#include <type_traits>

namespace pixie {

// What a particle is drawn as, its size being its radius (half the side of a square).
enum class ParticleShape : std::uint8_t {
    Disc,
    Square,
    // A hollow disc.
    Ring,
    // A disc stretched behind the particle along its velocity, over the distance it covers in
    // kStreakSeconds: sparks, rain.
    Streak,
};

// A float as an IEEE 754 half float (16 bits), rounded to nearest, ties to even; too large becomes
// infinity. Half floats keep 11 significant bits, about 0.05%, which a particle's radius needs.
constexpr std::uint16_t ToHalf(float value) {
    const auto bits = std::bit_cast<std::uint32_t>(value);
    const std::uint32_t sign = (bits >> 16) & 0x8000u;
    const std::uint32_t exponent = (bits >> 23) & 0xFFu;
    std::uint32_t mantissa = bits & 0x7FFFFFu;
    if (exponent == 0xFFu) {
        // Infinity stays infinity; a NaN stays a NaN.
        return static_cast<std::uint16_t>(sign | 0x7C00u | (mantissa != 0 ? 0x200u : 0u));
    }
    const int halfExponent = static_cast<int>(exponent) - 127 + 15;
    if (halfExponent >= 0x1F) {
        return static_cast<std::uint16_t>(sign | 0x7C00u);
    }
    if (halfExponent <= 0) {
        // A subnormal half, or zero.
        if (halfExponent < -10) {
            return static_cast<std::uint16_t>(sign);
        }
        mantissa |= 0x800000u;
        const int shift = 14 - halfExponent;
        std::uint32_t half = mantissa >> shift;
        const std::uint32_t rest = mantissa & ((1u << shift) - 1u);
        const std::uint32_t halfway = 1u << (shift - 1);
        if (rest > halfway || (rest == halfway && (half & 1u) != 0)) {
            ++half;
        }
        return static_cast<std::uint16_t>(sign | half);
    }
    std::uint32_t half = (static_cast<std::uint32_t>(halfExponent) << 10) | (mantissa >> 13);
    const std::uint32_t rest = mantissa & 0x1FFFu;
    // A carry out of the mantissa moves to the next exponent, or to infinity: both are right.
    if (rest > 0x1000u || (rest == 0x1000u && (half & 1u) != 0)) {
        ++half;
    }
    return static_cast<std::uint16_t>(sign | half);
}

// An IEEE 754 half float as a float; every half float is exact as a float.
constexpr float FromHalf(std::uint16_t half) {
    const std::uint32_t sign = static_cast<std::uint32_t>(half & 0x8000u) << 16;
    int exponent = (half >> 10) & 0x1F;
    std::uint32_t mantissa = half & 0x3FFu;
    if (exponent == 0x1F) {
        return std::bit_cast<float>(sign | 0x7F800000u | (mantissa << 13));
    }
    if (exponent == 0) {
        if (mantissa == 0) {
            return std::bit_cast<float>(sign);
        }
        // A subnormal half is a normal float: bring its leading bit up to the implicit one.
        exponent = 1;
        while ((mantissa & 0x400u) == 0) {
            mantissa <<= 1;
            --exponent;
        }
        mantissa &= 0x3FFu;
    }
    return std::bit_cast<float>(sign | (static_cast<std::uint32_t>(exponent + 112) << 23) | (mantissa << 13));
}

// One particle, 40 bytes, in float to keep large pools small. The GPU reads pools of these as they
// are, so this layout is also the ParticleRenderer's vertex format. Its look is set when it is born,
// by whoever creates it (MakeParticle), and does not change during its life.
struct Particle {
    // Where it is, and where it was at the start of the current step, in world units.
    pxPointf position;
    pxPointf previous;
    // World units per second.
    pxVec2f velocity;
    // Seconds since it was emitted, and how long it lives; a lifetime of 0 means forever.
    float age = 0.0f;
    float lifetime = 0.0f;
    pxColorRgba color{255, 255, 255};
    // Radius in world units, as a half float (ToHalf); Size() reads it.
    std::uint16_t size = ToHalf(2.0f);
    // The shape in the low two bits (kShapeBits), and kDynamic, reserved for dynamic particles.
    std::uint8_t flags = 0;
    // Unused, for the particle data design note's later fields.
    std::uint8_t spare = 0;

    static constexpr std::uint8_t kShapeBits = 0x03u;
    static constexpr std::uint8_t kDynamic = 0x04u;

    float Size() const { return FromHalf(size); }
    ParticleShape Shape() const { return static_cast<ParticleShape>(flags & kShapeBits); }
};

static_assert(std::is_trivially_copyable_v<Particle> && sizeof(Particle) == 40);

// A new particle at `position` with `velocity`, living `lifetime` seconds (0 is forever), with its
// look. Every path that creates a particle goes through here, so a particle is born complete.
inline Particle MakeParticle(pxPointf position, pxVec2f velocity, float lifetime, pxColorRgba color,
                             float size, ParticleShape shape) {
    Particle particle;
    particle.position = position;
    particle.previous = position;
    particle.velocity = velocity;
    particle.lifetime = lifetime;
    particle.color = color;
    particle.size = ToHalf(size);
    particle.flags = static_cast<std::uint8_t>(static_cast<std::uint8_t>(shape) & Particle::kShapeBits);
    return particle;
}

} // namespace pixie
