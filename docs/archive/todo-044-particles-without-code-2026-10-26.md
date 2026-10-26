# Particles in Scenes Without Code Completed: PIXIE-TODO-044

**Document ID:** ARCHIVE-TODO-044-PARTICLES-WITHOUT-CODE-20261026

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the particle step in every session-built World, and the particle system's draw style in scenes and the inspector

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-081

**Type:** completion

**Origin:** PIXIE-TODO-044 (PIXIE-TASK-186 and PIXIE-TASK-187)

**Author:** claude

**Reference date:** 2026-10-26

**Commits:** `73475d1c20ddbd6f8795fceb4b1e622aa03b0320` (promotion), `59cb7e7135143ce2ce9289e82909b6db098ccabe` (code)

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md#particles), [editor README](../../editor/README.md), [RUNTIME-FINDING-001](../modules/runtime/FINDINGS.md#runtime-finding-001-particle-authoring-in-a-blank-project)

## Accepted scope

The owner asked how to make something emit particles. A scene without code
could not: nothing outside the particles sample installed the particle step, so
a scene's `ParticleSystem` and `ParticleSpawner` never ran, which is the cause
behind RUNTIME-FINDING-001. The owner authorized the proposed fix on 2026-10-26:
every World a session builds runs the particle step, once, and `ParticleSystem`
describes its draw style (color, size, additive, fade) so the inspector edits it
and the scene saves it. Affectors (attractors, interaction) stay code-only.
PIXIE-TODO-043's in-progress work and the untracked `samples/TEST`, `TEST2` and
`tes` folders were to be left untouched.

## Task snapshot

**Parent:** PIXIE-TODO-044

- `PIXIE-TASK-186` — **Status:** done — Runtime: the session installs the particle step in every World it builds, and `AddParticleStep` installs once per World; descriptor fields one level into a member; `ParticleSystem` describes color, size, additive and fade; tests.
- `PIXIE-TASK-187` — **Status:** done — Verification with real input in the editor (Create entity, Add component, Play) and in the particles sample; the runtime reference and the authoring documents.

## Delivered behavior

`ProjectSession::Build` calls `AddParticleStep` after the project's `Build`, so
the editor's Play and the player run a scene's particle systems and spawners
without project code. `AddParticleStep` marks the World in the registry's
context and installs the step once: a project that already adds it, such as the
particles sample, keeps a single step in its own place.

The descriptor builder gains `Field<&T::member, &Member::field>`, a value field
one level into a member. `ParticleSystem` describes `color`, `size`, `additive`
and `fade` through it, reaching into the engine library's `ParticleStyle`. Scenes
saved before read unchanged, since a missing field keeps its default; the
descriptor version stays 1.

Found during verification: the editor showed color fields as text only, Shape's
color included. It now gives them PixieInspector's color editor (channels and a
picker), which the scope's "the inspector edits it" needed.

## Verification and evidence limits

Machine: Windows 11, Visual Studio 2026 through its developer shell, .NET 10. The
VAW binding resolved `bound_scoped` for the routes runtime, editor, samples and
documentation (VAW commit `c9d81ba`, profile `pixieengine` revision 1).

- Debug and Release native builds with zero warnings, and the editor with zero
  warnings; CTest passed 15 of 15 in Debug and in Release; 196 runtime tests.
- New tests: a spawner read from a scene emits 10 particles in 10 steps at 60
  per second with `AddParticleStep` called twice; the round trip of every
  built-in component now covers a non-default style. Broken on purpose and
  restored: without the once-per-World guard the scene test counts 20; with the
  nested write doing nothing the round trip fails.
- Real input in the editor, through the desktop (computer use), on a new blank
  project: Create entity, renamed `particles`, Add component, ParticleSystem,
  color set to 255, 140, 40 and size to 3 by typing in the channels, Shape
  removed; Create entity, renamed `spawner`, Add component, ParticleSpawner,
  system chosen as `particles #0`, rate 120, lifetime 2, speed 40 to 120; Play.
  Orange particles left the spawner in every direction and faded; Stop and
  Ctrl+S wrote the style and the spawner's reference into the scene. Reopened in
  a new editor, Play showed the same particles after 320 steps.
- The particles sample, which adds the step itself: after 6 seconds it showed
  10,900 particles, its 10,000 plus 150 per second from its spawner, at 60 steps
  per second and about 2.0 ms per step, so its step ran once.

Machine-local screenshots and the saved scene are under
`.local/validation/particles-without-code-2026-10-26/`; they are local evidence,
not bundled with this record. No player run of a blank project, other platform,
or push is claimed.

## Remaining boundaries

Affectors are not saved: a blank project cannot yet make particles orbit an
attractor or push each other, which the sample does in code. Saving affectors
needs its own design and is not scheduled; RUNTIME-FINDING-001 records it.
