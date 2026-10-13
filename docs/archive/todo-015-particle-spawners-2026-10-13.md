# PIXIE-TODO-015: Particle Systems and Spawners Apart

**Document ID:** ARCHIVE-TODO-015-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-015, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-030

**Type:** completion

**Origin:** PIXIE-TODO-015 (PIXIE-TASK-070 to PIXIE-TASK-073)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `d9538c9d723ae97edbb8c65403399368c5c53b9f`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [runtime reference](../modules/runtime/REFERENCE.md#particles)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13 (request E, answers 36 and 38)

**Accepted scope:** `ParticleEmitter` splits into `ParticleSystem` (pool, capacity, style, affectors) and `ParticleSpawner` entities that point at a system: rate, lifetime, speed range, an emission shape (point, circle, ring, rectangle, or segment, the last through PixieLib's `pxSegment`), and a direction mode (a fixed angle, the shape's outward normal, or toward a target entity), with `direction` and `spread` in radians, shown in degrees and edited together as an arc. Several spawners may feed one system. A new `Entity` field kind lets a descriptor reference another entity; the editor offers the entities that have the expected component. The particles sample uses spawners. The viewport gizmo for direction and cone is not part of this entry.

**Dependencies and gates:** 1. PIXIE-TODO-012; 2. PIXIE-TODO-014

**Completion criteria:** (1) tests cover each emission shape and direction mode and two spawners feeding one pool; (2) the particles sample runs with spawners, and the editor edits a spawner's direction and spread with the arc editor and its system and target by choice; (3) the performance targets of PIXIE-TODO-009 still hold; (4) zero warnings.

**Commits:** `d9538c9d723ae97edbb8c65403399368c5c53b9f` (the particle system and spawner, the Entity field kind in the runtime, the C API, and the editor, the sample, and tests); the runtime and capi references, the particles README, and the editor README are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-070` — **Status:** done — `ParticleSystem` (pool, capacity, style, affectors) and its update system, replacing `ParticleEmitter`.
- `PIXIE-TASK-071` — **Status:** done — `ParticleSpawner`: rate, lifetime, speed range, emission shapes (point, circle, ring, rectangle, segment), direction modes (fixed, shape normal, toward a target), direction and spread as an arc.
- `PIXIE-TASK-072` — **Status:** done — The `Entity` field kind in descriptors and the C API; the editor offers the entities that have the expected component.
- `PIXIE-TASK-073` — **Status:** done — Tests for each shape and direction mode and two spawners feeding one pool; the particles sample on spawners; the performance targets measured again.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51), .NET SDK 10.0.401; commit `d9538c9`.

- Debug and Release build with zero warnings, and the editor with zero
  warnings; `ctest` passes 5 of 5 in both, with 35 runtime tests.
- New tests: the two emission tests run through a spawner entity (rate,
  lifetime, aging, capacity); two spawners at 60 and 120 per second feed one
  pool 30 particles in 10 steps, 20 from the second, while a spawner pointing
  at no system emits nothing; each shape keeps its particles on it (the point,
  inside a disc, on a ring, inside a rectangle and on a segment both turned 90
  degrees with the spawner); each direction mode aims as described (fixed with
  the rotation plus the direction, outward on a ring, the segment's left
  normal, toward a target), and a spread of 1 radian keeps every angle within
  0.5 of the direction.
- Broken on purpose and restored: a ring at half its radius fails the shape
  test; ignoring the target fails the direction test.
- `capi-test` checks that `ParticleSpawner.system` is a `PIXIE_FIELD_ENTITY`
  referencing `ParticleSystem`. The editor run first showed `system` and
  `target` as numbers in an enum box: `entt::entity` is an enum class, so the
  reflection took it for an enum. The entity test now comes before the enum
  test, and the C API check would have caught it.
- The editor on particles, with real mouse-wheel input to scroll the
  inspector: the `spawner` entity selected, the direction mode set to
  `TowardTarget`, the spread to 20 degrees, and the target to "attractor #3"
  through the inspector. The saved frame shows the arc dial on `direction`
  with its spread, `target` as a choice reading "attractor #…", and the stream
  of particles leaving the ring toward the attractor. With the speed at 200 to
  220 it goes straight in.
- Performance, vsync on, 8-second runs, now with the spawner in the scene:
  100,000 particles against 16 attractors, interaction off, step 4.26 to
  4.40 ms on average, 5.9 ms at the slowest (5.4 to 5.7 ms in
  PIXIE-ARCHIVE-029: no slowdown, within run-to-run spread). 10,000 with the
  interaction on: 2.35 ms on average, 3.2 ms at the slowest, the pool also
  holding up to 1,200 particles from the spawner. Both targets stay well within
  the 16.7 ms budget.
- Not run: the rectangle and segment shapes in the editor (covered by tests),
  Linux.
