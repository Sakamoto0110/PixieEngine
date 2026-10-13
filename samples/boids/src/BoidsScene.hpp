#pragma once

#include "pixie/runtime/Entity.hpp"

#include <pixie/pxRect.hpp>

namespace pixie {
class World;
}

// Builds the boids World in code: the controls entity (BoidControls, Pointer, Bounds), the system
// that wraps boids at the bounds, and `count` boids spread over `area`. Returns the controls entity.
pixie::Entity SetUpBoids(pixie::World& world, const pxRect& area, int count);

// Adds or removes boids until there are `count`. Call it outside a step.
void SetBoidCount(pixie::World& world, pixie::Entity controls, int count);

int BoidCount(pixie::World& world);
