#pragma once

#include "pixie/runtime/Entity.hpp"

#include <pixie/pxRect.hpp>

namespace pixie {
class World;
}

// Builds the flocks World in code: the flock entity (FlockControls, Pointer, Bounds, and the
// spatial grid of its members), the systems that orient, color, and wrap the boids after
// integration, and `count` boids spread over `area`. Returns the flock entity.
pixie::Entity SetUpFlocks(pixie::World& world, const pxRect& area, int count);

// Adds or removes boids until there are `count`. Call it outside a step.
void SetFlockSize(pixie::World& world, pixie::Entity flock, int count);

int FlockSize(pixie::World& world);

// Adds one boid at `at`, heading in a random direction within the flock's speeds. Call it outside a
// step.
void AddBoidAt(pixie::World& world, pixie::Entity flock, pxPoint at);
