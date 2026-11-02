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

**Last promotion decision:** 2026-11-02

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

### `PIXIE-TODO-053` — Inspector fields, options and interactions overhaul

**Status:** promoted (expansion required)
**Source:** owner promotion on 2026-10-31: review and overhaul all the Inspector fields and options, and how they interact. Promoted deliberately vague; the owner marked it expansion required.
**Accepted scope:** every field and option the editor's Inspector shows for engine components and settings, and the interactions between them, such as options that enable, disable, hide or override other fields, and fields that have no effect in a given mode. The review covers labels, units, ranges, defaults, tooltips and widget choice. The overhaul applies the decisions from that review. The expansion defines the inventory, the target rules and the order of work, and where changes land: engine metadata, the editor or PixieInspector.
**Dependencies and gates:** expansion required. No implementation starts until the inventory and the proposed rules have the owner's review and this entry has a concrete scope and completion criteria. Edits keep undo/redo, save/reopen and the development channel's Inspector paths, and existing scenes load unchanged. Native engine code never depends on the editor. Lighting fields added by PIXIE-TODO-052 join the review once they exist.
**Completion criteria:** to be defined by the expansion. At minimum, every reviewed field and option has a recorded decision, and the agreed changes are covered by Inspector qualification over the development channel.
**Breakdown:** none

### `PIXIE-TODO-054` — Field chaining and manual edit locks

**Status:** promoted (local integration ready; owner visual review pending)
**Source:** owner authorization on 2026-11-01, including chaining Shape and Collider dimensions on the same selected objects.
**Accepted scope:** selection-scoped numeric chains and direct-edit locks through PixieInspector's generic API; per-bind total-delta scrubbing and absolute view-value entry; cross-component displayed dimensions; grouped native history, cancellation and lifecycle invalidation. Local source adoption is opt-in; the default published dependency stays unchanged.
**Dependencies and gates:** Inspector owns the generic implementation. This bounded authorization does not unfreeze PIXIE-TODO-053. No commit, push or publication is authorized.
**Completion criteria:** isolated Debug/Release builds, native-backed regression evidence for propagation, locks, history and lifecycle, default-package compatibility, publish refusal in local mode, documented limits, and a delivery handoff to the originating chat. Owner visual acceptance is recorded separately.
**Breakdown:** none

### `PIXIE-TODO-055` — Entity input controls and C++ component authoring

**Status:** promoted (local implementation qualified; independent review pending)
**Source:** owner authorization on 2026-11-01: generic entity KeyboardControl/MouseControl and C++ components created through Add component, with Open code and normal-build adoption; design and explicit legacy adapter reviewed by the coordinating owner chat.
**Accepted scope:** ordered raw device events and enabled-aware entity queries/callbacks through Behaviour; explicit KeyboardControl2D compatibility adapter; preserving C++ generation, registration/source metadata, CMake integration, VS Code opening, rebuild/reopen and stable selection/unsaved state. No gameplay/action mapping in the generic controls.
**Dependencies and gates:** preserve existing uncommitted field interaction/New folder work and machine-local Inspector adoption settings. No commit, push or publication is authorized. Native ABI changes require an explicit version bump and rebuild diagnostics.
**Completion criteria:** runtime lifecycle/order/fixed-step/capture regressions; generated multi-component C++ code compiles and persists through scene/editor/player; failure/preservation fixtures for code/CMake paths; normal editor Debug/Release builds expose the feature; proportional native/editor validation and independent review.
**Breakdown:** [Entity input and C++ component authoring](TASKLIST.md#pixie-todo-055--entity-input-and-c-component-authoring)

### `PIXIE-TODO-056` — Editor and C++ authoring consolidation

**Status:** promoted (ordered blocks; later design and UX gates retained)
**Source:** owner promotion on 2026-11-02 of [PIXIE-PROPOSAL-017](docs/proposals/editor-authoring-consolidation.md), including the subsequent section 4.1 rework, selection-aware headers, and removal of extra controller prerequisites for Behaviour events.
**Accepted scope:** consolidate the delivered C++ workflow into usable create/attach/edit/build operations, minimal contextual templates and discoverable grouped Behaviours; provide callbacks without attaching KeyboardControl/MouseControl merely to unlock events; reproduce/correct viewport gesture defects; inventory Inspector fields and clarify presets; evaluate Session/viewport presentation; coordinate the separate World/Scene experiment; retain later diagnostics and editor-asset/runtime-resource manager work as gated blocks. Documentation is delivered with each slice.
**Dependencies and gates:** preserve PIXIE-TODO-055's delivered C++ capabilities, source/CMake ownership, IntelliSense and unsaved/selection handoff, and PIXIE-TODO-054's interaction contracts. PIXIE-TODO-053 retains its inventory/rules review gate; the corresponding block below does not authorize an unreviewed generic Inspector rewrite. Proposal 016 retains its own World/Scene design, Luna assignment and independent acceptance; this entry authorizes coordination, not implementing that backend incidentally. Blocks A/B can be assigned independently; later blocks require their stated inputs, not completion of a World rewrite. Promotion records the work and does not launch every block, authorize a permanent final toolbar layout, or authorize commit/push/publication.
**Completion criteria:** all numbered child tasks are done or explicitly dropped with reasons; one real normal-editor exercise demonstrates create/attach/code/IntelliSense/build/play/save/reopen, contextual component access, visible build/attachment state and predictable camera gestures; reviewed field/preset decisions and UI prototype receive owner acceptance; each implemented slice has candidate hashes, relevant regressions, independent review, normal Debug/Release adoption and a concise guide. Deferred manager/telemetry blocks must either receive their bounded designs and qualification or an explicit disposition before parent closure; no blanket completion while they remain open.
**Breakdown:** [Editor and C++ authoring consolidation](TASKLIST.md#pixie-todo-056--editor-and-c-authoring-consolidation)

## Completed

Sorted by stable TODO ID. Each record owns its historical scope, tasks and
verification. The code of 039 to 042 is in commits `9d022c1` and `414e40f`, with their
records in `a18fe6b`; this table does not imply independent acceptance.

| Entry | Scope | Completion date | Record |
|---|---|---|---|
| `PIXIE-TODO-001` | documentation foundation | 2026-10-11 | [Completion](docs/archive/todo-001-documentation-foundation-2026-10-11.md) |
| `PIXIE-TODO-002` | main loop and the hello-gl sample | 2026-10-11 | [Completion](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md) |
| `PIXIE-TODO-003` | PixieLib, GLM, and EnTT vendored | 2026-10-13 | [Completion](docs/archive/todo-003-vendor-pixielib-glm-entt-2026-10-13.md) |
| `PIXIE-TODO-004` | documentation verifier | 2026-10-13 | [Completion](docs/archive/todo-004-documentation-verifier-2026-10-13.md) |
| `PIXIE-TODO-005` | input, 2D camera, and instanced shapes | 2026-10-13 | [Completion](docs/archive/todo-005-input-camera-shapes-2026-10-13.md) |
| `PIXIE-TODO-006` | runtime module: World, entities, Behaviours, and phases | 2026-10-13 | [Completion](docs/archive/todo-006-runtime-module-2026-10-13.md) |
| `PIXIE-TODO-007` | anchor 1: boids steered by the user | 2026-10-13 | [Completion](docs/archive/todo-007-anchor-1-boids-2026-10-13.md) |
| `PIXIE-TODO-008` | spatial grid and anchor 2: flocks | 2026-10-13 | [Completion](docs/archive/todo-008-spatial-grid-flocks-2026-10-13.md) |
| `PIXIE-TODO-009` | particles and anchor 3: attractors | 2026-10-13 | [Completion](docs/archive/todo-009-particles-attractors-2026-10-13.md) |
| `PIXIE-TODO-010` | engine side for editor hosts: host loop, render target, reflection, and `capi` | 2026-10-13 | [Completion](docs/archive/todo-010-host-loop-reflection-capi-2026-10-13.md) |
| `PIXIE-TODO-011` | C# ImGui editor over the anchors | 2026-10-13 | [Completion](docs/archive/todo-011-imgui-editor-2026-10-13.md) |
| `PIXIE-TODO-012` | single source of field metadata | 2026-10-13 | [Completion](docs/archive/todo-012-single-source-metadata-2026-10-13.md) |
| `PIXIE-TODO-013` | scenario values, scene rebuild, and time control | 2026-10-13 | [Completion](docs/archive/todo-013-scenario-time-control-2026-10-13.md) |
| `PIXIE-TODO-014` | attractor size and death on contact | 2026-10-13 | [Completion](docs/archive/todo-014-attractor-size-2026-10-13.md) |
| `PIXIE-TODO-015` | particle systems and spawners apart | 2026-10-13 | [Completion](docs/archive/todo-015-particle-spawners-2026-10-13.md) |
| `PIXIE-TODO-016` | selection and editing in the viewport | 2026-10-13 | [Completion](docs/archive/todo-016-viewport-selection-2026-10-13.md) |
| `PIXIE-TODO-017` | spawners placed by hand in the particles game | 2026-10-13 | [Completion](docs/archive/todo-017-spawners-by-hand-2026-10-13.md) |
| `PIXIE-TODO-018` | save and load scenes | 2026-10-14 | [Completion](docs/archive/todo-018-scenes-2026-10-14.md) |
| `PIXIE-TODO-019` | projects, edit mode, and play mode in the engine | 2026-10-14 | [Completion](docs/archive/todo-019-projects-2026-10-14.md) |
| `PIXIE-TODO-020` | new and open projects in the editor | 2026-10-14 | [Completion](docs/archive/todo-020-editor-projects-2026-10-14.md) |
| `PIXIE-TODO-021` | code in a project | 2026-10-14 | [Completion](docs/archive/todo-021-project-code-2026-10-14.md) |
| `PIXIE-TODO-022` | samples as projects | 2026-10-14 | [Completion](docs/archive/todo-022-samples-projects-2026-10-14.md) |
| `PIXIE-TODO-023` | editor polish, the editor's icon, and a viewport menu | 2026-10-14 | [Completion](docs/archive/todo-023-editor-polish-2026-10-14.md) |
| `PIXIE-TODO-024` | entity snapshots, scene-ID lookup, and names in the engine | 2026-10-14 | [Completion](docs/archive/todo-024-snapshots-2026-10-14.md) |
| `PIXIE-TODO-025` | moving, undo and redo, duplicating, deleting, and renaming in the editor | 2026-10-15 | [Completion](docs/archive/todo-025-authoring-2026-10-15.md) |
| `PIXIE-TODO-026` | continuous integration | 2026-10-15 | [Completion](docs/archive/todo-026-ci-2026-10-15.md) |
| `PIXIE-TODO-027` | images, sprites, and text fields in the engine | 2026-10-15 | [Completion](docs/archive/todo-027-sprites-engine-2026-10-15.md) |
| `PIXIE-TODO-028` | sprites in the editor, and a sprites sample | 2026-10-15 | [Completion](docs/archive/todo-028-sprites-editor-2026-10-15.md) |
| `PIXIE-TODO-029` | paths, list fields, and points of a sprite | 2026-10-15 | [Completion](docs/archive/todo-029-paths-2026-10-15.md) |
| `PIXIE-TODO-030` | a screen layer, text, player saves, and fragment metadata | 2026-10-15 | [Completion](docs/archive/todo-030-screen-text-2026-10-15.md) |
| `PIXIE-TODO-031` | text in the scene | 2026-10-15 | [Completion](docs/archive/todo-031-scene-text-2026-10-15.md) |
| `PIXIE-TODO-032` | the logic sandbox, stage 1: light the lamp | 2026-10-15 | [Completion](docs/archive/todo-032-logic-sandbox-stage-1-2026-10-15.md) |
| `PIXIE-TODO-033` | the logic sandbox, stage 1.5: saves and templates | 2026-10-16 | [Completion](docs/archive/todo-033-logic-sandbox-saves-templates-2026-10-16.md) |
| `PIXIE-TODO-034` | the logic sandbox, stage 1 fixes: selecting and removing wires | 2026-10-16 | [Completion](docs/archive/todo-034-logic-sandbox-wire-fixes-2026-10-16.md) |
| `PIXIE-TODO-035` | the logic sandbox: box selection and a toolbar | 2026-10-16 | [Completion](docs/archive/todo-035-logic-sandbox-toolbar-2026-10-16.md) |
| `PIXIE-TODO-036` | the logic sandbox, stage 2: chips and an 8-bit adder | 2026-10-16 | [Completion](docs/archive/todo-036-logic-sandbox-chips-adder-2026-10-16.md) |
| `PIXIE-TODO-037` | drawing only what is on screen | 2026-10-16 | [Completion](docs/archive/todo-037-view-culling-2026-10-16.md) |
| `PIXIE-TODO-038` | reading many small sets of entities into a large World | 2026-10-17 | [Completion](docs/archive/todo-038-scene-index-2026-10-17.md) |
| `PIXIE-TODO-039` | native-player F3 rates and F3+C command bar | 2026-10-20 | [Completion](docs/archive/todo-039-player-debug-2026-10-20.md) |
| `PIXIE-TODO-040` | generic CPU telemetry and active LogicSandbox integration | 2026-10-22 | [Completion](docs/archive/todo-040-telemetry-2026-10-22.md) |
| `PIXIE-TODO-041` | collision and character movement foundation | 2026-10-22 | [Completion](docs/archive/todo-041-collision-movement-2026-10-22.md) |
| `PIXIE-TODO-042` | scene collision and character authoring | 2026-10-22 | [Completion](docs/archive/todo-042-editor-movement-2026-10-22.md) |
| `PIXIE-TODO-043` | rigid body dynamics, authoring and character/body coupling | 2026-10-31 | [Completion](docs/archive/todo-043-rigid-body-dynamics-2026-10-31.md) |
| `PIXIE-TODO-044` | particles in scenes without code | 2026-10-26 | [Completion](docs/archive/todo-044-particles-without-code-2026-10-26.md) |
| `PIXIE-TODO-045` | particle affectors in scenes | 2026-10-26 | [Completion](docs/archive/todo-045-particle-affectors-2026-10-26.md) |
| `PIXIE-TODO-046` | spawner distribution and particle shapes | 2026-10-26 | [Completion](docs/archive/todo-046-spawner-distribution-shapes-2026-10-26.md) |
| `PIXIE-TODO-047` | each particle carries its look | 2026-10-26 | [Completion](docs/archive/todo-047-particle-look-2026-10-26.md) |
| `PIXIE-TODO-048` | lightweight dynamic particles and native contact sparks room | 2026-10-27 | [Completion](docs/archive/todo-048-dynamic-contact-sparks-2026-10-27.md) |
| `PIXIE-TODO-049` | published Inspector 0.3.0 adoption in the editor | 2026-10-27 | [Completion](docs/archive/todo-049-inspector-030-adoption-2026-10-27.md) |
| `PIXIE-TODO-051` | shared native engine, manifest game export, local MCP and approved deterministic capture | 2026-11-01 | [Completion](docs/archive/todo-051-shared-engine-export-mcp-2026-11-01.md) |
| `PIXIE-TODO-052` | 2D lighting: lights, shadows, emission, particle light, authoring, budgets and optional effects | 2026-10-31 | [Completion](docs/archive/todo-052-2d-lighting-2026-10-31.md) |
