# Scene Movement Authoring Completed: PIXIE-TODO-042

**Document ID:** ARCHIVE-TODO-042-EDITOR-MOVEMENT-20261022

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** scene collision and character authoring with reusable native services

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-080

**Type:** completion

**Origin:** PIXIE-TODO-042

**Author:** codex

**Reference date:** 2026-10-22

**Commits:** uncommitted working tree based on `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Current state:** [authoring guide](../editor-movement.md), [sample](../../samples/movement-lab/README.md)

## Accepted scope

The owner requested editor integration of the collision/movement foundation and
asked whether an editor could become part of a laboratory game. TODO-042 accepted
serializable scene components, native runtime installation, exclusive motion
ownership, authoring gizmos and shortcuts, a no-code sample, and lifecycle/consumer
verification. Existing movement APIs, dirty work and TEST/TEST2 projects were to be
preserved. Full in-game laboratory UI and undo-core extraction remain later work.

## Task snapshot

**Parent:** PIXIE-TODO-042

- `PIXIE-TASK-176` — **Status:** done — Authored components, synchronization and exclusive motion ownership.
- `PIXIE-TASK-177` — **Status:** done — Session lifecycle, reusable gizmos, diagnostics and editor shortcuts.
- `PIXIE-TASK-178` — **Status:** done — No-code scene, persistence/consumer checks and failure sensitivity.
- `PIXIE-TASK-179` — **Status:** done — Builds, editor visual evidence and documented remaining boundaries.

## Delivered behavior

Collider2D, CharacterMotor2D and optional KeyboardControl2D are reflected and saved
as ordinary scene components. ProjectSession installs the native scene service
for both editor and generic player. Edit synchronization does not advance time;
Play/Stop reconstructs transient collision/controller state. Motor entities bypass
legacy velocity/acceleration position integration. Geometry edits, removal,
configuration errors, retuning and external teleports are handled explicitly.
Borrowed scene queries can map current collider handles back to authored entities.

The editor offers Collider/Character creation, viewport placement, native edit
outlines and picking, generated inspector fields, undo/redo and diagnostics.
The movement-lab sample requires no game code. C API version **9** adds movement
diagnostics; existing game libraries must be rebuilt for this editor version.

## Verification and evidence limits

Final targeted runtime suites passed **122 tests with zero failed checks in each
of Debug and Release**. Full native Debug and Release builds passed, and the
Release C# editor build reported zero warnings/errors. Release CTest passed
**12/12 scenarios**, including movement authoring and a separate project consumer.
That full run preceded the final additive query-to-entity helpers and editor row
layout adjustment; subsequent final builds, both runtime suites and the targeted
C API movement fixture passed with those changes included.

Scripted native editor checks exercised creation, undo/redo, rename, save,
reopening, Play/Stop and invalid configuration diagnostics. Final screenshots were
visually inspected. A separate Play capture ran 161 steps (2.683 simulated seconds)
and showed the character resting on the floor. Input and jump behavior were checked
through the native C API fixture. A deliberately omitted motor caused the same
fixture to fail, demonstrating sensitivity to a missing integration component.
The versioned sample project and scene hashes remained unchanged by UI checks.

Machine-local logs, screenshots, plan and report are under
`.local/validation/editor-movement-2026-10-22/`. They are local evidence, not
portable proof bundled with this record. No human movement-feel acceptance,
cross-platform qualification, commit or push is claimed.

## Remaining boundaries

The native World, descriptors, movement services, queries, gizmos and scene
snapshots can support an in-game laboratory. Current editor gestures, shortcuts
and history remain in C#; an embeddable editing UI and explicit commit/reset/input
policies still require their own implementation. This delivery does not include
dynamic rigid bodies, triggers, one-way platforms, moving-platform attachment or
character-to-character collision. Motors require upright unit-scale transforms.
The existing particle-authoring investigation remains deferred and unchanged.
