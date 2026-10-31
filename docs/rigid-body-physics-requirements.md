# Rigid Body Physics: Requirements and Proposed Design

**Document ID:** GLOBAL-RIGID-BODY-PHYSICS-REQUIREMENTS

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** proposed body dynamics, character coupling, editor workflow and acceptance scenarios

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## 1. Status and ownership

Design baseline: 2026-10-22. The owner requested a detailed specification before
continuing implementation, with body dynamics preceding lighting and representative
test rooms. This document expands that direction; it does not claim implementation,
owner acceptance of every proposed default, or promotion of runtime work.
[TODO.md](../TODO.md) remains the execution scheduler.

This document owns the proposed physics design input and its acceptance matrix.
The [consumer requirements](circuit-puzzle-requirements.md) own game needs;
[ROADMAP.md](../ROADMAP.md) owns engine direction. On implementation, public headers
and the runtime reference own the actual API/contract. Do not maintain a competing
implemented contract here. Use stable requirement IDs below when promoting work.

Initial design baseline: [static collision and character movement](collision-movement.md)
and [scene authoring](editor-movement.md) exist. Dynamic rigid bodies, shared world
gravity, platform carrying and character pushing do not. The current motor has its
own gravity; current Collider2D is static geometry. Earlier completion records
remain valid for those bounded deliveries, not for this wider target.

Implementation progress is owned by TODO-043; the [body dynamics guide](body-dynamics.md)
now describes its first native/scene increment and explicitly lists the remaining
gates. The initial-baseline paragraph above is historical context, not current
implemented capability status. The [qualification record](physics-qualification.md)
links each requirement below to its mechanism and evidence and lists the owner reviews that remain.

The existing private Box2D 3.2.0 backend is retained. Its official
[simulation documentation](https://box2d.org/documentation/md_simulation.html)
describes body types, mass data, stepping and events; the vendored headers remain
the version-specific implementation reference. No dependency upgrade is proposed.

## 2. Scope and completion boundary

The first complete body-dynamics delivery covers static, kinematic and dynamic
bodies; compound convex geometry; rotation; mass properties; forces and impulses;
contact materials; collision layers; sensors/events; fixed-step lifecycle;
character interactions; editor authoring; diagnostics and telemetry. It must work
through the generic player and editor without game-specific simulation code.

Joints are a named follow-up: hinges, motorized connections and distance constraints
are useful for mechanisms, but are not prerequisites for a free box to tumble.
Destruction/fracture, ropes/cloth/soft bodies, fluids, buoyancy, 3D volume simulation,
network rollback and cross-platform bitwise determinism are outside this delivery.
One-way platforms and drop-through traversal remain separate character features.
Do not describe the initial delivery as all possible physics completed.

## 3. Ownership and data model

Names below describe proposed responsibilities, not finalized public declarations.

| Owner | Authored data | Runtime data / commands |
|---|---|---|
| World physics settings | Gravity vector, units per meter, solver substeps, sleep policy, layer names | One native simulation service per World, timings and diagnostics |
| Body | Static/kinematic/dynamic type, initial velocities, gravity scale, damping, rotation lock, continuous-collision option, mass mode | Pose, velocities, awake state; force/torque/impulse, teleport and kinematic target commands |
| Collider shape | Local shape pose, geometry, density, friction, restitution, layer membership/mask, sensor flag | Shape identity and contact membership |
| Character motor | Capsule, movement/jump tuning, gravity scale, push limits and support rules | Commands, velocity, support identity, grounding and timers |
| Visual | Sprite/Shape, visual offset and interpolation | Presentation only; does not determine collision automatically |

**PHY-001 — One simulation owner.** A scene World owns exactly one native physics
service for all scene bodies, characters and queries. Do not mirror obstacles into
two independently stepped worlds. Backend types and IDs stay private. Standalone
low-level collision consumers remain supported; scene integration must not silently
change their explicit stepping or lifetime contract.

**PHY-002 — Body and collider are separate.** A collider without an explicit body
retains current implicit-static behavior. A body can own multiple local shapes
with stable shape identities; initial editor workflow keeps them in one entity's
shape list, without adding transform parenting as a hidden dependency. Existing
single Collider2D data must migrate without duplicated geometry. Body mass,
velocity and forces belong to the body, not each collider independently.

**PHY-003 — Exclusive integration.** Physics bodies and motor-controlled characters
bypass legacy Velocity2D/Acceleration2D integration. Those components must not
silently compete with body state; inspector warnings explain conflicting data.
A CharacterMotor2D plus ordinary dynamic body on the same entity is invalid in
this design. The motor remains a controlled capsule, not a second rigid-body solver.

## 4. Units, gravity and mass

**PHY-004 — Explicit units.** Retain world units, seconds and Y-down coordinates.
Proposed default scale is 32 world units per meter, matching the current adapter.
Mass is kilograms; density is kg/m². Public distance/velocity/acceleration and
linear force/impulse use world-unit length; angular quantities use radians.
Inertia and torque contain squared length and must receive squared conversion
factors at the backend boundary. Inspector labels state the units; user-facing
angles can display degrees with an explicit conversion. Tests cover every factor.
Changing units-per-meter while running requires a reset, not an invisible rescale.

**PHY-005 — World gravity.** Store one authored gravity vector per scene World;
per-body and per-motor gravity scale defaults to 1 and may be 0. Non-physics
entities are unaffected. Proposed migrated scene default is (0, 1100) world
units/s² to preserve the current sample's intended feel; this is stylized gravity,
not a claim of Earth units. General bodies accept vector gravity. The first
upright character controller supports vertical downward or zero gravity only;
unsupported sideways/upward gravity produces a clear motor diagnostic rather
than pretending arbitrary gravity traversal is implemented.

World settings must be serialized and editable once per scene. Prefer a single
identified settings entity using the existing scene descriptor path; never select
an arbitrary first entity when duplicates exist. Defaults handle scenes with no
settings entity. Duplicate settings are invalid and diagnosed. Settings fragments
must not silently replace the destination World's settings when pasted.

**PHY-006 — Computed mass.** Default mode derives total mass, local center of mass
and rotational inertia from non-sensor shapes and their densities/offsets. For a
rectangle, area is full width × full height, converted to m². A density change or
shape edit recomputes properties at a safe boundary. Sensors contribute no mass
in the engine contract, even if the backend could include them. Dynamic bodies
require finite positive mass and usable geometry; do not hide invalid input by
inventing a unit mass.

**PHY-007 — Explicit mass.** An optional total-mass override scales the density-derived
distribution and inertia consistently, preserving its computed center of mass.
Display effective mass, area, center of mass and inertia read-only. Manual center
of mass/inertia overrides are deferred until a consumer needs them. Runtime
geometry/mass changes preserve pose and linear/angular velocity and wake the body;
they are authoring changes, not a promise to conserve momentum or energy.

Physical volume is not inferred from a 2D silhouette. If gameplay later needs
volume, define thickness and volume units in that feature, with a deliberate mass
coupling rule; do not expose independent mass, density and volume fields that
contradict one another.

## 5. Motion and contact rules

**PHY-008 — Body types.** Static geometry does not move through simulation.
Kinematic motion follows commanded velocity or a target converted into motion
over the fixed step, without forces changing its prescribed path. Dynamic motion
responds to gravity, forces, torque, impulses and contacts. Static/kinematic pairs
do not generate ordinary blocking contacts with each other; query/sensor promises
must be tested separately. A kinematic door can push dynamic objects but does not
automatically stop because an object blocks it; stopping/reversing is game policy.

**PHY-009 — Commands.** Distinguish continuous force/torque for one fixed step from
instantaneous linear/angular impulse. An off-center force/impulse can rotate a
body. Commands name their world/local space and application point. Repeating a
force during multiple render frames must not multiply its fixed-step effect.
Teleport is explicit, resets interpolation history, optionally clears velocity,
and wakes the body. Kinematic motion uses motion commands, not per-frame teleports.

**PHY-010 — Shapes and materials.** Support boxes, circles, capsules and convex
polygons, plus multiple shapes on one body. Concave decomposition is deferred;
invalid polygons are rejected clearly. Physics geometry bakes authored scale;
changes rebuild safely rather than scaling the backend during the solve. Reject
unsupported nonuniform circle/capsule scale. Shape offsets and rotation must
produce the same outline, queries, mass distribution and contacts.

Friction is finite and nonnegative; restitution is in [0, 1]. Proposed mixing is
geometric-mean friction and maximum restitution, with a defined low-speed bounce
threshold. Record the threshold and tested backend behavior in the implemented
contract. Damping, rotation lock, sleep/wake and opt-in continuous collision are
exposed with useful defaults and diagnostics. No universal no-tunneling guarantee
is made for arbitrary sizes/speeds, particularly moving sensors.

**PHY-011 — Layers.** Editor shows named membership and collision checkboxes, not
raw bit integers. Both sides must permit a blocking pair. Keep numeric native
representation and old category/mask compatibility. Undefined set bits appear as
unnamed layers rather than being discarded; layer names survive save/reopen.
Raycasts/overlap tests explicitly select layers and sensor inclusion.

**PHY-012 — Events and identities.** Provide begin/end contact and sensor overlap
events with stable body/shape/entity identities, step index and an explicit end
reason for removal/disable/reset where applicable. Multiple shape pairs must not
be confused with one entity pair; offer deliberate aggregation if needed.
Destroying/disabling an overlapping entity cannot leave a permanent active pair.
Events are copied into engine-owned storage, readable until the next step/reset;
never expose temporary backend buffers. Structural mutation is deferred out of
solver/event collection. Reset establishes a new event generation; stale IDs
cannot refer to reused entities. Body hit events, if offered, carry documented
normal/point/impulse units and thresholds rather than being treated as begin events.

## 6. Character interaction is its own delivery gate

**PHY-013 — Support motion.** A character can stand on, ride and leave translating
and rotating kinematic platforms. Support tracks a local contact point so rotation
contributes tangential motion. Apply carrying once per step; stepping off must not
retain stale support. Proposed jump policy inherits support-point velocity once
at takeoff, with configurable limits. Grounding obeys the existing slope rule.

**PHY-014 — Pushing and receiving motion.** A motor can push dynamic objects through
bounded impulses with authored effective push mass/force/speed limits. The motor
does not become infinitely strong because its position is commanded. Dynamic
objects can obstruct or displace the capsule; jumping against a box must not
launch it through uncontrolled penetration correction. Standing on movable dynamic
support applies a defined effective load. Effective character mass here is a
coupling parameter, not a second separately integrated body. Character-to-character
collision remains explicitly unsupported unless separately promoted.

**PHY-015 — Blocked and crushing cases.** Platforms, doors and falling bodies must
not cause persistent overlap, NaN positions or endless correction. Expose a blocked
or unsafe-placement result; damage, death and mechanism reversal are game policy.
When a kinematic path leaves no valid space, return the last valid pose and report
failure instead of claiming a physically feasible solution. Recovery behavior and
supported maximum platform displacement per step require executable cases.

The precise motor/body coupling algorithm is an implementation-stage design gate:
document prediction, impulse application and post-solve support correction before
coding that stage. One backend step and one motor integration per tick are required;
a successful free-body demo does not close PHY-013 through PHY-015.

## 7. Step order, scene lifecycle and compatibility

**PHY-016 — Fixed timeline.** Proposed baseline is a 1/60 s fixed outer step and
four solver substeps, with host-owned bounded catch-up and recorded overload time.
No internal second accumulator in the physics service. Outer timestep changes
require restart. Interpolation uses previous/current physics poses including
shortest-path angular interpolation; rendering and debug drawing never step physics.

Within a tick: capture previous transforms; synchronize safe authored changes;
run gameplay to collect commands using the last completed state; perform the
declared motor/body coupling preparation; apply commands and solve once; finish
bounded motor/support correction; publish poses, diagnostics and copied events;
apply deferred destruction safely. Existing World systems must retain a documented
order. Game reactions consume events on the next update; no reentrant solve.

**PHY-017 — Persistence and reset.** Save authored settings, geometry and initial
conditions, not backend IDs, accumulated forces, contacts, sleep state or motor
timers. Edit does not advance simulation. Play uses a separate runtime state;
Stop reconstructs the authored scene. Save during Play retains authored data.
Runtime checkpoints are a separate persistence contract, not accidental scene Save.
Edit undo/redo recreates settings without ghost bodies or contacts. Two sessions
must remain isolated; repeated opening/closing releases resources and handles.

**PHY-018 — Migration.** Existing Collider2D-only scenes remain static. Migrate
half extents to a full Width/Height editor presentation without changing geometry.
When introducing world gravity, preserve old effective per-motor gravity through
an explicit compatibility conversion; do not silently retune differing motors.
The schema/version conversion must cover no settings, legacy gravity zero and
multiple legacy gravity values. Preserve unknown fields according to the existing
scene compatibility policy. Keep original saves available for comparison.

## 8. Editor, diagnostics and reusable tooling

**PHY-019 — Authoring workflow.** Provide static obstacle, dynamic box and kinematic
platform creation presets. Inspector groups World, Body, Shapes and Surface
properties, with tooltips/units and fields appropriate to the selected type.
Full Width/Height, radius and named layers are the primary user controls.
Computed mass data is visible. A compound-body workflow must be possible without
writing C++ or guessing component relationships. Resize/rotate handles and gizmos
must match the actual collider and remain usable when visual/collider sizes differ.

**PHY-020 — Debug views and errors.** Toggle collider outlines, body origins,
centers of mass, contact normals, sensors, awake/asleep state and velocities.
Show configuration errors once per relevant change, with the owning entity/shape
and corrective action. Failed replacement removes stale geometry and identifies
the disabled body; undo or a valid edit recovers. Invalid scale, density, duplicate
settings, nonfinite input and conflicting motion owners must not reach assertions.

Native simulation, commands, queries, snapshots and debug geometry remain usable
without C#. An in-game laboratory may reuse them, but its input capture, selection,
history and commit/reset UI are a separate tool-layer delivery. No in-game editor
is considered finished just because the desktop authoring workflow works.

**PHY-021 — Telemetry.** Record synchronization/rebuild, solver, motor coupling,
event collection and publication CPU time separately; body/shape counts, awake
bodies, contacts/sensors, event counts, pending commands and rebuilds per step.
Avoid routine whole-scene reconstruction and per-step heap churn after warm-up.
Measure disabled instrumentation overhead. CSV summaries include configuration,
machine, population, run length and render settings; report p50/p95/p99 and worst
step, not FPS alone. No performance budget is accepted without a named machine
and scene. A smaller active set should measurably reduce relevant work.

## 9. Acceptance matrix

All targets below are proposed, to be pinned before their implementation stage.
Tests name seed, exact input, initial scene, timestep, units, duration and tolerance.
Baseline supported envelope: scale 32 units/m, non-sensor minimum thickness 8 units,
circle radius at least 4 units, linear speed through 1024 units/s, angular speed
through 8 rad/s, masses 0.1–1000 kg; stability cases initially use interacting mass
ratios no greater than 10:1. Combinations beyond the qualified envelope remain
unqualified even if the backend happens to handle them.

| Case | Requirements | Observable evidence / proposed pass condition |
|---|---|---|
| Free fall / gravity scale | PHY-004–009 | Measured acceleration matches world gravity × scale within declared integration tolerance; scale 0 remains unaccelerated; old non-physics entities unchanged |
| Mass and torque | PHY-004, 006–010 | Analytic box/circle mass checks; offset compound center/inertia checks; centered impulse translates, offset impulse also rotates; lock suppresses rotation |
| Ramp box | PHY-008–010 | Named initial poses demonstrate rest, sliding and tipping; rotation is solver-produced; no scripted tipping animation substitutes for contacts |
| Stack | PHY-010, 016 | Ten boxes run 60 simulated seconds with no persistent penetration above 0.5 world units after settling; final settled position variation ≤0.25 units and angle variation ≤0.5° over one second; sleep and wake observed |
| Materials | PHY-010 | Controlled slope and drop cases distinguish friction/restitution; low-speed contacts settle rather than bounce indefinitely |
| Fast body | PHY-010, 016 | Declared fast-body mode crosses the tested speed range against thin static and representative moving obstacles without crossing them; failure limits documented |
| Platform and character | PHY-013–016 | Translate, rotate, reverse, ride, jump and step off; no double carry/stale support; inherited velocity matches declared rule; ceiling obstruction yields bounded failure |
| Box and character | PHY-014–015 | Push light/heavy boxes, stand on dynamic support and receive a moving box; push caps observed; no catapult or unsolved persistent overlap |
| Layers / sensors / queries | PHY-011–012 | Two-sided filter table; sensors do not block; enter/exit/remove/disable/filter edits/session reset produce coherent pair lifetimes; stale handles rejected |
| Lifecycle and authoring | PHY-017–020 | Create compound body through editor, undo/redo, save/reopen, Play/Stop, two sessions and invalid edits; no ghost bodies; legacy fixtures migrate without changed outlines/effective gravity |
| Render-rate independence | PHY-016 | Same fixed-tick command stream at 30/60/144 render FPS yields the same bounded simulation trace on the qualified build; smooth interpolated rotation checked visually |
| Stress / telemetry | PHY-021 | Seeded variants with 0, 100 and 500 dynamic bodies plus sleeping/awake variants; explain measured scaling and spikes; collect warm and cold results separately |

Bounded tests must demonstrate sensitivity, for example by disabling gravity,
rotation write-back, pair cleanup or platform carrying in a test-only variant.
Author checks and exit 0 are not independent acceptance. Native/editor footage or
screenshots plus written observation establish what was actually inspected;
recorded human testimony remains valid when its screenshot is unavailable, with
that limitation stated. Character feel still requires owner playtesting.

## 10. Proposed delivery gates

These are design stages, not promoted TODOs or a promise of automatic execution.

| Stage | Deliverable | Exit gate |
|---|---|---|
| D0 | Pin data/schema, units, step order, migration and fixtures | Review PHY-001–007 and PHY-016–018 against current source; no ambiguous pose/gravity ownership |
| D1 | Shared native body service and scene settings | Gravity, body types, commands, rotation, compound mass, lifecycle tests and standalone consumer |
| D2 | Materials, filters, events and fast-body qualification | Stack/ramp/fast-body/filter/event tests and intentional failure sensitivity |
| D3 | Editor workflow and permanent physics room | No-code create/edit/save/Play/Stop, readable fields/gizmos, telemetry and native visual evidence |
| D4 | Character/body/platform coupling | Explicit coupling design followed by PHY-013–015 scenarios and owner feel review |
| D5 | Consolidation before lighting | Debug/Release and consumer regressions, migration evidence, stress report and limitations; unresolved required cases remain open |

The physics room is built incrementally from D1, not postponed until D3. Desktop
authoring and generic-player behavior must use the same simulation. Passing D3
does not imply D4, and an unresolved D4 case cannot disappear from the completion
record. On delivery, link implementation and evidence per requirement rather
than marking this whole document implemented at once.

## 11. Lighting and room sequence after physics

Existing scene serialization is retained; a room is representative content, not
an excuse to replace the scene model. Lighting requirements already live under
L01 onward in the [consumer analysis](circuit-puzzle-requirements.md#4-lighting-quality-critical-requirements).
Before lighting code, expand its render contract and acceptance budget with the
same level of detail as this physics design. Deferred choices include shadow
technique, translucency, composition, light units and material response.

Proposed rooms: (1) a dark room with one movable light, static occluders and a
physical box; (2) multiple lights with rotating/moving occluders; (3) a powered/
unpowered machine with readable interaction targets and smooth light transitions;
(4) a combined representative room for atmosphere/performance and panel opening.
Every room needs save/reopen, editor preview/player parity and named captures.
Qualify shadow leaks, moving-shadow stability, overlapping lights, resolution/
zoom behavior and actual GPU/CPU cost; a pretty screenshot alone does not close
the lighting stage. Atmosphere also needs sound, camera and composition, which
remain separate consumer requirements.
