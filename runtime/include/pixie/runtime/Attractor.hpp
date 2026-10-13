#pragma once

namespace pixie {

// Marks an entity with a Transform2D as a point that pulls particles, through an AttractorAffector.
// The pull falls with the square of the distance, softened near the center, so particles moving
// sideways fall into orbits: a circular orbit at distance r has speed sqrt(strength / r).
struct Attractor {
    // In world units cubed per second squared; negative pushes away.
    double strength = 4.0e6;
    // Distance below which the pull stops growing, in world units.
    double softening = 12.0;
    // Particles farther than this feel nothing; 0 means no limit.
    double radius = 0.0;
};

} // namespace pixie
