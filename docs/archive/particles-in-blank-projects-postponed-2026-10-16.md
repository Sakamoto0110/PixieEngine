# Particles in a Blank Project: Investigation Postponed

**Document ID:** ARCHIVE-PARTICLES-BLANK-PROJECT-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's observation that a particle system made in a blank project does not seem to work, put in the roadmap to be investigated late

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-062

**Type:** roadmap

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** with this record (parent `4912ad74dfacba1372649bea9312095107ea2105`)

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#current-direction)

The owner observed that making a particle system in a blank project, in the editor, does not seem
to work, and asked for the investigation to go in the roadmap, toward the end: after the logic
sandbox's blocks, and not promoted to `TODO.md`. Nothing was reproduced or changed.

A lead, read from the source at the parent commit and not confirmed by a run: the particle step is
added to a World by `AddParticleStep` (`runtime/src/ParticleStep.cpp`), which only the particles
sample calls (`samples/particles/src/game/ParticlesScene.cpp`). A project without code runs on the
generic library, `pixie-scene-capi`, whose project adds no step of its own; if that is the cause,
a blank project's particle systems and spawners never step, though they are saved and inspected.
The investigation starts by reproducing it, then checking that lead.
