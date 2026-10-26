# Each Particle Carries Its Look Completed: PIXIE-TODO-047

**Document ID:** ARCHIVE-TODO-047-PARTICLE-LOOK-20261026

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the 40-byte particle with its own color, size and shape, set by its spawner or its system

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-084

**Type:** completion

**Origin:** PIXIE-TODO-047 (PIXIE-TASK-192 and PIXIE-TASK-193)

**Author:** claude

**Reference date:** 2026-10-26

**Commits:** `7b77dd9` (promotion), `4a96567bdeec08d2d505934e3f48007cbffb5118` (code)

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md#particles), [ROADMAP](../../ROADMAP.md#accepted-direction-expansion-required)

## Accepted scope

PIXIE-TODO-046 delivered the particle shape in the system's style and recorded
"`Particle` stays 32 bytes", although the owner had meant the shape to live in the
particle's data, in the 40-byte layout of the roadmap's particle data design note
(`76ca966`); the divergence was not raised when the owner approved PIXIE-TODO-046.
On the owner's correction, approved on 2026-10-26: `Particle` grows to 40 bytes with
its look (color, half-float size, a flags byte holding the shape and a reserved
dynamic bit, a spare byte); one function fills every new particle; `Emit` gives
the system's look, which the style becomes; a spawner with `ownLook` gives its own;
the renderer reads the look per particle. The staggered births and jitter of
PIXIE-TODO-046 stay. Dynamic particles stay in the roadmap direction.

## Task snapshot

**Parent:** PIXIE-TODO-047

- `PIXIE-TASK-192` — **Status:** done — Engine: the 40-byte `Particle` with color, half-float size and flags, its filling function and half-float conversion, integer vertex attributes, and the renderer reading the look per particle; tests.
- `PIXIE-TASK-193` — **Status:** done — Runtime: `Emit` with the system's look, the spawner's own look, descriptors; tests; the cost measured before and after; verification with real input in the editor; the runtime reference.

## Delivered behavior

`Particle` (engine) holds, after its 32 bytes of motion and age, an RGBA8 `color`,
its radius as a half float (`ToHalf` rounds to nearest, ties to even, overflowing
to infinity and keeping subnormals; `FromHalf` is exact), a flags byte with the
shape in `kShapeBits` and `kDynamic` reserved, and a spare byte; `ParticleShape`
moved into `Particle.hpp`. `MakeParticle` builds every new particle.
`ParticleSystem::Emit` uses the style's color, size and shape, now the system's
default look, or a look given; a particle keeps its look for life, so editing the
style changes the particles born afterwards. `ParticleSpawner` gained `ownLook`,
`particleColor`, `particleSize` and `particleShape`, saved and edited like its
other fields; without `ownLook` saved scenes behave as before. The renderer reads
color (normalized bytes), size (`GL_HALF_FLOAT`) and flags (a new
`GlVertexArray::IntegerAttribute`) per instance; `minPixels`, `additive` and
`fade` stay uniforms of the pool's draw.

## Verification and evidence limits

The VAW binding resolved `bound_scoped` for runtime, editor, samples and
documentation (VAW commit `c9d81ba`, profile `pixieengine` revision 1).

- Debug and Release builds with zero warnings; CTest passed 15 of 15 in each;
  204 runtime tests in each.
- New tests: `sizeof(Particle)` is 40; half floats round-trip within one part in
  2,048, ties round to even, 1e6 becomes infinity, a subnormal survives;
  `MakeParticle` sets every field; in one pool, a spawner without its own look
  gives the system's red rings and one with it its green streaks, and a particle
  keeps its color after the style changes; the spawner's look round-trips through
  a scene. Broken on purpose and restored: a spawner ignoring `ownLook`, a
  `MakeParticle` dropping the shape, and half floats rounding ties up each fail
  their tests.
- Cost, measured on copies of the particles sample's project in Release without
  vsync, twice each: 100,000 particles without interaction went from about 2,230
  to 1,840 frames per second (a frame of 0.45 to 0.54 ms) and from 0.71 to 0.84
  ms to 0.84 to 0.88 ms per step; 10,000 with interaction stayed at 2.4 to 2.8
  ms per step and 3,300 to 4,150 frames per second, within the runs' spread.
- In the editor, on a test project with one particle system and four spawners:
  three with their own looks drew red squares, green rings and yellow streaks
  together; the fourth, on the system's look, was switched to its own orange
  rings through the inspector (by the editor's `--set`, which goes through the
  inspector, since a window outside this session's access covered the
  inspector), Play was pressed by a real click, and the saved scene keeps one
  system and the four looks. The particles sample looks as before.

Machine-local screenshots and the test scene are under
`.local/validation/particle-look-2026-10-26/`. No other platform or push is
claimed.

## Remaining boundaries

The dynamic bit is reserved and unused, and the spare byte is free; dynamic
particles, gravity and colliders stay in the roadmap direction, with the payload
and side-table rules of its design note.
