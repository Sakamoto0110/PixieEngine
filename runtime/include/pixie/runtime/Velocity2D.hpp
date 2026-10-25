#pragma once

#include <pixie/math/pxVec.hpp>

namespace pixie {

// How fast an entity moves, in world units and radians per second. The World integrates it into
// the Transform2D in PostUpdate, after every Behaviour has run. CharacterMotor2D and RigidBody2D
// entities are excluded: their physics owner integrates motion without a second legacy integrator.
struct Velocity2D {
    pxVec2 linear;
    double angular = 0.0;
};

} // namespace pixie
