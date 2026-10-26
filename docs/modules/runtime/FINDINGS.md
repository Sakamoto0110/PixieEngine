# Runtime Findings

**Document ID:** RUNTIME-FINDINGS

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** unconfirmed runtime observations and investigation dispositions

**Surface:** findings

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

This register owns current unconfirmed observations. A finding is neither a
confirmed defect nor scheduled implementation. [TODO.md](../../../TODO.md)
owns any promoted investigation or fix.

## RUNTIME-FINDING-001: Particle authoring in a blank project

**Status:** confirmed

**Reported:** 2026-10-16; clarified by the owner on 2026-10-22

**Review not before:** 2026-10-23

**Scope:** the editor's component authoring and a generic project without game code

### Observation and evidence

The owner could not recreate the particles sample's effect using the editor in
a blank project. Two supplied screenshots show a running sample with a particles
entity and a separate blank project with ParticleSystem/ParticleSpawner components.
The blank-project screenshot shows Edit mode, Steps 0 and emission rate 0, but
the owner then tried changing values for approximately 15 minutes without success.
Those screenshot defaults do not explain the reported failure.

The images were visually inspected in the conversation and copied into local
review storage. This written description remains testimony; it is not a fresh
runtime reproduction or a promise that image files exist in a clone. The
[dated records](../../archive/roadmap-reconciliation-2026-10-22.md#particle-report-clarification-and-postponement)
preserve the owner clarifications and the original
[postponement](../../archive/particles-in-blank-projects-postponed-2026-10-16.md).

### Source observations and limits

- [Sample setup](../../../samples/particles/src/game/ParticlesScene.cpp) calls
  AddParticleStep and configures the pool and spawner in code.
- [SceneProject](../../../runtime/include/pixie/runtime/SceneProject.hpp) has an
  empty Build; [ProjectSession](../../../runtime/src/ProjectSession.cpp) loads
  scene components and calls the project's Build. It draws particles but does
  not install AddParticleStep in that generic path.
- [ParticleStep](../../../runtime/src/ParticleStep.cpp) registers particle updates
  explicitly. This is a lead for missing execution setup, not proof that it is
  the only authoring/runtime gap.

No native reproduction was performed during this documentation review. The root
cause, usable editor workflow and required correction remain unconfirmed.

### Investigation entry and desired outcome

Consult the prior implementation context before resuming. In a blank project,
create a system and spawner using only editor controls; set a valid system
reference and nonzero emission, enter Play and compare with the sample. Check
component defaults, pool/style/affector authoring and runtime-step installation.
Save/reopen and Play/Stop must preserve the intended configuration. The goal is
an editor-authorable effect; changing a screenshot's values alone is insufficient.
This investigation is deferred and has no active TODO entry.

### Outcome (2026-10-26)

Reproduced and confirmed: a session's World never ran the particle step, so a
blank project's spawner emitted nothing whatever its values. PIXIE-TODO-044
installs the step in every World a session builds and describes the system's
draw style. In a blank project, a system and a spawner made with Create entity
and Add component, with the system chosen and a nonzero rate and speed, emit
on Play, with the edited color and size, and again after save and reopen
([completion record](../../archive/todo-044-particles-without-code-2026-10-26.md)).
What remains is the sample's orbits: affectors (attractors, interaction) are
still added only in code, and saving them needs its own design; it is not
scheduled.
