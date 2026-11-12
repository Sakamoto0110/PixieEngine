#pragma once

#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>

#include <cmath>
#include <numbers>

namespace pixie {

/// Evaluated world pose in world units with Y down. Rotation is radians from +X toward +Y and
/// scale is dimensionless. Edit an attached entity through the Parenting2D.hpp pose setters.
struct Transform2D {
    pxPoint position;
    double rotation = 0.0;
    pxVec2 scale{1.0, 1.0};
};

/// World-owned start-of-step pose for rendering interpolation. Use World::ResetInterpolation
/// after a teleport to reset the sample, including the attached branch's local history.
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
