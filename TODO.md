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

**Last promotion decision:** 2026-10-18

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

None. New work enters this queue only through an explicit owner decision.

## Completed

One pointer per completed entry, in stable TODO ID order. The linked record
owns the completed scope, its task breakdown, decisions, and verification at
that baseline; the pointer does not establish current runtime acceptance.

| Entry | Completed scope | Completion date |
|---|---|---|
| [`PIXIE-TODO-001`](docs/archive/todo-001-documentation-foundation-2026-10-11.md) | documentation foundation | 2026-10-11 |
| [`PIXIE-TODO-002`](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md) | main loop and the hello-gl sample | 2026-10-11 |
| [`PIXIE-TODO-003`](docs/archive/todo-003-vendor-pixielib-glm-entt-2026-10-13.md) | PixieLib, GLM, and EnTT vendored | 2026-10-13 |
| [`PIXIE-TODO-004`](docs/archive/todo-004-documentation-verifier-2026-10-13.md) | documentation verifier | 2026-10-13 |
| [`PIXIE-TODO-005`](docs/archive/todo-005-input-camera-shapes-2026-10-13.md) | input, 2D camera, and instanced shapes | 2026-10-13 |
| [`PIXIE-TODO-006`](docs/archive/todo-006-runtime-module-2026-10-13.md) | runtime module: World, entities, Behaviours, and phases | 2026-10-13 |
| [`PIXIE-TODO-007`](docs/archive/todo-007-anchor-1-boids-2026-10-13.md) | anchor 1: boids steered by the user | 2026-10-13 |
| [`PIXIE-TODO-008`](docs/archive/todo-008-spatial-grid-flocks-2026-10-13.md) | spatial grid and anchor 2: flocks | 2026-10-13 |
| [`PIXIE-TODO-009`](docs/archive/todo-009-particles-attractors-2026-10-13.md) | particles and anchor 3: attractors | 2026-10-13 |
| [`PIXIE-TODO-010`](docs/archive/todo-010-host-loop-reflection-capi-2026-10-13.md) | engine side for editor hosts: host loop, render target, reflection, and `capi` | 2026-10-13 |
| [`PIXIE-TODO-011`](docs/archive/todo-011-imgui-editor-2026-10-13.md) | C# ImGui editor over the anchors | 2026-10-13 |
| [`PIXIE-TODO-012`](docs/archive/todo-012-single-source-metadata-2026-10-13.md) | single source of field metadata | 2026-10-13 |
| [`PIXIE-TODO-013`](docs/archive/todo-013-scenario-time-control-2026-10-13.md) | scenario values, scene rebuild, and time control | 2026-10-13 |
| [`PIXIE-TODO-014`](docs/archive/todo-014-attractor-size-2026-10-13.md) | attractor size and death on contact | 2026-10-13 |
| [`PIXIE-TODO-015`](docs/archive/todo-015-particle-spawners-2026-10-13.md) | particle systems and spawners apart | 2026-10-13 |
| [`PIXIE-TODO-016`](docs/archive/todo-016-viewport-selection-2026-10-13.md) | selection and editing in the viewport | 2026-10-13 |
| [`PIXIE-TODO-017`](docs/archive/todo-017-spawners-by-hand-2026-10-13.md) | spawners placed by hand in the particles game | 2026-10-13 |
| [`PIXIE-TODO-018`](docs/archive/todo-018-scenes-2026-10-14.md) | save and load scenes | 2026-10-14 |
| [`PIXIE-TODO-019`](docs/archive/todo-019-projects-2026-10-14.md) | projects, edit mode, and play mode in the engine | 2026-10-14 |
| [`PIXIE-TODO-020`](docs/archive/todo-020-editor-projects-2026-10-14.md) | new and open projects in the editor | 2026-10-14 |
| [`PIXIE-TODO-021`](docs/archive/todo-021-project-code-2026-10-14.md) | code in a project | 2026-10-14 |
| [`PIXIE-TODO-022`](docs/archive/todo-022-samples-projects-2026-10-14.md) | samples as projects | 2026-10-14 |
| [`PIXIE-TODO-023`](docs/archive/todo-023-editor-polish-2026-10-14.md) | editor polish, the editor's icon, and a viewport menu | 2026-10-14 |
| [`PIXIE-TODO-024`](docs/archive/todo-024-snapshots-2026-10-14.md) | entity snapshots, scene-ID lookup, and names in the engine | 2026-10-14 |
| [`PIXIE-TODO-025`](docs/archive/todo-025-authoring-2026-10-15.md) | moving, undo and redo, duplicating, deleting, and renaming in the editor | 2026-10-15 |
| [`PIXIE-TODO-026`](docs/archive/todo-026-ci-2026-10-15.md) | continuous integration | 2026-10-15 |
| [`PIXIE-TODO-027`](docs/archive/todo-027-sprites-engine-2026-10-15.md) | images, sprites, and text fields in the engine | 2026-10-15 |
| [`PIXIE-TODO-028`](docs/archive/todo-028-sprites-editor-2026-10-15.md) | sprites in the editor, and a sprites sample | 2026-10-15 |
| [`PIXIE-TODO-029`](docs/archive/todo-029-paths-2026-10-15.md) | paths, list fields, and points of a sprite | 2026-10-15 |
| [`PIXIE-TODO-030`](docs/archive/todo-030-screen-text-2026-10-15.md) | a screen layer, text, player saves, and fragment metadata | 2026-10-15 |
| [`PIXIE-TODO-031`](docs/archive/todo-031-scene-text-2026-10-15.md) | text in the scene | 2026-10-15 |
| [`PIXIE-TODO-032`](docs/archive/todo-032-logic-sandbox-stage-1-2026-10-15.md) | the logic sandbox, stage 1: light the lamp | 2026-10-15 |
| [`PIXIE-TODO-033`](docs/archive/todo-033-logic-sandbox-saves-templates-2026-10-16.md) | the logic sandbox, stage 1.5: saves and templates | 2026-10-16 |
| [`PIXIE-TODO-034`](docs/archive/todo-034-logic-sandbox-wire-fixes-2026-10-16.md) | the logic sandbox, stage 1 fixes: selecting and removing wires | 2026-10-16 |
| [`PIXIE-TODO-035`](docs/archive/todo-035-logic-sandbox-toolbar-2026-10-16.md) | the logic sandbox: box selection and a toolbar | 2026-10-16 |
| [`PIXIE-TODO-036`](docs/archive/todo-036-logic-sandbox-chips-adder-2026-10-16.md) | the logic sandbox, stage 2: chips and an 8-bit adder | 2026-10-16 |
| [`PIXIE-TODO-037`](docs/archive/todo-037-view-culling-2026-10-16.md) | drawing only what is on screen | 2026-10-16 |
| [`PIXIE-TODO-038`](docs/archive/todo-038-scene-index-2026-10-17.md) | reading many small sets of entities into a large World | 2026-10-17 |
| [`PIXIE-TODO-039`](docs/archive/documentation-sanitation-phase1-2026-10-18.md) | Current planning documentation, phase 1 | 2026-10-18 |
| [`PIXIE-TODO-040`](docs/archive/documentation-sanitation-phase2-2026-10-18.md) | Technical documentation reconciliation, phase 2 | 2026-10-18 |
| [`PIXIE-TODO-041`](docs/archive/view-geometry-contract-investigation-2026-10-18.md) | Last-render geometry contract and bounded investigation | 2026-10-18 |
