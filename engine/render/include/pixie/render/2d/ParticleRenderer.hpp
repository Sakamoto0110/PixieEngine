#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/render/View.hpp"
#include "pixie/render/gl/GlBuffer.hpp"
#include "pixie/render/gl/GlVertexArray.hpp"
#include "pixie/render/2d/Particle.hpp"
#include "pixie/render/gl/ShaderProgram.hpp"

#include <pixie/pxColorRgba.hpp>

#include <span>

namespace pixie {

// How long a Streak's tail is, in seconds of the particle's motion.
inline constexpr float kStreakSeconds = 0.05f;

// How a pool of particles is drawn. `color`, `size` (radius in world units) and `shape` are the pool's
// default look, which a particle takes when it is born unless its creator gives it its own; each
// particle carries its look, so changing these does not repaint the particles already alive. The rest
// applies to the whole pool: no particle is drawn smaller than minPixels on screen, and the blending
// and fading.
struct ParticleStyle {
    pxColorRgba color{255, 255, 255};
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
    PIXIE_RENDER_API bool Create();
    PIXIE_RENDER_API void Destroy();

    // `alpha` interpolates from Particle::previous to Particle::position, as FrameTime::alpha does.
    PIXIE_RENDER_API void Draw(std::span<const Particle> particles, const ParticleStyle& style, double alpha,
              const View& view);

private:
    ShaderProgram program_;
    GlVertexArray vertexArray_;
    GlBuffer particles_;
};

} // namespace pixie
