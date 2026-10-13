#pragma once

#include <pixie/pxPoint.hpp>
#include <pixie/pxRect.hpp>

// The components of the "flock" entity, which every Flocker reads. The App writes the pointer and
// the bounds before each step; the rest is what a user (or later the editor) tunes.

enum class PointerMode { Attract, Repel };

struct FlockControls {
    // Neighbours are the boids within this distance.
    double neighbourRadius = 46.0;
    // Neighbours closer than this push the boid away.
    double separationRadius = 18.0;
    double separationWeight = 1.8;
    double alignmentWeight = 1.0;
    double cohesionWeight = 0.9;
    double pointerWeight = 1.4;
    double pointerRadius = 170.0;
    PointerMode pointerMode = PointerMode::Attract;
    double minSpeed = 70.0;
    double maxSpeed = 180.0;
    // The largest change of velocity per second for each steering rule.
    double maxForce = 320.0;
};

// The pointer in world units, and whether it is over the view.
struct Pointer {
    pxPoint position;
    bool active = false;
};

// The part of the world the flock lives in; boids wrap from one edge to the opposite one.
struct Bounds {
    pxRect area;
};

// Marks the entities the flock's spatial grid indexes.
struct FlockMember {};
