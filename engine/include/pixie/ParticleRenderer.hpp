#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/GlBuffer.hpp"
#include "pixie/GlVertexArray.hpp"
#include "pixie/Particle.hpp"
#include "pixie/ShaderProgram.hpp"

#include <pixie/pxColorRgba.hpp>

#include <span>

namespace pixie {

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
};

// Draws a pool of particles as instanced antialiased discs, straight from the pool's memory: the
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
