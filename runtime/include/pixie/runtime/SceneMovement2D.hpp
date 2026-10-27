#pragma once

#include "pixie/runtime/CharacterController2D.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include <entt/entity/entity.hpp>
#include <string>
#include <array>
#include <vector>
#include <pixie/pxRect.hpp>

namespace pixie {
class World;
class ShapeRenderer;
class Camera2D;

// Stable 1-based slots name the existing 31 scene filter bits. List order is immaterial;
// removing a name never removes or renumbers a collider's bits.
struct CollisionLayer2D {
    int slot = 1;
    std::string name;
    bool operator==(const CollisionLayer2D&) const = default;
};
struct CollisionLayers2D {
    std::vector<ComponentRecord<CollisionLayer2D>> layers{{{1, "Default"}, {}}};
    bool operator==(const CollisionLayers2D&) const = default;
};
struct CollisionLayerNames2D {
    std::array<std::string, 31> names;
    std::vector<std::string> errors;
};
// Empty/undefined slots are returned as "Unnamed layer N". Invalid duplicate slots are
// unnamed, diagnosed and never used to change physics filters. No backend is allocated.
CollisionLayerNames2D DescribeCollisionLayers2D(const World& world);

enum class ColliderKind2D { Box, Circle, Polygon };

// Authored static geometry. Transform2D supplies placement/rotation/scale; offset is local.
// Category/mask expose 31 bits so existing integer inspectors represent every value exactly.
struct Collider2D {
    ColliderKind2D kind = ColliderKind2D::Box;
    pxVec2 halfSize{80, 12};
    double radius = 24;
    pxPoint offset;
    std::vector<pxPoint> points{{-24, -12}, {24, -12}, {24, 12}, {-24, 12}};
    int category = 1;
    int mask = 0x7fffffff;
    bool enabled = true;
    double density = 1, friction = 0.6, restitution = 0;
    bool sensor = false;
    double rotation = 0; // local radians, applied before entity scale and placement
    bool operator==(const Collider2D&) const = default;
};

// One body with independently posed/materialized shapes. It replaces Collider2D on that entity.
// Without RigidBody2D this is static geometry. An empty/all-disabled list has no active body.
struct CompoundCollider2D {
    std::vector<ComponentRecord<Collider2D>> shapes{{Collider2D{}, {}}};
    bool enabled = true;
    bool operator==(const CompoundCollider2D&) const = default;
};

// Only authored settings are saved. Velocity, grounding, timers and backend handles are runtime state.
// The tuning is CharacterSettings2D's, except gravity: the motor falls at the World's gravity
// (PhysicsSettings2D, by default 1100 down) times gravityScale. Version 1 saved its own acceleration;
// reading one sets gravityScale to that acceleration / 1100, so an old scene falls as it did.
struct CharacterMotor2D {
    double radius = 12;
    double halfSegment = 12;
    double maxSpeed = 200;
    double acceleration = 1400;
    double braking = 1800;
    double gravityScale = 1;
    double terminalSpeed = 700;
    double jumpSpeed = 380;
    double maxSlopeDegrees = 45;
    double coyoteSeconds = 0.08;
    double jumpBufferSeconds = 0.1;
    double effectiveMass = 1;
    double maxPushForce = 1200, maxPushSpeed = 200, maxSupportForce = 1200;
    double maxInheritedSpeed = 512, maxCarryStep = 8, maxRecoveryDistance = 8;
    bool operator==(const CharacterMotor2D&) const = default;
};

// Optional input bridge for a playable scene without game code. Omit it when game code supplies commands.
struct KeyboardControl2D { bool enabled = true; };
struct CharacterCommand2D { double horizontal = 0; bool jumpPressed = false; };

// Idempotent per World. A session installs this after loading/building its scene, in both hosts.
// No collision backend is allocated until the scene contains relevant components.
void InstallSceneMovement2D(World& world);
// Refreshes authored geometry/settings without advancing time, including while editing.
void SyncSceneMovement2D(World& world);
// Rebuilds transient bodies, contacts and motor state from current authored components/poses.
// Call outside a World step. Borrowed collision/motor views must be reacquired afterward.
void ResetSceneMovement2D(World& world);
std::vector<std::string> SceneMovementErrors2D(World& world);
const CapsuleMotion2D* CharacterMotion2D(World& world, entt::entity entity);
// Last synchronized controller settings, borrowed with the same lifetime as CharacterMotion2D.
const CharacterSettings2D* CharacterTuning2D(World& world, entt::entity entity);
const CharacterCouplingState2D* CharacterCoupling2D(World& world, entt::entity entity);
// Borrowed query view after the latest sync; null until relevant components are synchronized.
const CollisionWorld2D* SceneCollisionWorld2D(World& world);
// Borrowed mutable service for game commands; reacquire after sync/reset. Empty handle if invalid/missing.
CollisionWorld2D* ScenePhysicsWorld2D(World& world);
BodyHandle2D SceneBody2D(World& world, entt::entity entity);
entt::entity SceneColliderEntity2D(World& world, ColliderHandle2D collider);
struct ScenePhysicsEvent2D {
    PhysicsEvent2D event;
    entt::entity entityA = entt::null, entityB = entt::null;
};
// Last completed scene tick; identities of removed entities are retained in the copied event.
std::span<const ScenePhysicsEvent2D> ScenePhysicsEvents2D(World& world);

// Reusable authoring presentation, not a dependency on the C# editor. Empty for invalid/missing geometry.
std::vector<pxPoint> MovementOutline2D(World& world, entt::entity entity);
std::vector<std::vector<pxPoint>> MovementOutlines2D(World& world, entt::entity entity);
void DrawMovementGizmos2D(World& world, ShapeRenderer& renderer, const Camera2D& camera);
entt::entity PickMovementGizmo2D(World& world, pxPoint point, double tolerance);
std::vector<entt::entity> PickMovementGizmosIn2D(World& world, const pxRect& rectangle);
}
