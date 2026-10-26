#pragma once
#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>
#include <cstdint>
#include <vector>

namespace pixie {
enum class BodyType2D { Static, Kinematic, Dynamic };
enum class BodyShapeKind2D { Box, Circle, Capsule, Polygon };
struct BodyHandle2D {
    const void* owner = nullptr;
    std::uint64_t value = 0;
    bool operator==(const BodyHandle2D&) const = default;
};
// Native compound shape. Distances are world units, density kg/m^2, angles radians.
struct BodyShape2D {
    BodyShapeKind2D kind = BodyShapeKind2D::Box;
    pxVec2 halfSize{16, 16};
    double radius = 16, halfSegment = 16;
    pxPoint offset;
    double rotation = 0;
    std::vector<pxPoint> points;
    double density = 1, friction = 0.6, restitution = 0;
    std::uint64_t category = 1, mask = ~std::uint64_t{0};
    bool sensor = false; // Nonblocking; contributes no mass even when density is authored.
};
// Authored body configuration. Current velocities/contacts live only in the service.
struct RigidBody2D {
    BodyType2D type = BodyType2D::Dynamic;
    pxVec2 initialVelocity{0, 0};
    double initialAngularVelocity = 0;
    double gravityScale = 1, linearDamping = 0, angularDamping = 0;
    double massOverride = 0; // Zero derives mass from shapes; positive scales their distribution.
    bool fixedRotation = false, continuous = false;
    bool operator==(const RigidBody2D&) const = default;
};
struct PhysicsSettings2D {
    pxVec2 gravity{0, 1100};
    double unitsPerMeter = 32;
    int substeps = 4;
    bool operator==(const PhysicsSettings2D&) const = default;
};
struct BodyMotion2D {
    pxPoint position;
    double rotation = 0;
    pxVec2 velocity{0, 0};
    double angularVelocity = 0;
    bool awake = false;
};
struct BodyMass2D {
    double mass = 0, inertia = 0; // kg and kg*worldUnits^2, about the center of mass.
    pxPoint localCenter;
    double area = 0; // m^2 of solid shapes, including zero-density shapes; overlaps add, sensors do not.
};
}
