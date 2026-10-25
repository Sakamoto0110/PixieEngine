# Body Dynamics Foundation

**Document ID:** RUNTIME-BODY-DYNAMICS-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** explicit body simulation, scene ownership and initial dynamics delivery limits

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Native ownership and units

[CollisionWorld2D](../runtime/include/pixie/runtime/CollisionWorld2D.hpp) now
supports opt-in rigid bodies through AddBody and StepBodies. Existing static
AddBox/Circle/Polygon and standalone CharacterController2D remain explicit;
queries do not advance time. All bodies/shapes share the same backend world.
[RigidBody2D.hpp](../runtime/include/pixie/runtime/RigidBody2D.hpp) owns body
configuration, shape descriptions, handles and motion/mass results.

Distances, velocity and acceleration use world units; mass is kg and density
kg/m². Force and linear impulse are kg*worldUnits/s² and kg*worldUnits/s.
Torque/angular impulse and returned inertia contain squared world-unit length;
the adapter converts them with unitsPerMeter². Angles are radians in C++.
Default scale is 32 world units per meter. Public dimensions/material scalars
are validated before backend creation. The currently accepted scale range is
0.001 through 1,000,000; this numerical guard is not a qualified gameplay envelope.

AddBody accepts 1–256 box/circle/capsule/convex-polygon shapes with individual
local poses, density and surface properties. Shapes share rigid motion but have
distinct query handles. Total mass/center/inertia are computed from geometry;
a positive massOverride scales that distribution. Zero requests computed mass; a positive override must be at least 0.000001 kg.
Dynamic bodies need positive-density geometry with computed mass in [0.000001, 1000000] kg. Inertia returned by BodyMass is
about the center of mass; fixed rotation suppresses angular response.

Body/shape handles are service-owned and runtime-only. ContainsBody/Contains and
removal reject foreign/stale handles. Identity values do not restart when a
service is recreated. RemoveBody destroys its shapes. For compatibility, Remove
on a collider removes its owning body, including all compound shapes; it is not
an individual compound-shape editing API.

SetGravity configures the shared vector and wakes dynamic bodies on a change;
unchanged synchronization preserves sleeping bodies. StepBodies advances once by the caller's
fixed delta (accepted range (0, 0.1] seconds), with 1–32 solver substeps; it has no
accumulator. ApplyForce/ApplyTorque last one physics step; impulses are immediate.
Application points use world coordinates. SetBodyVelocity commands dynamic or
kinematic velocity. TeleportBody resets velocity by default, or preserves it when
explicitly requested. Native callers own interpolation and command scheduling.

## Scene integration

ProjectSession already installs the scene service for the generic editor/player.
A Collider2D-only entity stays static. Add RigidBody2D and Transform2D to make it
an explicit static, kinematic or dynamic body. Collider2D supplies the first
scene shape and density/friction/restitution. CompoundCollider2D now provides
multiple independently posed/materialized scene shapes on that same body, with
typed list authoring in the editor.

One optional PhysicsSettings2D entity holds gravity, unitsPerMeter and substeps.
No settings entity uses (0,1100), 32 and 4. Duplicates/invalid settings disable
the service with a diagnostic, remove stale geometry and permit recovery after
correction. Scale changes after simulation has stepped require a new World/reset.
Edit-mode scale changes rebuild safely without advancing time.

The legacy velocity/acceleration integrator excludes both motor and rigid-body
entities. PreUpdate synchronizes authored state for queries. After gameplay and
legacy non-physics integration, PostUpdate refreshes changes, steps bodies once,
publishes position/rotation, then moves existing character motors. This is the
initial uncoupled order, not the future character pushing/platform-carry contract.

SceneBody2D maps an entity to its runtime body handle. ScenePhysicsWorld2D returns
a borrowed mutable service for commands; reacquire after synchronization/reset.
SceneColliderEntity2D maps hits for both static and rigid-body shapes. Moving a
body through external Transform2D edits teleports and clears velocities/reset
interpolation; physics write-back is tracked so it is not mistaken for an edit.
Shape/property rebuilds preserve live velocities and authored pose. Initial
velocities apply on initial construction/reset, not every step or retune.

CharacterMotor2D now takes the World's gravity: it falls at worldGravity.y ×
gravityScale, with the (0,1100) default when no settings entity exists. The
scale defaults to 1, may be 0, and must be finite and nonnegative. Version 1
motors saved their own acceleration as `gravity`; reading one converts it to
gravityScale = gravity / 1100, so each motor keeps the fall it had without
settings and the factor the earlier bridge gave it with them. Differing and zero
legacy values stay distinct. Saving writes version 2 without the old field;
version 2 or newer saves are never reconverted, and unknown fields are kept.
Checked-in version 1 saves, such as the movement laboratory, are read in place.
A frozen copy of that laboratory's version 1 save,
[movement-lab-v1.scene](../tests/runtime/fixtures/movement-lab-v1.scene), keeps the
comparison available after the sample itself is saved at version 2.
Native CharacterSettings2D keeps its own `gravity` acceleration.
Upright motors reject sideways or upward world gravity. Ordinary dynamic bodies
support vector gravity. A motor and rigid body on one entity is diagnosed rather
than integrated twice.

## Authoring and verification boundaries

CollisionLayers2D now stores scene layer names as records with fixed slots 1..31.
Collider2D and each compound shape expose **belongs to** / **collides with**
checkboxes. Renaming/clearing names preserves existing category/mask integers,
including unnamed set bits. See [named layer authoring](editor-movement.md#named-collision-layers).
This configuration does not introduce PhysicsSettings2D or change gravity.

The existing generated inspector exposes RigidBody2D and PhysicsSettings2D with
tooltips. Add them using Add component; select the corresponding entities in the
[physics-lab](../samples/physics-lab/README.md) sample for an initial example.
Scene Save/reopen stores configuration and initial velocities. ProjectSession
Save during Play retains the authored snapshot; Stop reconstructs it. Direct
WriteScene of a live World still serializes its current Transform2D: use the
session's authored-save workflow when that is the intended behavior.

This baseline began with D0/D1 under [TODO-043](../TODO.md#pixie-todo-043--rigid-body-dynamics-and-authoring);
later stage increments do not imply completion of the [full specification](rigid-body-physics-requirements.md).
[Contact/sensor events and bounded dynamics qualification](body-events.md) are now
available in the same service. The [editor workflow](editor-movement.md) now provides
body creation presets, native component admission and full collider size presentation.
Compound-shape authoring now uses native record lists, shared body motion and the
[compound laboratory](../samples/compound-lab/README.md). Named-layer UI is delivered; computed-mass
inspector views remain pending. Stack/material and representative fast-body fixtures qualify the
cases documented in that guide; broader stress qualification remains pending. Motor queries can see dynamic
geometry, but pushing, loading and platform carrying are not implemented or
claimed. Do not use this baseline as evidence of safe crushing behavior.

Telemetry retains collision.static_bodies and adds physics.bodies and
physics.solve; event collection and pair/sensor gauges extend them in D2.
Full synchronization/coupling counters and budgets belong
to later gates. A visible tumbling box proves that scenario, not general
stability or owner acceptance of character feel.

The executable regression cases are in
[RigidBodyTests.cpp](../tests/runtime/RigidBodyTests.cpp) and the C API
[body-authoring fixture](../tests/capi/CapiTest.c). They cover analytic unit/mass
checks, contact-induced rotation, world/scene ownership and session Save/Stop/
reopen. The test-only omitted-body variant must fail; it is not a production
runtime option. Machine-local logs/captures for this increment are stored under
`.local/validation/body-dynamics-2026-10-22/`, with their remaining evidence limits.

