# Collision and Character Movement Foundation

**Document ID:** RUNTIME-COLLISION-MOVEMENT-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** static collision queries, capsule motion and caller-owned character control

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

The public APIs are [CollisionWorld2D](../runtime/include/pixie/runtime/CollisionWorld2D.hpp)
and [CharacterController2D](../runtime/include/pixie/runtime/CharacterController2D.hpp).
They form the first bounded movement foundation. Box2D is private to the
implementation; game code sees PixieLib points/vectors and engine handles.

## Ownership and update order

A caller owns a CollisionWorld2D and one or more controllers. All access is on
one thread. Geometry consists of explicitly added static boxes, circles and
convex polygons. Collider handles belong to that collision world, must not
outlive it and are not scene IDs or serialized values. Removal invalidates the
handle; foreign/stale handles are rejected by mutations. Transform edits update
queries immediately. A supplied Telemetry collector must outlive the scene.

Step a controller once per fixed update after setting the current geometry.
Use its returned position to present the character; interpolate from the prior
position when drawing between steps. The neutral sample does this through a
named World Physics-phase system. It does not attach Velocity2D to the character.
Do not integrate the same position through both the controller and Velocity2D.
Existing World integration/order and existing entities are unchanged.

The low-level API is explicit. The additional [scene authoring layer](editor-movement.md)
provides reflected components and generic session initialization, with saved data
and Play/Stop integration. Standalone users can still own the collision scene
and controller directly without using the scene layer.

## Units, geometry and query rules

Coordinates, dimensions, translations and speeds use world units, with Y down;
angles are radians except the named maxSlopeDegrees setting. The backend uses
meters privately, defaulting to 32 world units per meter. This default is an
adapter scale, not a mandate that all game art has 32 pixels per meter.

Inputs must be finite and converted coordinates must lie within +/-1,000,000
backend meters. This is an input bound, not a precision guarantee at that scale;
single-precision collision has scale-dependent tolerances, including the
pinned solver's 0.005-meter contact tolerance (0.16 world units at
the default scale). It permits that much penetration to reduce jitter; changing
the scale changes this world-unit tolerance. Radii/box extents
are positive. Polygons require 3..8 distinct points forming a nondegenerate
convex hull. Rotation and dimensions are explicit; there is no implicit sprite
or Transform2D scaling. Layer category/mask checks apply in both directions.

RayCast returns the closest hit or no hit, with world point, unit normal,
fraction of the requested translation and collider identity. Initial ray
overlap follows the backend's zero-fraction hit behavior. Capsule movement
resolves nearby contact planes, casts toward the target and clips velocity
against final contacts. It is a vertical capsule with fixed rotation.

Movement is bounded to eight passes and 64 contact planes; plane overflow is
reported rather than silently dropping contacts. Grounding requires a suitable
upward normal and velocity not directed away from it. This grounding threshold
is not a guarantee that all steeper geometry is impassable. Deep embedding,
impossible corridors and pathological corner sets need separate level/solver
policies; no universal depenetration or pixel-exact guarantee is claimed.

## Character tuning

Settings define capsule size, maximum horizontal speed, acceleration, braking, gravity,
terminal speed, jump speed, walkable support angle, coyote time and jump buffer.
Horizontal input is clamped to [-1,1]. Step requires 0 < delta <= 0.05 seconds;
use the existing fixed-step host rather than passing an arbitrary render delta.
Teleport clears velocity, grounding and buffered commands. On walkable support, gravity only
presses the capsule along the support's normal, so a character standing on a slope below the
walkable angle stays put; above it, the character is not grounded and slides. Jump is an optional
command used by the test room; it does not decide the side-scroller's abilities.

## Measurement and remaining work

An optional collector records capsule/raycast CPU time, static body count and
last solver iteration count without rendering. Use one collision scene per
collector for unambiguous gauges; repeated timing registrations aggregate by
name. Metrics are CPU costs, not GPU timing or proof of controller feel.

The [movement room](../samples/movement/README.md) is the neutral consumer.
Automated tests establish bounded mechanics and local repeatability, not
cross-platform bitwise determinism or owner acceptance of movement feel.

[Explicit rigid-body simulation](body-dynamics.md) is now available in the shared
query world. [Contact/sensor lifecycle events](body-events.md) are available for
physics shapes. [Bounded character/body coupling](character-body-coupling.md) now
adds platform carry, push/load and blocked/unsafe placement through the two-phase
native API; the standalone const Step path remains query-only. Player-capsule
sensor registration and one-way surfaces remain later stages. Scene serialization and
editor authoring are available through the linked scene layer. SetTransform can move static geometry between queries;
that alone is not a moving-platform controller.

## Backend provenance

The owner selected Box2D plus an engine-owned controller on 2026-10-22.
The pinned upstream [3.2.0 release](https://github.com/erincatto/box2d/releases/tag/v3.2.0)
is recorded in [DEPENDENCIES.md](../DEPENDENCIES.md). Its
[character toolkit](https://box2d.org/documentation/md_character.html) is marked
experimental upstream. The adapter uses its collide/cast/plane primitives;
input policy, tuning, grounding, command buffering and integration remain ours.
This isolation and focused tests limit reliance on upstream mover API details.
