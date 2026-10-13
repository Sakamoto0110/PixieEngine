#pragma once

#include <pixie/math/pxVec.hpp>

namespace pixie {

// How fast an entity's Velocity2D changes, in world units per second squared. A Behaviour that
// steers writes this, its own state, and leaves Velocity2D to the World: in PostUpdate the World
// adds it to the velocity, then moves the transform by the new velocity. It persists until changed.
struct Acceleration2D {
    pxVec2 linear;
};

} // namespace pixie
