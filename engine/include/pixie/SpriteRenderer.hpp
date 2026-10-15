#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/GlBuffer.hpp"
#include "pixie/GlVertexArray.hpp"
#include "pixie/ShaderProgram.hpp"

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <array>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace pixie {

// How a sprite's texture is sampled.
enum class SpriteFilter : std::uint8_t {
    // Linear, between mipmaps: smooth at any size, and no aliasing when drawn small.
    Smooth,
    // Nearest texel: crisp pixel art.
    Pixel,
};

// One sprite as the GPU reads it, 48 bytes: a rectangle of a texture, placed in the world.
struct SpriteInstance {
    // Where the pivot lies, in world units.
    pxPointf position;
    // Radians, from +X toward +Y, around the pivot.
    float rotation = 0.0f;
    // Full width and height in world units; a negative one mirrors the sprite on that axis.
    pxVec2f size{1.0f, 1.0f};
    // The pivot within the rectangle, from (0, 0) at the top-left to (1, 1) at the bottom-right.
    pxVec2f pivot{0.5f, 0.5f};
    // Texture coordinates of the top-left and bottom-right corners: u0, v0, u1, v1, with v = 0 at
    // the image's top.
    std::array<float, 4> uv{0.0f, 0.0f, 1.0f, 1.0f};
    // Multiplies the texture, straight alpha.
    pxColorRgba color{255, 255, 255};
};

static_assert(std::is_trivially_copyable_v<SpriteInstance> && sizeof(SpriteInstance) == 48);

// Draws textured rectangles as instances. Sprites are submitted during a frame and flushed
// together: by layer, lower first, and in submission order within a layer; consecutive sprites
// with the same texture and filter share one draw call. Textures hold premultiplied alpha (see
// DecodePng).
class SpriteRenderer {
public:
    // Needs a current GL context; returns false if the shaders do not build.
    bool Create();
    void Destroy();

    void Submit(unsigned texture, SpriteFilter filter, int layer, const SpriteInstance& instance);
    // Draws what was submitted since the last flush, seen through `camera`, then forgets it.
    void Flush(const Camera2D& camera);

    // Draw calls made by the last Flush.
    int LastDrawCalls() const { return lastDrawCalls_; }

private:
    struct Pending {
        int layer;
        unsigned texture;
        SpriteFilter filter;
        SpriteInstance instance;
    };

    ShaderProgram program_;
    GlVertexArray vertexArray_;
    GlBuffer instances_;
    std::array<unsigned, 2> samplers_{};
    std::vector<Pending> pending_;
    std::vector<SpriteInstance> ordered_;
    int lastDrawCalls_ = 0;
};

} // namespace pixie
