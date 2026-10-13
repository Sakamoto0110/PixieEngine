#pragma once

#include <pixie/math/pxVec.hpp>
#include <pixie/pxColorRgba.hpp>
#include <pixie/pxPoint.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pixie {

// The shapes the ShapeRenderer draws. Each spans [-1, 1] on both axes before scaling.
enum class ShapeKind : std::uint8_t {
    // Points along +X: the tip at (1, 0), the base at x = -0.7.
    Triangle,
    // A filled disc of radius 1, antialiased.
    Circle,
    // A circle outline of radius 1, antialiased, two pixels wide on screen.
    Ring,
    // A filled square.
    Quad,
};

inline constexpr int kShapeKindCount = 4;

// One shape as the GPU reads it, 24 bytes.
struct ShapeInstance {
    // Center, in world units.
    pxPointf position;
    // Radians, from +X toward +Y.
    float rotation = 0.0f;
    // Half extents, in world units: the shape spans [-scale, scale] before rotating.
    pxVec2f scale{1.0f, 1.0f};
    pxColorRgba color{255, 255, 255};
};

static_assert(std::is_trivially_copyable_v<ShapeInstance> && sizeof(ShapeInstance) == 24);

} // namespace pixie
