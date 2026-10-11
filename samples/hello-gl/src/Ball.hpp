#pragma once

#include "Color.hpp"
#include "Vec2.hpp"

// Positions are in framebuffer pixels, with y pointing down.
struct Ball {
    Vec2 position;
    // Position before the last step; Render interpolates from here to position.
    Vec2 previousPosition;
    // Pixels per second.
    Vec2 velocity;
    float radius = 16.0f;
    Color color;
};

// Moves the ball by one step and reflects it off the edges of a box from (0, 0) to bounds.
void Step(Ball& ball, float seconds, Vec2 bounds);
