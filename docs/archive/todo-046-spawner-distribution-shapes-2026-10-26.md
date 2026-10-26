# Spawner Distribution and Particle Shapes Completed: PIXIE-TODO-046

**Document ID:** ARCHIVE-TODO-046-SPAWNER-DISTRIBUTION-SHAPES-20261026

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** staggered births and jitter in particle spawners, and per-system particle shapes

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-083

**Type:** completion

**Origin:** PIXIE-TODO-046 (PIXIE-TASK-190 and PIXIE-TASK-191)

**Author:** claude

**Reference date:** 2026-10-26

**Commits:** `e5a59f7` (promotion), `1fab8fe18179c090a1530c23d3ba74764ab39d11` (code)

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md#particles)

## Accepted scope

The owner observed that a spawner at a high rate stacks its particles on one
spot, so their interaction makes them explode, and asked for particle shapes.
Approved as proposed on 2026-10-26: each particle a spawner emits in a step gets
its own moment within the step; `ParticleSpawner.jitter` scatters starts with a
Gaussian offset, 0 by default; `ParticleStyle.shape` offers Disc, Square, Ring and
Streak per system. Per-spawner color, size and shape, and dynamic particles, stay
in the roadmap direction recorded in `76ca966`, with the owner's data design note.

## Task snapshot

**Parent:** PIXIE-TODO-046

- `PIXIE-TASK-190` — **Status:** done — Spawner: each particle's moment within the step, and `jitter`; `ParticleStyle.shape` drawn by the particle renderer, with the velocity as a vertex attribute; descriptors; tests.
- `PIXIE-TASK-191` — **Status:** done — Verification with real input in the editor (each shape, a high-rate Point spawner with interaction); the runtime reference.

## Delivered behavior

A particle emitted in a step is born when its share of the rate came due: it
starts at its birth point, which is its `previous` position, and ends the step
having moved and aged only for the part of the step it lived; at one particle per
step it is born at the step's end, so saved scenes keep their timing. `jitter`
offsets each start by a Gaussian of standard deviation `jitter / 2` per axis,
drawing random numbers only when it is above 0, so scenes without it keep their
sequences. `ParticleShape` lives in the engine's `ParticleStyle`, described as the
system's `shape`; the renderer reads the velocity as a fifth vertex attribute,
lengthens the quad behind a Streak by its speed times `kStreakSeconds` (0.05 s),
and the fragment shader covers a disc, a square, a ring with a hole of 0.6 of its
radius, or a capsule fading toward the tail. `Particle` stays 32 bytes.

## Verification and evidence limits

The VAW binding resolved `bound_scoped` for runtime, editor, samples and
documentation (VAW commit `c9d81ba`, profile `pixieengine` revision 1).

- Debug and Release builds with zero warnings; CTest passed 15 of 15 in each;
  202 runtime tests in each.
- New tests: ten particles of one step at 600 per second end 0.9, 0.8, ... 0
  units out, all born at the spawner, each aged for the part of the step it
  lived; 4,000 starts with jitter 10 put 84 to 89% within 10 units and a mean
  distance between 6.0 and 6.55 (theory: 86.5% and 6.27); a stream of 3,000 per
  second with interaction slows to under half its mean speed with jitter 4; the
  shape round-trips through a scene. Broken on purpose and restored: without the
  staggering the spacing test fails; with jitter ignored the spread test fails;
  with the shape not described the round trip fails.
- Measured at 3,000 per second, speed 40 to 80, interaction radius 4, after 1.5
  s: the mean speed was 760 (fastest 1,475) with every particle of a step on one
  point, 539 (753) with the staggering alone, 308 with jitter 2, 204 (269) with
  jitter 4, and 160 with jitter 8; what remains is the pressure of 4,500 packed
  particles, which the interaction is meant to have.
- Real input in the editor (computer use), on a test project with one system per
  shape and two dense spawners: on Play the four shapes drew as a disc, a square,
  a hollow ring and streaks along their motion; the dense spawner without jitter
  spread a haze far past its emission speed, the one with jitter 4 a dense core
  and a tight cloud. The disc system's shape changed to Streak through the
  inspector's choice, and Stop and Ctrl+S saved it.

Machine-local screenshots and the test scene are under
`.local/validation/spawner-distribution-shapes-2026-10-26/`. A slow Streak (20 to
45 units per second against a radius of 6) is barely longer than a disc, as its
length follows the speed. No other platform or push is claimed.

## Remaining boundaries

The Streak's tail length is fixed at 0.05 s of motion, and a Ring's hole at 0.6 of
its radius; neither is a field. Shape, color and size stay per system until the
per-spawner direction is expanded.
