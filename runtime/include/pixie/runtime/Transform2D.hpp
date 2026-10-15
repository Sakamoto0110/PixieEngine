#pragma once

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <cmath>
#include <numbers>

namespace pixie {

// Where an entity is, in world units, Y down. Rotation is in radians, from +X toward +Y.
struct Transform2D {
    pxPoint position;
    double rotation = 0.0;
    pxVec2 scale{1.0, 1.0};
};

// The Transform2D as it was at the start of the current step, kept by the World for every entity
// with a Transform2D, so rendering can interpolate between two steps. Set it equal to the
// Transform2D (World::ResetInterpolation) after a jump that should not be drawn as motion.
struct PreviousTransform2D {
    Transform2D value;
};

// The transform `alpha` of the way from `from` to `to`, as rendering draws it between two steps; the
// rotation turns the short way round, so a turn across ±pi does not spin backwards.
inline Transform2D Interpolate(const Transform2D& from, const Transform2D& to, double alpha) {
    const double turn = std::remainder(to.rotation - from.rotation, 2.0 * std::numbers::pi);
    return {pxPoint::Lerp(from.position, to.position, alpha), from.rotation + turn * alpha,
            from.scale + (to.scale - from.scale) * alpha};
}

} // namespace pixie
