#pragma once

#include <entt/entity/entity.hpp>

#include <pixie/math/pxVec.hpp>

#include <cstdint>
#include <numbers>

namespace pixie {

class Random;
struct ParticleSystem;
struct Transform2D;

// Where on the spawner a particle starts, around the spawner's Transform2D (which also rotates the
// rectangle and the segment).
enum class EmissionShape : std::uint8_t {
    Point,
    // Anywhere inside a disc of `radius`.
    Circle,
    // On a circle of `radius`.
    Ring,
    // Anywhere inside a rectangle of half extents `size`.
    Rectangle,
    // Anywhere on the segment from the spawner to `segmentEnd`, an offset from it.
    Segment,
};

// The angle a particle leaves at, before `direction` is added and the spread applied.
enum class DirectionMode : std::uint8_t {
    // The spawner's rotation.
    Fixed,
    // Outward from the shape where the particle starts: from the center for a point, a circle, or a
    // ring; the nearest edge's normal for a rectangle; the segment's left normal (+90 degrees).
    ShapeNormal,
    // Toward the target entity's Transform2D (the spawner's rotation when there is none).
    TowardTarget,
};

// An entity that feeds a ParticleSystem: `rate` particles per second leave its emission shape in a
// cone `spread` wide around a direction, at a speed in [speedMin, speedMax], and live `lifetime`
// seconds. Several spawners may feed one system. Angles are in radians.
struct ParticleSpawner {
    // The entity whose ParticleSystem receives the particles; none, the spawner emits nothing.
    entt::entity system = entt::null;
    double rate = 0.0;
    // Seconds; 0 means forever.
    float lifetime = 0.0f;
    double speedMin = 0.0;
    double speedMax = 0.0;
    EmissionShape shape = EmissionShape::Point;
    double radius = 0.0;
    pxVec2 size{0.0, 0.0};
    pxVec2 segmentEnd{0.0, 0.0};
    // Scatters each start around the point the shape chose, in world units: a Gaussian offset with
    // most particles (about 86%) within this distance. 0 keeps the shape's point.
    double jitter = 0.0;
    DirectionMode directionMode = DirectionMode::Fixed;
    // Added to the mode's angle.
    double direction = 0.0;
    // Width of the cone around the direction.
    double spread = 2.0 * std::numbers::pi;
    // For TowardTarget.
    entt::entity target = entt::null;
    // Fraction of a particle carried to the next step.
    double pending = 0.0;
};

// Each particle it emits in a step has its own moment within the step, when its share of `rate`
// came due: it is born there and then, so by the end of the step it has travelled, and aged, only
// for the part of the step it lived, and a fast stream comes out evenly spaced rather than in a
// stack per step. Its `previous` is its birth point, so it is drawn leaving the spawner.
//
// Emits the particles `spawner` owes for `delta` seconds into `system`, from `transform` (the
// spawner's), aiming at `target` (the target's transform, or null). Returns how many it emitted.
int EmitFromSpawner(ParticleSpawner& spawner, const Transform2D& transform, const Transform2D* target,
                    ParticleSystem& system, Random& random, double delta);

} // namespace pixie
