# PIXIE-TODO-008: Spatial Grid and Anchor 2, Flocks

**Document ID:** ARCHIVE-TODO-008-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-008, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-020

**Type:** completion

**Origin:** PIXIE-TODO-008 (PIXIE-TASK-036 to PIXIE-TASK-039)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `147ec99816aa7af225985b2ffcc6db0e4c11d1ac`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`samples/flocks/README.md`](../../samples/flocks/README.md) and the [runtime reference](../modules/runtime/REFERENCE.md#order-independence-and-neighbours)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** [PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md), anchor 2

**Accepted scope:** a uniform spatial grid in the `runtime` module, rebuilt in `PreUpdate`, that answers radius queries. A `flocks` sample: boids steered by separation, alignment, and cohesion with their neighbours, and by the mouse; each writes only its own acceleration in `Update`, and `PostUpdate` integrates. Keys tune the weights until the editor exists.

**Dependencies and gates:** 1. PIXIE-TODO-007

**Completion criteria:** (1) grid tests agree with a brute-force search over random points; (2) the result of a step does not depend on update order (a test shuffles it); (3) 5,000 flocking boids at 60 updates per second, measured and recorded; (4) zero warnings in Debug and Release.

**Commits:** `147ec99816aa7af225985b2ffcc6db0e4c11d1ac` (grid, index, acceleration, tests, and the sample); the README and the runtime reference are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-036` — **Status:** done — `SpatialGrid` in the `runtime` module: a uniform grid rebuilt in `PreUpdate`, with radius queries; tests against a brute-force search over random points.
- `PIXIE-TASK-037` — **Status:** done — `flocks` sample: separation, alignment, and cohesion over grid neighbours, plus the mouse; each boid writes only its own acceleration; keys tune the weights; its `README.md`.
- `PIXIE-TASK-038` — **Status:** done — A test that shuffles the Behaviour update order and gets the same step result.
- `PIXIE-TASK-039` — **Status:** done — Verification: 5,000 flocking boids at 60 updates per second, measured and recorded.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51); commit `147ec99`, Release unless noted.

- Debug and Release build with zero warnings; `ctest` passes in both, with 19
  runtime tests. The Debug sample runs with GL debug output on and no GL
  message.
- Grid tests: 3,000 random points, cell sizes 7, 50, and 333, 200 random
  queries each with radii up to 260, agree with a brute-force search and
  report no item twice. Broken on purpose and restored: without the own-cell
  check, items in colliding cells are counted twice and the comparison fails
  (358 failed checks); with a strict `<`, the boundary test fails.
- Order independence: two Worlds of 400 steering boids, one with its
  Behaviours' storage shuffled, end 30 steps bit for bit the same when the
  Behaviour writes its `Acceleration2D`. The negative control, the same
  Behaviour writing its `Velocity2D` during `Update`, ends different, so the
  test catches a Behaviour that breaks the rule.
- Real input through the operating system: with the cursor held at the middle
  of the window, Space switched the pointer to repelling (logged), and the
  saved frame shows the flocks parting around the red ring; Q twice, W once,
  and D once changed the weights as logged (separation 2.0, alignment 1.1,
  cohesion 0.8). A 12-second run without input shows aligned groups of boids
  sharing a heading color.
- Measured with vsync on, 8-second runs, from the sample's log:

  | Boids | Frames per second | Updates per second | Step, average | Step, slowest |
  |---|---|---|---|---|
  | 2,000 | 100 | 60 | 2.0 to 2.1 ms | 2.7 ms |
  | 5,000 | 100 | 60 | 6.9 to 7.5 ms | 9.3 ms |
  | 10,000 | 2.8 | 42.7 | 23.2 to 23.4 ms | 27.5 ms |

  The completion target, 5,000 flocking boids at 60 updates per second, holds
  with the step at under half the 16.7 ms budget. At 10,000 the step exceeds
  the budget: the loop falls behind and frames collapse, since the main loop
  clamps a frame to 0.25 seconds of catch-up. The flock lives in the visible
  area, so doubling the count doubles the density and each boid's neighbours;
  the cost grows close to the square of the count.
- Not run: optimizations (caching neighbours' velocities in the grid, capping
  the neighbours considered), another machine, Linux.
