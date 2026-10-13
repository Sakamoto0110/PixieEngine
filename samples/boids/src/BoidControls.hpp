#pragma once

#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

// The components of the "controls" entity, which every Boid reads. The App writes the pointer and
// the bounds before each step; the values below are what a user (or later the editor) tunes.

enum class SteerMode { Attract, Repel };

struct BoidControls {
    SteerMode mode = SteerMode::Attract;
    // How far from the pointer a boid feels it, in world units.
    double radius = 220.0;
    // Speed limits and the largest change of velocity per second, in world units.
    double maxSpeed = 260.0;
    double cruiseSpeed = 110.0;
    double maxForce = 520.0;
    // Random turning when the pointer is out of reach, in radians per second at most.
    double wander = 2.5;
};

// The pointer in world units, and whether it is over the view.
struct Pointer {
    pxPoint position;
    bool active = false;
};

// The part of the world the boids live in; they wrap from one edge to the opposite one.
struct Bounds {
    pxRect area;
};
