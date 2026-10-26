#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/GlBuffer.hpp"
#include "pixie/GlVertexArray.hpp"
#include "pixie/Particle.hpp"
#include "pixie/ShaderProgram.hpp"

#include <pixie/pxColorRgba.hpp>

#include <cstdint>
#include <span>

namespace pixie {

// What each particle of a pool is drawn as, `size` being its radius (half the side of a square).
enum class ParticleShape : std::uint8_t {
    Disc,
    Square,
    // A hollow disc.
    Ring,
    // A disc stretched behind the particle along its velocity, over the distance it covers in
    // kStreakSeconds: sparks, rain.
    Streak,
};

// How long a Streak's tail is, in seconds of the particle's motion.
inline constexpr float kStreakSeconds = 0.05f;

// How a pool of particles looks: all of a pool share one style.
struct ParticleStyle {
    pxColorRgba color{255, 255, 255};
    // Radius in world units; a particle is never drawn smaller than minPixels on screen.
    float size = 2.0f;
    float minPixels = 1.0f;
    // Additive blending, so overlapping particles glow; otherwise alpha blending.
    bool additive = true;
    // Fade out over the particle's lifetime (particles that live forever do not fade).
    bool fade = true;
    ParticleShape shape = ParticleShape::Disc;
};

// Draws a pool of particles as instanced antialiased shapes, straight from the pool's memory: the
// GPU interpolates each particle between its previous and current position, so the CPU does no
// per-particle work to draw.
class ParticleRenderer {
public:
    // Needs a current GL context; returns false if the shaders do not build.
    bool Create();
    void Destroy();

    // `alpha` interpolates from Particle::previous to Particle::position, as FrameTime::alpha does.
    void Draw(std::span<const Particle> particles, const ParticleStyle& style, double alpha,
              const Camera2D& camera);

private:
    ShaderProgram program_;
    GlVertexArray vertexArray_;
    GlBuffer particles_;
};

} // namespace pixie
