# Owner's Review of the Anchors and the Editor

**Document ID:** ARCHIVE-EDITOR-REVIEW-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's change requests after reviewing the anchors and the editor, and the answers that shaped PIXIE-TODO-012 to PIXIE-TODO-016

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-025

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** with this record (parent `c0f68c0cf50cbaaae4fd2b59203ac0b6a427291b`)

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#current-direction) and [`TODO.md`](../../TODO.md)

## How the requests arrived

After reviewing what PIXIE-TODO-003 to PIXIE-TODO-011 built, the owner worked
with the session that maintains PixieInspector, which published PixieInspector
0.2.0 for it (`AddField` with its own readers and writers, display units, and
an arc editor) and relayed six requests, A to F, with the design of a single
source of metadata that the owner had approved. Claude proposed five blocks
with numbered questions; the owner answered them and added gestures and time
control.

## Requests

- **A.** A single source of field metadata, "even if it costs more": the
  native registry carries labels, tooltips, ranges, scrub, units, and the rest
  of PixieInspector's node options; the C API exports them; the editor builds
  inspectors from them with `AddField`, with no proxy class per component (the
  hand-written proxies also carried `[Inspector*]` attributes, against the
  owner's decision of 2026-10-12 that proxies and handles carry none). Range
  and scrub steps are in the displayed unit. C++26 reflection could generate
  the descriptors once MSVC supports it; the project stays on C++20.
- **B.** Tooltips with units, scrubbing, and angles in degrees for every
  field.
- **C.** Multi-selection: Ctrl and Shift clicks in the entity list, box
  selection in the viewport, and inspectors bound to several entities.
- **D.** Scenario values (counts and the like) apart from live values, with a
  button that rebuilds the scene with them.
- **E.** Particle systems and spawners apart, with emission shapes and
  direction modes, direction and spread edited together as an arc.
- **F.** Attractors with a size.

## Answers

Recorded in the working list of open questions, numbered 31 to 42:

- Block order: metadata, scenario values with time control, attractor size,
  spawners, selection.
- The C API goes to version 2: `pixie_field_info` and `pixie_field_text`, with
  no compatibility with version 1.
- Fields a project rewrites every step are shown read-only.
- The viewport gets a "Game" and a "Select" mode; the click gestures belong to
  Select.
- A scenario belongs to its Project, mirrored in a component that a rebuild
  reads back.
- An `Entity` field kind references other entities.
- **Attractor scale:** the pull stays `strength` (not "mass": that would edge
  toward simulating orbital mechanics, a goal for some day but not now), and
  the size is a separate value; a die-on-contact flag removes a particle that
  enters the attractor's body; without the flag, behaviour stays as it was.
- The viewport gizmo for direction and cone waits until after the spawners.
- Push the 22 commits of the first blocks first (done: `b73e76b..c0f68c0` on
  `origin/main`).
- **Added by the owner:** in Select mode, a left click selects an entity,
  Shift + left click adds to the selection, Ctrl + left click on empty space
  creates a random boid or a particle at rest, and Ctrl + left click on a boid
  or a particle deletes it. Particles are never selected; only entities
  (boids, attractors, spawners) are.
- **Added by the owner:** buttons that slow time down, speed it up, and pause;
  slowing down is the main goal, and speeding up stops at 1.25, 1.5, 1.75, and
  2 times. Claude chose the slower steps: 0.75, 0.5, 0.25, and 0.1.
