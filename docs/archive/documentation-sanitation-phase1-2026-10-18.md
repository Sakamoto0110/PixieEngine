# Current Planning Documentation: Sanitation Phase 1

**Document ID:** ARCHIVE-DOCUMENTATION-SANITATION-PHASE1-20261018

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** bounded sanitation of the current planning documents, with stable IDs and existing historical records preserved

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-074

**Type:** completion

**Origin:** PIXIE-TODO-039

**Author:** codex

**Reference date:** 2026-10-18

**Commits:** with this record (parent `2365251e7ea50fe730e937c03bdd404f46a86f40`); uncommitted at verification

**Reference commit:** `2365251e7ea50fe730e937c03bdd404f46a86f40`

**Original path:** ROADMAP.md (Current direction), TODO.md (completed pointers), and TASKLIST.md (empty open-breakdown summary)

**Current state:** [ROADMAP.md](../../ROADMAP.md#current-direction), [TODO.md](../../TODO.md), and [the documentation policy](../governance/documentation-policy.md)

## Scope and authority

The owner requested a first documentation sanitation phase. The repository's
documentation policy and schema remain the format and authority owners.
The task was performed on branch `codex/pixie-docs-phase1`, in a separate
worktree at the reference commit. No planning policy, document vocabulary,
module boundary, product direction, or ID scheme was replaced.

VAW `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`, version
0.2.0.0, resolved the machine-local `pixieengine` profile, revision 1.
Common compatibility and the `documentation` route returned
`bound_scoped`. The build, capi, editor, engine, runtime, and samples routes
were not selected or qualified. This establishes scoped binding only.

The owner excluded ROADMAP-PROMOTIONS-PENDING.md from sanitation. It was not
copied into this worktree, and no design or promotion from it was
adopted. CLAUDE-SESSION-REPORT.md was read only as a handoff observation and
was also left unchanged. Neither untracked file becomes a repository
dependency or a policy source.

## Entry as completed

### `PIXIE-TODO-039` — Sanitize current planning documentation, phase 1

**Status:** completed

**Source:** owner request on 2026-10-18 to begin the first documentation sanitation phase, preserving the existing policy and excluding ROADMAP-PROMOTIONS-PENDING.md.

**Accepted scope:** keep direction in ROADMAP.md, turn completed TODO entries into ordered pointers, simplify the empty task scheduler, clarify proposal and archive routing, and correct stale schema-index statements. Preserve stable IDs, accepted guardrails, existing archive bodies, code, builds, and sample saves. ROADMAP-PROMOTIONS-PENDING.md and CLAUDE-SESSION-REPORT.md are input-only and are not edited, moved, or incorporated into a new planning model.

**Dependencies and gates:** the documentation route resolves for pixieengine/1; work stays on the owner's requested separate branch; no dependency adoption, implementation promotion, commit, or push is included.

**Completion criteria:** the documentation verifier passes; all earlier completion pointers and proposal states are retained; roadmap guardrails and historical records are preserved; excluded files retain their observed hashes; changes contain documentation only and state the evidence limits.

**Breakdown:** none

## Reconciliation

- ROADMAP.md keeps consumer anchors, accepted boundaries, open directions,
  guardrails, and freezes. Its old sequence of implementation blocks is
  preserved below as history.
- TODO.md retains every earlier completion target and date, ordered by the
  existing TODO IDs, plus the pointer for this documentation operation.
- TASKLIST.md routes completed breakdowns to their original parent records.
  Existing task IDs and archive records are retained.
- The proposal router describes ideas and outcomes without interpreting a
  promoted proposal as active work.
- The schema index identifies the existing verifier and the limits of
  mechanical checking. Its obsolete pre-code description was corrected.

The old archive router remains an ID-ordered table, with one appended record.
Decision, promotion, and completion records retain their existing meanings.
Changing the planning hierarchy, renumbering tasks, and reclassifying old
records are outside this phase.

## Roadmap chronology at the reference baseline

The following wording is the former Current direction section at the
reference commit. Relative links were rebased from ROADMAP.md into this
archive directory. Dates, claims, order, and wording are historical; they
do not assert current behavior or executable scheduling.

Original ROADMAP.md Git blob: `f41d88a6452b914f9046646d9fa2e098d6fe2b5c`.
SHA-256 of the extracted section before link rebasing (UTF-8, LF):
`5f2ec7fdc04f236cc389e2bb761562c9f82b75f3d661d0e1b26170e6aacefa5a`.

Decided on 2026-10-13
([decision record](../archive/object-model-and-editor-decisions-2026-10-13.md)):
three anchor scenarios drive the first cut, each with user interaction and an
editor: boids steered by the user, flocks, and particle systems with
attractors ([PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md)).
The work is scheduled in [`TODO.md`](../../TODO.md) as blocks, simple and
foundational work first, each broken into tasks in
[`TASKLIST.md`](../../TASKLIST.md):

1. Dependencies: PixieLib, GLM, and EnTT vendored (PIXIE-TODO-003).
2. A documentation verifier (PIXIE-TODO-004).
3. Input, a 2D camera, and instanced shapes (PIXIE-TODO-005).
4. The `runtime` module: World, entities, Behaviours, and update phases
   (PIXIE-TODO-006).
5. Anchor 1, boids (PIXIE-TODO-007).
6. A spatial grid and anchor 2, flocks (PIXIE-TODO-008).
7. Particles and anchor 3, attractors (PIXIE-TODO-009).
8. The engine side for editor hosts: host loop, render target, reflection, and
   `capi` (PIXIE-TODO-010).
9. The C# ImGui editor over the three anchors (PIXIE-TODO-011).

All nine blocks were completed on 2026-10-13: the three anchors run as
players and in the editor (see [`TODO.md`](../../TODO.md#completed) and the
[archive](../archive/README.md)). The owner's review of them, the same day,
added five more blocks
([decision record](../archive/editor-review-decisions-2026-10-13.md)):

10. A single source of field metadata in the native registry (PIXIE-TODO-012).
11. Scenario values, scene rebuild, and time control (PIXIE-TODO-013).
12. Attractor size and death on contact (PIXIE-TODO-014).
13. Particle systems and spawners apart (PIXIE-TODO-015).
14. Selection and editing in the viewport (PIXIE-TODO-016).
15. Spawners placed by hand in the particles game (PIXIE-TODO-017), asked for
    by the owner after the review.

Blocks 10 to 15 were completed on 2026-10-13. On 2026-10-14 the owner chose
the next phase
([decision record](../archive/projects-and-scenes-decisions-2026-10-14.md)):
games as project folders created and opened from the editor, together with
scene serialization ([PIXIE-PROPOSAL-005](../proposals/projects-and-scenes.md)):

16. Saving and loading scenes (PIXIE-TODO-018).
17. Projects, edit mode, and play mode in the engine (PIXIE-TODO-019).
18. New and open projects in the editor (PIXIE-TODO-020).
19. Code in a project (PIXIE-TODO-021).
20. Samples as projects (PIXIE-TODO-022).
21. A centered start screen, the editor's icon, a viewport menu, and view-to-world conversion
    (PIXIE-TODO-023), asked for by the owner after seeing the editor.

Blocks 16 to 21 were completed on 2026-10-14: scene entities are saved and loaded, the engine opens
project folders and edits and plays their scenes, the editor makes and opens projects, a project can
have code of its own, which the editor builds against its engine, and the three anchor samples are
projects of that kind.

Next, the owner chose authoring in the viewport, before sprites and exporting a game
([PIXIE-PROPOSAL-006](../proposals/authoring-in-the-viewport.md),
[decision record](../archive/authoring-decisions-2026-10-14.md)):

22. Entity snapshots, scene-ID lookup, and names in the engine (PIXIE-TODO-024).
23. Moving, undo and redo, duplicating, deleting, and renaming in the editor (PIXIE-TODO-025).

Blocks 22 and 23 were completed on 2026-10-15: a scene is built in the editor with the mouse, and
every change to it can be undone. The continuous integration followed (PIXIE-TODO-026).

Next, the owner chose sprites ([PIXIE-PROPOSAL-007](../proposals/sprites.md),
[decision record](../archive/sprites-decisions-2026-10-15.md)):

24. Images, sprites, and text fields in the engine (PIXIE-TODO-027).
25. Sprites in the editor, and a sprites sample (PIXIE-TODO-028).

Blocks 24 and 25 were completed on 2026-10-15: a scene draws PNG images, placed in the editor by
dropping them on the viewport, and a project without code plays in the engine's generic player.
Next, the owner proposed a first game on the engine, a sandbox of logic circuits
([decision record](../archive/logic-sandbox-decisions-2026-10-15.md)), recorded as three
unpromoted proposals: what the engine needs for it
([PIXIE-PROPOSAL-008](../proposals/interactive-2d-games.md)), the game itself
([PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md)), and one shared engine library with
exporting a game ([PIXIE-PROPOSAL-010](../proposals/shared-engine-and-export.md)). The owner
promoted the engine's part first:

26. Paths, list fields, and points of a sprite (PIXIE-TODO-029).
27. A screen layer, text, player saves, and fragment metadata (PIXIE-TODO-030).

Block 26 was completed on 2026-10-15: the engine draws lines with rounded corners and a moving fill,
and the editor edits lists of points. Block 27 was completed the same day: a game draws its interface
and text on a screen layer, keeps its own files in a user data folder, and saves sets of entities
with metadata. PIXIE-PROPOSAL-008 is carried out; the logic sandbox
([PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md)) waits on the owner's promotion. Before it, the
owner promoted text in the World as data
([PIXIE-PROPOSAL-011](../proposals/scene-text.md); screen text stays code):

28. Text in the scene (PIXIE-TODO-031).

Block 28 was completed on 2026-10-15: text is part of a scene, saved, picked, and drawn among the
sprites, and the editor treats fonts as assets. Then the owner promoted the logic sandbox
([PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md)), the engine's first game, in a project folder
of its own:

29. The logic sandbox, stage 1: light the lamp (PIXIE-TODO-032).
30. The logic sandbox, stage 1.5: saves and templates (PIXIE-TODO-033).

Block 29 was completed on 2026-10-15: on the engine as it is, a player places gates, wires them
through bends, and lights a lamp, in a project of its own with its own tests. After playing it, the
owner put fixes to stage 1 before block 30
([promotion record](../archive/logic-sandbox-fixes-promotion-2026-10-16.md)):

31. The logic sandbox, stage 1 fixes: selecting and removing wires (PIXIE-TODO-034), before block 30.

Block 31 was completed on 2026-10-16: wires and their bends are selected and removed, and a right
click on a wired input removes its wire. Then the owner asked for a box selection and a toolbar,
before the templates of block 30
([promotion record](../archive/logic-sandbox-toolbar-promotion-2026-10-16.md)):

32. The logic sandbox: box selection and a toolbar (PIXIE-TODO-035).

Block 32 was completed on 2026-10-16: a box selects pieces and their wires, the selection is copied
and pasted with new IDs, and a toolbar deletes, copies, pastes, saves, and loads.

Block 30 was completed on 2026-10-16: the circuit is saved and loaded, and parts of it become
templates, placed from the hotbar's Templates page. PIXIE-PROPOSAL-009 is carried out.

Then the owner promoted a new anchor for the game
([promotion record](../archive/logic-sandbox-adder-promotion-2026-10-16.md)):

33. The logic sandbox, stage 2: chips and an 8-bit adder (PIXIE-TODO-036).

Block 33 was completed on 2026-10-16: templates come back as chips, boxes with labelled pins that
open by zoom down to their gates, and an 8-bit ripple adder of full-adder chips of half-adder chips
adds in the game's start scene.

Then the owner asked for drawing to skip what is off screen, as the zoom grows
([promotion record](../archive/view-culling-promotion-2026-10-16.md)):

34. Drawing only what is on screen (PIXIE-TODO-037).

Block 34 was completed on 2026-10-16: a session submits only the sprites, texts, paths, and shapes
that may show on screen.

Then the logic sandbox's calculator, thousands of chips each read as a small set of entities, took
minutes to load, nearly all of it in `ReadEntities`, and the owner asked for it to load fast
([promotion record](../archive/scene-index-promotion-2026-10-17.md)):

35. Reading many small sets of entities into a large World (PIXIE-TODO-038).

Block 35 was completed on 2026-10-17: the World keeps its scene entities by scene ID, so reading a set
of entities costs what the set holds, not the World's size.

Later, after the logic sandbox, and not promoted yet: the owner observed on 2026-10-16 that a
particle system made in a blank project does not seem to work, and asked for it to be investigated
toward the end ([decision record](../archive/particles-in-blank-projects-postponed-2026-10-16.md)).
An unconfirmed lead: only the particles sample adds the particle step to its World
(`AddParticleStep`, in `samples/particles/src/game/ParticlesScene.cpp`), and a project without code
runs on the generic library, which does not; if that holds, its particle systems never step.

The anchor blocks supersede the vertical slice
of [PIXIE-PROPOSAL-003](../proposals/vertical-slice.md), with anchor 1 as the
first scenario. The `platform` and `render` boundaries stay undecided; their
code grows in the provisional engine library until they are.

## Evidence limits

The baseline checker passed for 118 Markdown files in the primary checkout;
the clean isolated worktree contained 116 because the two untracked handoff
files were not copied. Structural checks alone did not detect the misplaced
chronology or the stale statements.

No source implementation, public API, dependency, or build configuration was
changed. Native builds, application runs, interactive checks, and historical
screenshots or logs were not rerun or recovered. Older acceptance claims
remain bounded to their own records; this phase grants no new runtime,
performance, or independent acceptance.

Verification results for this change are appended after the checks run.

## Verification on 2026-10-18

- Documentation: `python -B scripts/check_docs.py` checked 117 Markdown files
  in the isolated worktree and reported 0 problems. The existing checker was
  not modified.
- Diff hygiene: `git diff --check` reported no whitespace errors.
- Scope: the change set contains exactly eight Markdown paths. No source,
  build configuration, dependency, sample, or save path was changed.
- Conservation against the reference commit: all 38 previous completion
  targets and dates are retained; their IDs are ordered and unchanged.
  The original roadmap prefix, guardrails, freezes, and goals are unchanged.
  The extracted chronology retains its wording and link destinations.
- History: all 73 earlier archive records are unchanged. The archive router
  retains its earlier rows and appends only PIXIE-ARCHIVE-074.
- Proposals: all 12 earlier proposal rows, states, and outcomes are unchanged.
  Relative links in the changed documents were also checked against actual
  directory-entry casing, separately from Windows' case-insensitive lookup.
- Exclusions: SHA-256 checks of the two primary-checkout files matched their
  initial observations. ROADMAP-PROMOTIONS-PENDING.md remained
  `48a80859917788b027c5d5d6407e85c455b0061c50031de00c5235f075162259`;
  CLAUDE-SESSION-REPORT.md remained
  `0ed31d37ac73d09d4cfb91e92533821d5cde28cd39941e7f8e4af8d019b8d1a8`.
- The supplemental conservation comparison initially removed a leading
  newline while extracting the snapshot for comparison. Correcting that
  comparison made it pass without changing the historical text. These are
  author checks, not independent acceptance.
- No native build, automated runtime suite, application session, commit,
  push, or merge was performed for this documentation change.
