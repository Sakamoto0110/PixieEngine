# PIXIE-TODO-007: Anchor 1, Boids Steered by the User

**Document ID:** ARCHIVE-TODO-007-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-007, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-019

**Type:** completion

**Origin:** PIXIE-TODO-007 (PIXIE-TASK-033 to PIXIE-TASK-035)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `8162c4d4b4be6527ff336fa6d59a48fb0ac45426`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`samples/boids/README.md`](../../samples/boids/README.md)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** [PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md), anchor 1

**Accepted scope:** a `boids` sample. Each boid is an entity with a `Boid` Behaviour that steers toward or away from the mouse within a radius, with a maximum speed and force, and wraps at the view edges. Boids are drawn as triangles oriented by their velocity, interpolated. Keys switch between attracting and repelling and change the count.

**Dependencies and gates:** 1. PIXIE-TODO-006

**Completion criteria:** (1) 1,000 boids at 60 updates per second, measured and recorded; (2) the steering responds to the mouse as the `README.md` describes; (3) zero warnings in Debug and Release.

**Commits:** `8162c4d4b4be6527ff336fa6d59a48fb0ac45426` (the sample, the step timer, and the ring width); the README is added with this record.

## Breakdown as completed

- `PIXIE-TASK-033` — **Status:** done — `boids` sample: the `Boid` Behaviour steering toward or away from the mouse within a radius, with maximum speed and force, wrapping at the view edges.
- `PIXIE-TASK-034` — **Status:** done — Keys for attract or repel and for the count; triangles oriented by velocity; its `README.md`.
- `PIXIE-TASK-035` — **Status:** done — Verification: 1,000 boids at 60 updates per second, measured and recorded, and the steering checked against the README.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51); commit `8162c4d`, Release unless noted.

- Debug and Release build with zero warnings; `ctest` passes. The Debug sample
  runs with GL debug output on and logs no GL message.
- Real input through the operating system, with the saved last frame:
  - Attracting, the cursor held at client pixel (896, 288) for 7 seconds:
    the boids inside the ring gathered into a ball centered on the cursor.
  - Space pressed, the cursor held at (512, 360): the log reports
    "repelling", and the frame shows an empty disc the size of the ring
    around the cursor, with boids outside it.
  - Up pressed three times and Down once: the log reports 1,100, 1,200,
    1,300, then 1,200 boids.
- Measured with vsync on, 6-second runs, from the sample's log:

  | Boids | Frames per second | Updates per second | Step, average | Step, slowest |
  |---|---|---|---|---|
  | 1,000 | 100.0 | 59.4 to 60.0 | 0.105 to 0.107 ms | 0.195 ms |
  | 10,000 | 100 | 60 | 1.07 to 1.11 ms | 1.43 ms |
  | 50,000 | 100 | 60 | 5.1 to 5.2 ms | 6.6 ms |

  The completion target, 1,000 boids at 60 updates per second, holds with the
  step at under 1% of the 16.7 ms budget.
- `SubmitShapes` draws the World here for the first time: triangles oriented
  by velocity, interpolated, with wrapped boids not streaking across the view.
- Changed alongside: the ShapeRenderer's rings are now two pixels wide at any
  size (they were a fifth of the radius, too heavy for the pointer's ring).
- Not run: another machine, Linux, or a display with system scaling.
