#pragma once

#include <pixie/math/pxVec.hpp>

namespace pixie {

/// Linear velocity in world units per second and angular velocity in radians per second.
/// The built-in Physics system updates Transform2D after Behaviours and before PostUpdate.
/// It excludes entities with CharacterMotor2D or RigidBody2D.
struct Velocity2D {
    pxVec2 linear;
    double angular = 0.0;
};

} // namespace pixie
