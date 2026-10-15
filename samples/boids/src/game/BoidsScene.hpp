#pragma once

#include "pixie/runtime/Entity.hpp"

#include <pixie/pxRect.hpp>

namespace pixie {
class World;
}

// Builds what the scene does not hold: the controls' Pointer and Bounds (over `area`) when they are
// missing, the system that wraps boids at the bounds, and `count` boids spread over `area`.
void SetUpBoids(pixie::World& world, pixie::Entity controls, const pxRect& area, int count);

// Adds or removes boids until there are `count`. Call it outside a step.
void SetBoidCount(pixie::World& world, pixie::Entity controls, int count);

int BoidCount(pixie::World& world);

// Adds one boid at `at`, heading in a random direction at the cruise speed. Call it outside a step.
void AddBoidAt(pixie::World& world, pixie::Entity controls, pxPoint at);
