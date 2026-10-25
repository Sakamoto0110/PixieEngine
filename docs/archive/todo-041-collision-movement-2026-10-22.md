# Collision and Movement Foundation Completed: PIXIE-TODO-041

**Document ID:** ARCHIVE-TODO-041-COLLISION-MOVEMENT-20261022

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** private Box2D static collision API, explicit character controller and neutral movement room

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-079

**Type:** completion

**Origin:** PIXIE-TODO-041

**Author:** codex

**Reference date:** 2026-10-22

**Commits:** uncommitted working tree based on `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Current state:** [movement guide](../collision-movement.md), [sample](../../samples/movement/README.md)

## Accepted scope and decision

The owner selected collisions/movement as the next front and explicitly chose
Box2D behind PixieEngine's API plus an engine-owned character controller.
PIXIE-TODO-041 bounded the first delivery to static geometry/queries, capsule
movement, controller tuning, a neutral room, telemetry and regression evidence.
The selected backend and accepted scope are preserved here without a second
permission-only record. The original scheduler entry required source isolation,
ownership/units/input checks, floor/wall/ramp/overlap/high-speed/replay tests,
Debug/Release builds, runtime regressions, native evidence and documented limits.

## Task snapshot

**Parent:** PIXIE-TODO-041

- `PIXIE-TASK-172` — **Status:** done — Pin backend provenance, define public ownership/units and preserve World integration.
- `PIXIE-TASK-173` — **Status:** done — Implement static geometry, filtered queries, capsule collision resolution and controller tuning.
- `PIXIE-TASK-174` — **Status:** done — Verify collision, lifecycle, invalid inputs and deterministic local replay.
- `PIXIE-TASK-175` — **Status:** done — Deliver neutral native/headless room with telemetry; validate builds and document limits.

## Result and ownership

CollisionWorld2D owns an isolated backend scene. Public APIs expose PixieLib
primitives and engine collider identities, with private Box2D types. Static
boxes/circles/convex polygons, transform/removal operations, filtered closest
raycasts and capsule contact resolution are available. CharacterController2D
owns tunable acceleration, braking, gravity, grounding and optional jump commands
with buffering/grace. It is stepped explicitly; World/Velocity2D integration
was not changed. The sample runs it in a named World PostUpdate system and
interpolates presentation without attaching a second position integrator.

CPU query/movement timing and scene gauges use the existing collector. Gauges
refresh after collection is re-enabled. Collision and input policy are independent
of rendering. No C API surface/version change or editor component was introduced.

Box2D 3.2.0 was vendored from the official tag archive, SHA-256
`ed94013e8d16b19876757920bfa6d7663358aef8ba9407cc18146da6b714029e`.
All 321 archive files match the downloaded bytes; the MIT license is included.
It is a private static backend in the observed builds, single precision, one
query-world worker. Its experimental character primitives remain isolated by
the adapter. No cross-platform or pixel-exact movement guarantee is inferred.

## Verification

On the owner's Windows x64/MSVC machine (Ryzen 7 5700X), Debug and Release
builds passed with engine warnings treated as errors. Runtime tests passed
**112 tests in each configuration**, including **18 new collision/movement
tests**. Final Release CTest passed **11/11 scenarios**, including a separately
built project consumer. Debug's full native build passed; the final targeted
Debug runtime/sample rebuild and runtime suite passed after the gauge fix.
No Debug CTest or new C# editor build was performed for this delivery.

Coverage includes foreign/stale handles, shape edits/removal, masks in both
directions, malformed inputs, circle/polygon query identity, plane-capacity
failure, thin-wall high-speed casting, initial floor overlap, landing/no drift,
wall velocity, braking/reversal, ceiling collision, walkable ramps, steep-support
classification, buffered/grace jumps, teleport, scale tolerance, local replay,
legacy World integration and telemetry reset/enable transitions.

Initial tests exposed a real adapter error: fresh planes had no solver push
history, so velocity clipping was skipped. The adapter now explicitly marks
active touching planes before clipping. Contact assertions account for the
pinned solver's measured 0.005-meter tolerance: 0.16 world units at scale 32.
This tolerance is part of the documented limit, not hidden as pixel accuracy.

The neutral headless room completed 600 updates and exported 600 movement timing
samples with four static bodies. Final character position was approximately
(336.16, 150.16), grounded at the wall. Disabled collection retained the same
movement outcome with zero timing samples. The native autoplay room ran and
produced a 1280x720 capture; visual inspection showed floor, ramp, ceiling, wall
and the capsule resting at the wall. This is mechanical/autoplay evidence, not
an interactive keyboard playtest or owner acceptance of movement feel. Native
runs occurred alongside builds/checks; no FPS improvement or performance budget
acceptance is claimed.

Machine-local logs, source/binary hashes, CSV, dependency archive and capture are
retained in the dated movement validation folder. These files are local evidence,
not a promise of remote backup. Source, structural docs and record checks are
separate from native/consumer evidence. VAW binding resolved `bound_scoped` for
runtime, samples, build and documentation, profile `pixieengine` revision 1,
VAW baseline `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`.

## Remaining boundaries

This is a caller-owned static collision/controller foundation. Reflected ECS
components, scene serialization, editor authoring, trigger lifecycle events,
dynamic bodies, one-way rules and moving-platform attachment/crushing are not
implemented. Explicitly moving static geometry is not platform attachment.
Character dimensions/tuning are test-room choices; game abilities and final
feel still require playtesting. Only local same-build replay was compared.

Existing dirty work and old game checkouts/saves were preserved. No commit,
push, publication or independent owner acceptance occurred.
