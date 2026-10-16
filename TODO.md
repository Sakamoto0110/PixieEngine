# PixieEngine Promoted Work

**Document ID:** GLOBAL-TODO

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** formally promoted work, gates, dependencies, and completion criteria

**Surface:** work-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

**Last promotion decision:** 2026-10-16

## Purpose and admission rule

This file is the only scheduler. It holds work that the owner has formally
promoted, with enough scope to execute and close it. It does not hold ideas
([`docs/proposals/`](docs/proposals/README.md)), direction
([`ROADMAP.md`](ROADMAP.md)), or completion narratives
([`docs/archive/`](docs/archive/README.md)).

An entry whose scope is large, or that decomposes into many small tasks, keeps
one entry here and its breakdown in [`TASKLIST.md`](TASKLIST.md). The rules are
in the [documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd).

Unchecked or `promoted` entries are not automatically authorized to start; an
entry may keep an explicit gate after promotion.

## Current promoted work

Promoted on 2026-10-15 from [PIXIE-PROPOSAL-009](docs/proposals/logic-sandbox.md) (a sandbox of
logic circuits), in this order ([promotion record](docs/archive/logic-sandbox-promotion-2026-10-15.md)).
The game lives in a project folder of its own, `C:\Users\Sakamoto\dev\LogicSandbox`, outside this
repository; this file tracks it because it is the engine's first game and its proof. Its first
block, PIXIE-TODO-032, and everything before it are completed; see below. On 2026-10-16 the owner
promoted fixes to stage 1, PIXIE-TODO-034, to come before PIXIE-TODO-033
([promotion record](docs/archive/logic-sandbox-fixes-promotion-2026-10-16.md)), completed the same day, and then
a box selection and a toolbar, PIXIE-TODO-035, before the templates of PIXIE-TODO-033
([promotion record](docs/archive/logic-sandbox-toolbar-promotion-2026-10-16.md)), completed the same day.

### `PIXIE-TODO-033` — The logic sandbox, stage 1.5: saves and templates

**Status:** promoted

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-009](docs/proposals/logic-sandbox.md), stage 1.5

**Accepted scope:** the whole circuit saved and loaded with Ctrl + S and Ctrl + O in the game's `UserDataFolder`; a box selection saved as a named template, its fragment's metadata holding the name, the inputs (input pins not fed from inside), and the final outputs (output pins feeding nothing inside); the hotbar's Templates page, switched with Tab, keys 1 to 9 for the first nine and 0 for a menu of all, a slot's hover showing name, inputs, and outputs; placing a template pastes its circuit with new IDs at the mouse. Out: a template as a closed box with pins of its own.

**Dependencies and gates:** 1. PIXIE-TODO-032; 2. PIXIE-TODO-034, put before it by the owner on 2026-10-16; 3. PIXIE-TODO-035, before its templates (PIXIE-TASK-150 and 151), on 2026-10-16

**Completion criteria:** (1) the game's tests: a circuit saved and loaded gives the same pieces, wires, and states; a template's metadata counts its inputs and outputs; a pasted template gets new IDs and keeps its inner wires; the Templates page lists saved templates; (2) zero warnings in Debug and Release; (3) the README describes saving and templates.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-033--the-logic-sandbox-stage-15-saves-and-templates)

## Completed

`PIXIE-TODO-001` (documentation foundation) completed on 2026-10-11. Its
entry, the policy review, and its verification are preserved in the
[archive record](docs/archive/todo-001-documentation-foundation-2026-10-11.md).

`PIXIE-TODO-002` (main loop and the hello-gl sample) completed on 2026-10-11.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md).

`PIXIE-TODO-003` (PixieLib, GLM, and EnTT vendored) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-003-vendor-pixielib-glm-entt-2026-10-13.md).

`PIXIE-TODO-004` (documentation verifier) completed on 2026-10-13. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-004-documentation-verifier-2026-10-13.md).

`PIXIE-TODO-005` (input, 2D camera, and instanced shapes) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-005-input-camera-shapes-2026-10-13.md).

`PIXIE-TODO-006` (runtime module: World, entities, Behaviours, and phases)
completed on 2026-10-13. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-006-runtime-module-2026-10-13.md).

`PIXIE-TODO-007` (anchor 1: boids steered by the user) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-007-anchor-1-boids-2026-10-13.md).

`PIXIE-TODO-008` (spatial grid and anchor 2: flocks) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-008-spatial-grid-flocks-2026-10-13.md).

`PIXIE-TODO-009` (particles and anchor 3: attractors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-009-particles-attractors-2026-10-13.md).

`PIXIE-TODO-010` (engine side for editor hosts: host loop, render target,
reflection, and `capi`) completed on 2026-10-13. Its entry, breakdown, and
verification are preserved in the
[archive record](docs/archive/todo-010-host-loop-reflection-capi-2026-10-13.md).

`PIXIE-TODO-011` (C# ImGui editor over the anchors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-011-imgui-editor-2026-10-13.md).

`PIXIE-TODO-012` (single source of field metadata) completed on 2026-10-13. Its
entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-012-single-source-metadata-2026-10-13.md).

`PIXIE-TODO-013` (scenario values, scene rebuild, and time control) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-013-scenario-time-control-2026-10-13.md).

`PIXIE-TODO-014` (attractor size and death on contact) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-014-attractor-size-2026-10-13.md).

`PIXIE-TODO-015` (particle systems and spawners apart) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-015-particle-spawners-2026-10-13.md).

`PIXIE-TODO-017` (spawners placed by hand in the particles game) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-017-spawners-by-hand-2026-10-13.md).

`PIXIE-TODO-016` (selection and editing in the viewport) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-016-viewport-selection-2026-10-13.md).

`PIXIE-TODO-018` (save and load scenes) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-018-scenes-2026-10-14.md).

`PIXIE-TODO-019` (projects, edit mode, and play mode in the engine) completed
on 2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-019-projects-2026-10-14.md).

`PIXIE-TODO-020` (new and open projects in the editor) completed on
2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-020-editor-projects-2026-10-14.md).

`PIXIE-TODO-023` (editor polish, the editor's icon, and a viewport menu) was
promoted and completed on 2026-10-14. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-023-editor-polish-2026-10-14.md).

`PIXIE-TODO-021` (code in a project) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-021-project-code-2026-10-14.md).

`PIXIE-TODO-022` (samples as projects) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-022-samples-projects-2026-10-14.md).

`PIXIE-TODO-024` (entity snapshots, scene-ID lookup, and names in the engine)
completed on 2026-10-14. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-024-snapshots-2026-10-14.md).

`PIXIE-TODO-025` (moving, undo and redo, duplicating, deleting, and renaming in
the editor) completed on 2026-10-15. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-025-authoring-2026-10-15.md).

`PIXIE-TODO-026` (continuous integration) was promoted and completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-026-ci-2026-10-15.md).

`PIXIE-TODO-027` (images, sprites, and text fields in the engine) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-027-sprites-engine-2026-10-15.md).

`PIXIE-TODO-028` (sprites in the editor, and a sprites sample) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-028-sprites-editor-2026-10-15.md).

`PIXIE-TODO-029` (paths, list fields, and points of a sprite) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-029-paths-2026-10-15.md).

`PIXIE-TODO-030` (a screen layer, text, player saves, and fragment metadata)
completed on 2026-10-15. Its entry, breakdown, and verification are preserved in
the [archive record](docs/archive/todo-030-screen-text-2026-10-15.md).

`PIXIE-TODO-031` (text in the scene) completed on 2026-10-15. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-031-scene-text-2026-10-15.md).

`PIXIE-TODO-032` (the logic sandbox, stage 1: light the lamp) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-032-logic-sandbox-stage-1-2026-10-15.md).

`PIXIE-TODO-034` (the logic sandbox, stage 1 fixes: selecting and removing wires)
completed on 2026-10-16. Its entry, breakdown, and verification are preserved in
the [archive record](docs/archive/todo-034-logic-sandbox-wire-fixes-2026-10-16.md).

`PIXIE-TODO-035` (the logic sandbox: box selection and a toolbar) completed on
2026-10-16. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-035-logic-sandbox-toolbar-2026-10-16.md).
