#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/core/FramebufferSize.hpp"
#include "pixie/render/View.hpp"
#include "pixie/render/gl/GlBuffer.hpp"
#include "pixie/render/gl/ShaderProgram.hpp"

#include <pixie/pxColorRgba.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace pixie {

enum class LightKind : std::uint32_t { Point, Spot, Directional };

// One light as the GPU reads it, 64 bytes in four vec4-sized rows. Colors are linear, already
// multiplied by the light's intensity.
struct LightInstance {
    float x = 0.0f;
    float y = 0.0f;
    // World units; the light reaches exactly 0 there. Unused by directional lights.
    float range = 1.0f;
    // The exponent p of saturate(1 - (d / range)^2)^p.
    float falloff = 2.0f;
    std::array<float, 3> color{1.0f, 1.0f, 1.0f};
    LightKind kind = LightKind::Point;
    // Unit vector the light points along (spot), or travels along (directional).
    float directionX = 1.0f;
    float directionY = 0.0f;
    // Cosines of the cone's half-angle and of the half-angle where it starts to fade (spot).
    float cosOuter = -1.0f;
    float cosInner = -1.0f;
    // Receiver layers this light reaches.
    std::uint32_t layers = 0xFFu;
    // Occluder layers that shadow it; 0 casts no shadow.
    std::uint32_t shadowLayers = 0u;
    // The source's size, which sets the penumbra: a radius in world units (point, spot) or an
    // angular radius in radians (directional). 0 gives hard shadows.
    float source = 0.0f;
    std::uint32_t padding = 0u;
};

static_assert(std::is_trivially_copyable_v<LightInstance> && sizeof(LightInstance) == 64);

// One edge of a convex occluder, 32 bytes, in world units. Polygons wind with a positive signed
// area, so the edge's outward normal is (b - a) turned to (dy, -dx).
struct OccluderEdge {
    float ax = 0.0f;
    float ay = 0.0f;
    float bx = 0.0f;
    float by = 0.0f;
    // The occluder's layers, matched against a light's shadowLayers.
    std::uint32_t layers = 1u;
    // Bit 0: the occluder shadows its own interior (it casts from its light-facing edges).
    std::uint32_t flags = 0u;
    std::array<std::uint32_t, 2> padding{};
};

static_assert(std::is_trivially_copyable_v<OccluderEdge> && sizeof(OccluderEdge) == 32);

enum class ToneMapping : std::uint8_t {
    // Linear values are clamped to [0, 1].
    None,
    // Narkowicz's fit of the ACES filmic curve.
    Aces,
};

// What a lit frame combines with: the ambient light (linear, already times its intensity), exposure
// and tone mapping.
struct LightingParameters {
    std::array<float, 3> ambient{1.0f, 1.0f, 1.0f};
    float exposure = 1.0f;
    ToneMapping toneMapping = ToneMapping::None;
};

// The sRGB transfer function, exactly: an 8-bit-style channel in [0, 1] to linear and back.
PIXIE_RENDER_API float DecodeSrgb(float channel);
PIXIE_RENDER_API float EncodeSrgb(float linear);

// Lights a 2D frame: the world is drawn as usual into an albedo target (and, for receivers that
// differ from the default, a receiver target), then lights accumulate in linear space, and the
// combine writes albedo times light, exposed, tone-mapped and encoded, into the framebuffer that was
// bound when the frame began. See docs/lighting-l0-design.md for the order.
//
// The receiver target holds, per pixel, the light layers of the topmost receiver (red, 0 to 255)
// and whether it is unlit (green, 0 or 1); it starts as every layer, lit.
//
// Owns its GL objects: Destroy it while the context is still current.
class LightRenderer {
public:
    // Needs a current GL context; returns false if the shaders do not build.
    PIXIE_RENDER_API bool Create();
    PIXIE_RENDER_API void Destroy();

    // Remembers the framebuffer and viewport bound now, then draws into the albedo target, sized to
    // `size` and cleared to `background` (opaque).
    PIXIE_RENDER_API void BeginWorld(FramebufferSize size, pxColorRgba background);
    // Draws into the receiver target from now on. Its encoding is SpriteRenderer's and
    // ShapeRenderer's receiver pass's (LightingPass::Receivers).
    PIXIE_RENDER_API void BeginReceivers();
    // Draws into the emission target from now on: linear radiance, premultiplied by coverage, added
    // unlit by the combine (LightingPass::Emission).
    PIXIE_RENDER_API void BeginEmission();
    // Accumulates ambient and `lights`, seen through `view`, each shadowed by the `occluders` edges
    // of its shadow layers (shadow volumes with analytic penumbrae, docs/lighting-l0-design.md),
    // then combines into the framebuffer and viewport remembered by BeginWorld, which stay bound.
    PIXIE_RENDER_API void Finish(const View& view, const LightingParameters& parameters,
                                 std::span<const LightInstance> lights,
                                 std::span<const OccluderEdge> occluders = {});

    // The lights drawn by the last Finish, and how many of them cast shadows.
    int LastLightCount() const { return lastLightCount_; }
    int LastShadowedCount() const { return lastShadowedCount_; }

private:
    struct Target {
        GLuint texture = 0;
        GLenum format = 0;
    };
    bool Resize(FramebufferSize size);
    void DestroyTargets();

    ShaderProgram lightProgram_;
    ShaderProgram shadowProgram_;
    ShaderProgram combineProgram_;
    GlBuffer lightBuffer_;
    GlBuffer edgeBuffer_;
    GLuint vertexArray_ = 0;
    GLuint worldFramebuffer_ = 0;
    GLuint lightFramebuffer_ = 0;
    GLuint maskFramebuffer_ = 0;
    Target albedo_;
    Target receivers_;
    Target emission_;
    Target light_;
    Target mask_;
    std::vector<LightInstance> ordered_;
    FramebufferSize size_;
    GLint outputFramebuffer_ = 0;
    std::array<GLint, 4> outputViewport_{};
    int lastLightCount_ = 0;
    int lastShadowedCount_ = 0;
};

} // namespace pixie
