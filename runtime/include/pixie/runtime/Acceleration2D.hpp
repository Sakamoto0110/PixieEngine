#pragma once

#include <pixie/math/pxVec.hpp>

namespace pixie {

/// Persistent linear acceleration in world units per second squared. A steering Behaviour can
/// write it each tick. The built-in Physics system applies it to Velocity2D before integrating
/// the transform. CharacterMotor2D and RigidBody2D entities skip this legacy pass.
struct Acceleration2D {
    pxVec2 linear;
};

} // namespace pixie
