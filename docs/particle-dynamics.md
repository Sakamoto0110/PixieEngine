# Dynamic Particle Sparks

**Document ID:** RUNTIME-PARTICLE-DYNAMICS-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** first bounded native gravity/bounce path and the interactive sparks room

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Accepted first increment

Owner request, 2026-10-26: clicking fires sparks; a player pushing a dynamic box along the floor
creates sparks; a shortcut resets the room. R resets the authored room state,
live particles, player/box motion and the random sequence. Physics stays native.

Particles keep their 40-byte CPU/GPU layout and interpolation previous position.
The reserved dynamic bit is chosen at birth, by a spawner or explicit emission.
Static and dynamic particles can share a pool. An optional ParticlePhysics2D
component on the pool supplies shared gravity scale, collision radius,
restitution, friction, bounce threshold and collision filter. Per-particle
profiles, side tables, streams and SIMD loop redesign are later work.

## Algorithm pinned before implementation

Install scene physics before the particle step. Apply affectors as before, then
integrate each particle once. Ordinary particles keep their existing ballistic
path without collider queries. Dynamic particles receive world gravity and sweep
a circle through the current scene collision world. Newborn dynamics begin at
their birth point and integrate only their age within this tick; they must not
sweep the legacy backdated ballistic segment before birth.

Circle casts ignore sensors and honor the filter. On contact, work in the
surface point's moving frame. Restitution uses the greater particle/surface
value; Coulomb friction uses their geometric mean. Small incoming normal speeds
use zero restitution to let sparks settle. Keep previous at the step's starting
pose, not reconstructed from the post-bounce velocity.

At most four contact responses per particle per tick bound corner work. On
exhaustion, discard remaining travel and stop; do not silently pass through the
obstacle. Recover an initial overlap through the existing mover query only within
four collision radii; unrecoverable particles expire. The collision radius must
satisfy the native mover's world-scale minimum. No particle creates a backend
body, pushes a body, collides with another particle through this path or advances
the physics world again. Existing ParticleInteraction remains opt-in and separate.

Dynamic CPU time, population, query/contact counts and bounded failures are
reported separately. No dynamic population is silently capped by physics.
Moving geometry is queried at its completed pose; arbitrary-speed moving-body
crossings and deformable/rotating particle shapes are outside this increment.

## Interactive room

The sparks sample consumes the runtime service. Right click emits a burst; left
drag selects/moves a box or the player. A/D or arrows move the character and Space
jumps. All dynamic boxes emit from native contact impulses and relative sliding
speed, on floor, ramp or other boxes. R restores bodies, masses, effects and seed. The sample's input policy is not part of the engine's
particle physics. Particle appearance remains authored at birth.

The owner clarified during implementation that the box is pushed by a player,
not dragged with a mouse. The room uses the native character/body coupling;
mouse input is reserved for spark bursts. The profile component is required on
the receiving pool to enable gravity/collision; omitting it keeps motion
ballistic, while preserving dynamic newborn timing. Collision uses the shared
profile radius, independently of artwork size/shape. Query-only character
capsules are not particle obstacles in this first increment.

`particles.physics` measures the configured pool's integration pass, including
flag scanning and ordinary-particle fallback. Gauges report dynamic particles
visited, geometric queries, contacts, expired invalid/unrecoverable particles
and exhausted contact budgets for the latest step. Physics queries run only for
dynamic births. Four responses bound work per particle, not the total dynamic
population. A deeply buried birth that supplies no mover plane is detected by
an overlap query and expired. Invalid tuning/delta raises a native validation
error; it is not silently accepted.

Shared physics settings are live per pool; only birth kind and appearance are
fixed per particle. R rebuilds transient scene physics after restoring authored
player/box poses, clearing contact warm state as well as their velocities.

## Bounded verification

The initial room passed 229 runtime tests and all 17 CTest suites in Debug and
Release. The C API suite includes click, player push, floor emission and R reset
with movement held. A separate hidden native input/render check confirmed 48
click sparks, actual dynamic box displacement with floor sparks, cleared motion,
repeatable particle images after R, and Stop restoration without changing the
scene file. Disabling bounce and disabling particle clearing on R each failed
its corresponding check; exact source restoration passed again.

This is a Windows/OpenGL bounded first increment. The earlier 100,000-particle
non-interacting measurements are not dynamic-collision performance evidence.
Physical-pointer acceptance and larger dynamic workloads are not claimed.

## Accepted room extension

The owner requested freely rotating rectangular boxes and a single editable mass
textbox for the selected body. The field uses kilograms (mass, not gravitational
force), accepts positive finite values in [0.01, 1000] and commits on Enter.
Escape cancels; invalid input leaves the body unchanged. Selecting another body
cancels uncommitted text. R restores authored mass and poses as well as motion.
Right click emits sparks; left selects/drags movable bodies. These UI changes do
not increase the particle layout or allocate a rigid body per particle.

The contact-room revision below supersedes the original proximity-based emitter
and implements the reviewed robustness corrections.

## Accepted contact-room revision

Owner request: implement the reviewed robustness fixes and replace the blue-box
floor-distance approximation with native contact-driven emission for all movable
boxes, including freely rotating rectangles, ramps and box-on-box landings.
Physics resolution runs in a dedicated phase before PostUpdate, independently of
registration order. AddParticleStep no longer installs scene movement; a standalone
collision-enabled World installs it explicitly (ProjectSession already does).

Invalid pool settings report a per-pool diagnostic and move that tick ballistically,
with no invalid native query. The direct particle API continues to throw. World
stepping clears its stepping flag on exceptions; it does not roll back partial work.

Native contact snapshots expose stable endpoints, point identity, normal, separation
and normal impulse. The room derives impact intensity from an increase in impulse
per reduced mass and sliding from post-solve relative tangential speed.
Impact count uses the square root of impulse rise times that speed change, so
greater effective mass increases the bounded effect at the same drop speed. These are
FX signals, not a force applied by particles. Sleeping stacks must not emit forever.
At most 256 sparks are emitted per scene tick, each point contributes at most 32,
and births probe a bounded set of nearby free positions. One 10,000-particle pool
and the 40-byte layout remain unchanged. No particle rigid bodies are allocated.

## Contact-room qualification

The revised runtime passed 235 runtime tests and all 17 CTest suites in each
Debug/Release configuration. Editor Release built without warnings/errors.
Six new regressions cover exception-state recovery, Physics order, legacy
integration before native geometry, ordinary particles without scene-system
installation, invalid-pool fallback/recovery and native manifold snapshots.

Persistent C API fixtures remove the blue box and isolate green-floor, ramp and
box-on-box emission. A 4 kg drop produced 16 surviving counted sparks versus 8
at 1 kg in the same bounded fixture. An isolated sliding-only fixture left only
the blue push box: initial rest emitted zero, motor-driven sliding emitted 76,
and settled rest emitted zero. The two affected suites passed per configuration
after adding this last fixture. These are FX signals, not calibrated abrasion
energy. Native hidden GL captures showed contact sparks on ramps/boxes in both
configurations and verified R clearing without changing the authored scene file.
