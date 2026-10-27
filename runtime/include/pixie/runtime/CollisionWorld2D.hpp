#pragma once

#include "pixie/Telemetry.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/PhysicsDebug2D.hpp"
#include <pixie/math/pxVec.hpp>
#include <pixie/pxPoint.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace pixie {

// Runtime-only identity. A handle belongs to one live CollisionWorld2D and is never serialized.
struct ColliderHandle2D {
    const void* owner = nullptr;
    std::uint64_t value = 0;
    bool operator==(const ColliderHandle2D&) const = default;
};

struct CollisionFilter2D {
    std::uint64_t category = 1;
    std::uint64_t mask = ~std::uint64_t{0};
};

struct CollisionHit2D {
    ColliderHandle2D collider;
    pxPoint point;
    pxVec2 normal;
    double fraction = 0;
    bool sensor = false;
    BodyHandle2D body;
};

struct CollisionMaterial2D { double friction = 0, restitution = 0; };

enum class PhysicsEventKind2D { Begin, End };
enum class PhysicsPairKind2D { Contact, Sensor };
enum class PhysicsEndReason2D { Separated, Removed, Disabled, Reconfigured, FilterChanged, Reset, Teleported };
struct PhysicsEndpoint2D {
    ColliderHandle2D collider;
    BodyHandle2D body;
    bool sensor = false;
};
struct PhysicsEvent2D {
    PhysicsEventKind2D kind = PhysicsEventKind2D::Begin;
    PhysicsPairKind2D pair = PhysicsPairKind2D::Contact;
    PhysicsEndReason2D reason = PhysicsEndReason2D::Separated;
    PhysicsEndpoint2D a, b; // Contacts sort shape IDs; sensor pairs keep the sensor in a.
    std::uint64_t step = 0, generation = 0;
};
// On-demand post-solve manifold point. Normal points from a to b; impulse is kg*worldUnits/s.
struct PhysicsContactPoint2D {
    PhysicsEndpoint2D a, b;
    pxPoint point;
    pxVec2 normal;
    double separation = 0, normalImpulse = 0;
    std::uint16_t pointId = 0;
};
struct PhysicsStats2D {
    std::size_t bodies = 0, shapes = 0, sensors = 0, contactPairs = 0, sensorPairs = 0;
    int awakeBodies = 0;
};

struct CapsuleMotion2D {
    pxPoint position;
    pxVec2 velocity{0, 0};
    pxVec2 groundNormal{0, -1};
    bool grounded = false;
    int planeCount = 0;
    int solverIterations = 0;
};

// Query snapshot; normals point from the obstacle toward the capsule. Separation and point
// use world units. Contains only nonsensor shapes, in stable collider-identity order.
struct CapsuleContact2D {
    ColliderHandle2D collider;
    BodyHandle2D body;
    pxPoint point;
    pxVec2 normal;
    double separation = 0;
};

// Caller-owned collision/query scene, main-thread only. Public distances use world units,
// Y down. Backend meters are private. Telemetry, if supplied, must outlive this object.
// Legacy AddBox/Circle/Polygon stay static. Dynamics advance only through explicit StepBodies.
// This never changes a World/Velocity2D directly. Geometry changes are explicit.
class CollisionWorld2D {
public:
    explicit CollisionWorld2D(double unitsPerMeter = 32, Telemetry* telemetry = nullptr);
    ~CollisionWorld2D();
    CollisionWorld2D(const CollisionWorld2D&) = delete;
    CollisionWorld2D& operator=(const CollisionWorld2D&) = delete;

    // Mover queries require radius > 0.01 meters after float conversion; throws with the
    // scale-dependent world-unit minimum. Scene tools can validate without advancing time.
    void ValidateMoverCapsule(double radius, double halfSegment) const;
    // Same validation for authoring tools before a backend exists; no World is allocated.
    static void ValidateMoverCapsule(double radius, double halfSegment, double unitsPerMeter);

    ColliderHandle2D AddBox(pxPoint center, pxVec2 halfSize, double rotation = 0,
                            CollisionFilter2D filter = {});
    ColliderHandle2D AddCircle(pxPoint center, double radius, CollisionFilter2D filter = {});
    // 3..8 points forming a convex hull, in local world units; degenerate input is rejected.
    ColliderHandle2D AddPolygon(pxPoint center, std::span<const pxPoint> points,
                                double rotation = 0, CollisionFilter2D filter = {});
    bool Remove(ColliderHandle2D collider, PhysicsEndReason2D reason = PhysicsEndReason2D::Removed);
    bool Contains(ColliderHandle2D collider) const;
    void SetTransform(ColliderHandle2D collider, pxPoint center, double rotation = 0);
    std::size_t Size() const;
    // All shapes share this query world; no second backend simulation is created.
    BodyHandle2D AddBody(pxPoint position, double rotation, const RigidBody2D& definition,
                         std::span<const BodyShape2D> shapes);
    bool RemoveBody(BodyHandle2D body, PhysicsEndReason2D reason = PhysicsEndReason2D::Removed);
    bool ContainsBody(BodyHandle2D body) const;
    std::vector<ColliderHandle2D> BodyColliders(BodyHandle2D body) const;
    CollisionMaterial2D Material(ColliderHandle2D collider) const;
    BodyMotion2D BodyMotion(BodyHandle2D body) const;
    BodyMass2D BodyMass(BodyHandle2D body) const;
    // Copies actual contact manifolds without stepping or waking bodies; may include speculative points.
    std::vector<PhysicsContactPoint2D> BodyContacts(BodyHandle2D body) const;
    BodyType2D BodyType(BodyHandle2D body) const;
    bool BodyEnabled(BodyHandle2D body) const;
    // Explicit teleport/filter/enabled-state revision; ordinary solver motion does not increment it.
    std::uint64_t BodyRevision(BodyHandle2D body) const;
    pxPoint BodyLocalPoint(BodyHandle2D body, pxPoint worldPoint) const;
    pxPoint BodyWorldPoint(BodyHandle2D body, pxPoint localPoint) const;
    pxVec2 BodyPointVelocity(BodyHandle2D body, pxPoint worldPoint) const;
    void SetBodyVelocity(BodyHandle2D body, pxVec2 velocity, double angularVelocity);
    void SetBodyEnabled(BodyHandle2D body, bool enabled);
    void SetCollisionFilter(ColliderHandle2D collider, CollisionFilter2D filter);
    void TeleportBody(BodyHandle2D body, pxPoint position, double rotation, bool clearVelocity = true);
    void ApplyForce(BodyHandle2D body, pxVec2 force, pxPoint worldPoint);
    void ApplyImpulse(BodyHandle2D body, pxVec2 impulse, pxPoint worldPoint);
    void ApplyTorque(BodyHandle2D body, double torque);
    void ApplyAngularImpulse(BodyHandle2D body, double impulse);
    void SetGravity(pxVec2 gravity);
    pxVec2 Gravity() const;
    void StepBodies(double delta, int substeps = 4);
    // Engine-owned snapshot, invalidated only by the next StepBodies/Reset, not by queued removals.
    std::span<const PhysicsEvent2D> Events() const;
    PhysicsStats2D Stats() const;
    // Copies backend geometry/manifolds without stepping, changing sleep state or consuming events.
    std::vector<PhysicsDebugLine2D> DebugGeometry(std::uint32_t flags) const;
    std::uint64_t Generation() const;
    bool NeedsSimulation() const;
    // Publishes reset ends for old identities immediately, then starts a fresh generation.
    void Reset();
    std::optional<CollisionHit2D> RayCast(pxPoint origin, pxVec2 translation,
                                         CollisionFilter2D filter = {}, bool includeSensors = false) const;
    std::optional<CollisionHit2D> CircleCast(pxPoint origin, double radius, pxVec2 translation,
                                           CollisionFilter2D filter = {}) const;
    bool OverlapsCircle(pxPoint center, double radius, CollisionFilter2D filter = {}) const;
    std::vector<ColliderHandle2D> OverlapBox(pxPoint center, pxVec2 halfSize,
                                            CollisionFilter2D filter = {}, bool includeSensors = false) const;
    // Capsule is vertical: segment endpoints at center Y +/- halfSegment, plus radius.
    // Resolves initial overlap, casts towards target and clips velocity. Slopes are grounded only
    // when their upward normal meets minGroundNormal. Maximum 64 simultaneous planes; throws on overflow.
    CapsuleMotion2D MoveCapsule(pxPoint center, double radius, double halfSegment,
                                pxVec2 translation, pxVec2 velocity,
                                double minGroundNormal = 0.70710678,
                                CollisionFilter2D filter = {}) const;
    std::vector<CapsuleContact2D> CapsuleContacts(pxPoint center, double radius, double halfSegment,
                                               CollisionFilter2D filter = {}) const;
    double CapsuleSlop() const; // Same touching tolerance as MoveCapsule, in world units.
private:
    struct Impl;
    // Sole ownership keeps backend headers and lifetime out of the public API.
    std::unique_ptr<Impl> impl_;
};

} // namespace pixie
