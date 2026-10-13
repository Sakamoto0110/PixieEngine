# PIXIE-TODO-009: Particles and Anchor 3, Attractors

**Document ID:** ARCHIVE-TODO-009-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-009, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-021

**Type:** completion

**Origin:** PIXIE-TODO-009 (PIXIE-TASK-040 to PIXIE-TASK-045)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `16112a26fd57994a3882275411ba9e8d12275e4c`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`samples/particles/README.md`](../../samples/particles/README.md) and the [runtime reference](../modules/runtime/REFERENCE.md#particles)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** [PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md), anchor 3

**Accepted scope:** in the `runtime` module: a `ParticleEmitter` component that owns a contiguous particle pool (emission rate, lifetime, capacity); batch affectors that receive the whole pool; `Attractor` entities and an affector that pulls particles toward them; a short-range particle-to-particle interaction through a spatial grid; particles drawn as instanced circles straight from the pool. A `particles` sample places and drags attractors with the mouse and switches between 1, 10, 100, and thousands of particles.

**Dependencies and gates:** 1. PIXIE-TODO-006; 2. PIXIE-TODO-008 (spatial grid)

**Completion criteria:** (1) 100,000 particles against up to 16 attractors, and 10,000 particles with short-range interaction, each at 60 updates per second, measured and recorded; (2) orbits form around an attractor as the `README.md` describes; (3) zero warnings in Debug and Release.

**Commits:** `16112a26fd57994a3882275411ba9e8d12275e4c` (particles in the engine and the runtime, tests, and the sample); the README and the runtime reference are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-040` — **Status:** done — `ParticleEmitter`: a contiguous pool with emission rate, lifetime, and capacity, updated by an engine system.
- `PIXIE-TASK-041` — **Status:** done — Batch affectors that receive the whole pool; `Attractor` entities and the affector that pulls particles toward them.
- `PIXIE-TASK-042` — **Status:** done — Short-range particle-to-particle interaction through a spatial grid.
- `PIXIE-TASK-043` — **Status:** done — Particles drawn as instanced circles straight from the pool.
- `PIXIE-TASK-044` — **Status:** done — `particles` sample: place and drag attractors with the mouse, switch between 1, 10, 100, and thousands of particles; its `README.md`.
- `PIXIE-TASK-045` — **Status:** done — Verification: 100,000 particles against up to 16 attractors, and 10,000 with interaction, at 60 updates per second, measured and recorded.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51); commit `16112a2`, Release unless noted.

- Debug and Release build with zero warnings; `ctest` passes in both, with 23
  runtime tests. The Debug sample runs with GL debug output on and no GL
  message.
- Particle tests, each seen failing on a deliberately broken copy, restored
  afterwards: an inverse-distance pull instead of inverse-square breaks the
  circular orbit (which otherwise stays within 3% of its radius over a full
  turn at 60 steps per second); applying the interaction's changes in place
  makes it depend on particle order; expiring particles late breaks the
  lifetime test.
- Measured with vsync on, 8-second runs, from the sample's log:

  | Particles | Attractors | Interaction | Frames per second | Step, average | Step, slowest |
  |---|---|---|---|---|---|
  | 100,000 | 16 | off | 100 | 4.3 to 4.6 ms | 5.5 ms |
  | 10,000 | 1 | on | 100 | 1.78 to 1.81 ms | 2.3 ms |
  | 10,000 | 16 | on | 100 | 9.4 to 9.5 ms | 11.2 ms |
  | 10,000 | 3 | off | 100 | 0.11 to 0.12 ms | 0.20 ms |

  Updates stay at 60 per second in every run. With vsync off, 100,000
  particles against 16 attractors draw at about 1,680 frames per second.
  Both completion targets hold within the 16.7 ms budget: 100,000 particles
  against 16 attractors, and 10,000 with short-range interaction.
- Real input through the operating system: a left press on empty space at
  client pixel (256, 180) and a drag by (+150, +60) added a second attractor
  (logged) and left it at (406, 240), with particles drawn toward it. A right
  click on the middle attractor removed it (logged: 0 attractors), and the 3
  key respawned 100 particles (logged).
- Seen in saved frames: with one attractor and the interaction off, a disc of
  particles that keeps its shape; with the interaction on, a puffier disc
  held together by the attractor, with a cleared core; with 16 attractors on a
  ring, a glowing ring of 100,000 particles.
- Tuned during the work: the interaction's defaults went from a radius of 6
  and a stiffness of 3,000 to 4 and 1,200, with damping 3. The first values,
  at the presets' density, blew the disc apart.
- Not run: rate emission in the sample (the presets place particles
  directly; rate emission is covered by the tests), another machine, Linux.
