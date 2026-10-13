# PIXIE-TODO-014: Attractor Size and Death on Contact

**Document ID:** ARCHIVE-TODO-014-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-014, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-029

**Type:** completion

**Origin:** PIXIE-TODO-014 (PIXIE-TASK-067 to PIXIE-TASK-069)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `bef16f9d5281a8555a8650ba7ee79eade2cc18c5`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [runtime reference](../modules/runtime/REFERENCE.md#particles)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13 (request F, answer 37)

**Accepted scope:** the attractor's pull stays `strength`, and its size becomes a separate value: a body radius, in world units, times its transform's scale, which the attractor's shape follows. The old `radius` becomes `reach` (the distance beyond which particles feel nothing). A new flag on the attractor, die on contact, removes a particle that enters its body. With the flag off, the behaviour is the same as before: size does not change the pull.

**Dependencies and gates:** 1. PIXIE-TODO-012

**Completion criteria:** (1) a test shows a particle entering a body removed with the flag on and passing through with it off; (2) the orbit test still holds; (3) the particles sample shows attractors sized by their body, editable in the inspector; (4) zero warnings.

**Commits:** `bef16f9d5281a8555a8650ba7ee79eade2cc18c5` (the attractor, the affector, the particle system, the sample, and tests); the runtime reference and the particles README are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-067` — **Status:** done — `Attractor`: `size` (body radius, times the transform's scale), `reach` (renamed from `radius`), and the die-on-contact flag; the particle system removes the particles an affector marks dead, in the same step.
- `PIXIE-TASK-068` — **Status:** done — Tests: contact with the flag on and off, and the orbit test unchanged.
- `PIXIE-TASK-069` — **Status:** done — The particles sample: attractor shapes follow their body; verification in the player and the editor.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51), .NET SDK 10.0.401; commit `bef16f9`.

- Debug and Release build with zero warnings; `ctest` passes 5 of 5 in both,
  with 32 runtime tests.
- New tests: a particle at rest inside a 20-unit body is removed with the flag
  and kept without it, and kept outside the body; the same particle at 30
  units is kept at scale 1 and removed at scale 2; a particle pulled straight
  in from 100 units is removed in the step it reaches a 15-unit body. The
  orbit test still keeps its radius within 3%.
- Broken on purpose and restored: without the removal right after the
  affectors, the three contact tests fail; with the flag ignored (every
  attractor killing), the flag test fails.
- The editor on particles: the center attractor selected, `size` set to 60 and
  "die on contact" set through the inspector; the saved frame shows its ring
  at 60 units with no particle inside, the Shape's size following the body,
  and the Attractor section with strength, softening, size, reach, and the
  flag.
- 100,000 particles against 16 attractors, interaction off, vsync on: the
  step went from 4.3 to 4.6 ms (PIXIE-ARCHIVE-021) to 5.4 to 5.7 ms, the cost of
  the contact test in the inner loop; still a third of the 16.7 ms budget.
- Not run: an attractor with a non-uniform scale in the editor (the body uses
  the larger of the two scales), Linux.
