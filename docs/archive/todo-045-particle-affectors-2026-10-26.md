# Particle Affectors in Scenes Completed: PIXIE-TODO-045

**Document ID:** ARCHIVE-TODO-045-PARTICLE-AFFECTORS-20261026

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the attraction and interaction affectors as scene components, and the particles sample moved onto them

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-082

**Type:** completion

**Origin:** PIXIE-TODO-045 (PIXIE-TASK-188 and PIXIE-TASK-189)

**Author:** claude

**Reference date:** 2026-10-26

**Commits:** `69e7a11` (promotion), `7388a23d640439ee71b49284762def524baa34f5` (code)

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md#particles), [particles README](../../samples/particles/README.md)

## Accepted scope

After PIXIE-TODO-044, a project without code could emit particles but not make
them orbit or interact, since affectors were added only in code. The owner asked
on 2026-10-26 for every affector to be fixed. Both runtime affectors became scene
components on a particle system's entity, `ParticleAttraction` and
`ParticleInteraction`, applied by the particle step after the affectors added in
code; Add component accepts them only on an entity with a `ParticleSystem`; the
particles sample moved onto them. At the owner's request the same day, the
untracked `samples/TEST`, `TEST2` and `tes` projects were removed (sent to the
Recycle Bin), and the PIXIE-TODO-043 gate no longer names them.

## Task snapshot

**Parent:** PIXIE-TODO-045

- `PIXIE-TASK-188` — **Status:** done — Runtime: `ParticleAttraction` and `ParticleInteraction`, described and applied by the particle step after code-added affectors; the attractor pull's scale; the Add component guard; tests.
- `PIXIE-TASK-189` — **Status:** done — The particles sample on the components, its scene rewritten; verification with real input in the editor and in the sample; the runtime reference, the sample README, and RUNTIME-FINDING-001.

## Delivered behavior

`ParticleAttraction` (enabled, scale) pulls the system's particles toward every
attractor, the scale multiplying each attractor's strength (0 nothing, negative
pushing away); `AttractorAffector` gained the same `scale`, 1 by default.
`ParticleInteraction` (enabled, radius, stiffness, damping) pushes the system's
overlapping particles apart. The step applies attraction, then interaction,
after the affectors added in code, keeping their grids and buffers in the
registry's context by system entity; a system with neither and no code affector
is pulled by nothing. Their admission rule names the missing `ParticleSystem`.

The particles sample adds no affector in code. Its `ParticleControls` gave way to
`ParticleInteraction`, which the I key toggles; its scene holds the pool's look,
attraction and interaction, which the code sets only for a World built without a
scene; `minPixels`, not described, stays in code.

## Verification and evidence limits

The VAW binding resolved `bound_scoped` for runtime, editor, samples and
documentation (VAW commit `c9d81ba`, profile `pixieengine` revision 1).

- Debug and Release native builds with zero warnings; CTest passed 15 of 15 in
  each, including the particles project opened through the C API; 199 runtime
  tests.
- New tests: one step after an attractor 100 units away, attraction pulls; it
  does nothing when disabled or absent, reverses exactly with scale -1, and
  doubles with scale 2. Two particles 1 unit apart move apart with interaction
  and stay at rest without it. Add component refuses both on an entity without a
  `ParticleSystem` and accepts them on one. The scene round trip carries both
  with non-default values. Broken on purpose and restored: without the step
  applying them, three checks fail; ignoring the scale fails two; accepting them
  anywhere fails the guard test.
- Real input in the editor (computer use) on the blank project of PIXIE-TODO-044:
  the system was given ParticleAttraction and ParticleInteraction through Add
  component; a new entity at x = 250 was given an Attractor, with the two
  affector components shown greyed in its list. On Play the particles bent
  toward the attractor; scale 6, typed while playing, drew the stream around it,
  and scale -6 pushed every particle away from it. Stop and Ctrl+S saved both
  components and the attractor.
- The particles sample in Release: the same disc and stream as before, 10,900
  particles after 6 seconds, 1.9 to 2.4 ms per step (2.0 ms before the change),
  and, through computer use, I turned the interaction off and on, in the panel
  and in the log.

Machine-local screenshots and the saved scene are under
`.local/validation/particle-affectors-2026-10-26/`. No other platform or push is
claimed.

## Remaining boundaries

Affectors written in code by a game remain code-only; only the two runtime
affectors have scene components. RUNTIME-FINDING-001 has nothing left open.
