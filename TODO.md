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

**Last promotion decision:** 2026-10-28

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

### `PIXIE-TODO-043` — Rigid body dynamics and authoring

**Status:** in-progress
**Source:** owner authorization on 2026-10-22 to begin the detailed physics specification.
**Accepted scope:** [PHY-001 through PHY-021](docs/rigid-body-physics-requirements.md), delivered through D0–D5. Lighting follows the physics consolidation gate; it is not implementation scope here.
**Dependencies and gates:** preserve standalone collision APIs, existing scene behavior and earlier dirty work (the TEST, TEST2 and tes projects were removed at the owner's request on 2026-10-26; individual tests live outside the repository). Pin the motor coupling algorithm before D4. Separate measured mechanical evidence from owner feel acceptance.
**Completion criteria:** body/world ownership, dynamics/materials/events, editor/lifecycle/migration and character/platform scenarios satisfy the specification's qualified matrix; unmet requirements remain explicit. No claim of all physics completion from a native body demo.
**Breakdown:** [TASKLIST.md](TASKLIST.md#pixie-todo-043--rigid-body-dynamics)

### `PIXIE-TODO-051` — Shared engine, game export and local MCP development channel

**Status:** in-progress
**Source:** owner promotion of [PIXIE-PROPOSAL-010](docs/proposals/shared-engine-and-export.md) on 2026-10-28, explicitly extended to [PIXIE-PROPOSAL-013](docs/proposals/dev-channel.md) and [PIXIE-PROPOSAL-014](docs/proposals/deterministic-capture.md) as the development-channel and deterministic-capture scope. The owner's extension supersedes the supplied implementation prompt's exclusion of that channel.
**Accepted scope:** one shared native engine containing Core/Platform/Render/App, Runtime and C API; directly linked game players and development-only game modules; shared MSVC runtime across all native targets with app-local runtime deployment; one native manifest-allowlisted export tool reused by the editor; a separate local MCP development channel for Engine/Inspector operations; repeatable seeded, sized, fixed-step captures and approved snapshot scenarios. Nothing related to development operations enters game exports, publish or remote-release artifacts. The channel uses separate world.* and inspector.*/editor.* paths, descriptor-based discovery, session stepping, input/view operations and C API-backed snapshot/restore. Inspector writes reuse existing --set/--do handlers and EditRecorder/history, never a parallel writer. The player serves only world.*. Capture returns image content through MCP and writes only beneath the folder fixed when the channel opens. Entities use scene IDs and components/fields use names, never field indices. One client is served; queued commands run once per frame between Advance and Render, including while paused, and errors carry PixieResult plus pixie_last_error text. Channel save follows the owner's answer to question 2 of proposal 013 (2026-10-30): the editor's `editor.do save` reuses the existing save handler and is present in development builds, and the build flag `-p:PixieChannelSave=false` leaves it out; world.* and the player never save. Existing command-line behavior remains compatible.
**Dependencies and gates:** first prove module-defined EnTT components can be added, iterated and removed across the DLL boundary, and registered through ComponentTypes/read through the C API. Stop on type-ID disagreement and report mechanisms/costs instead of bypassing the gate. Only after it passes migrate production targets to /MD (/MDd for Debug), including third parties. Preserve the four-library dependency directions from the split baseline. Keep existing C API functions and semantics; additions raise the API version, and removals or semantic changes require a new owner decision. Obtain PixieLib shared-runtime evidence and agree the Inspector channel contract before dependent changes. Native engine code never depends on Inspector or the editor.
**Completion criteria:** all child tasks finish with the [qualification matrix](docs/shared-engine-export-validation.md), including project-code, both native presets/configurations, visual comparison, editor/module lifecycle, isolated exports, dependency closure, size measurements, and independently broken copies of all four development-artifact barriers. Qualify each accepted channel command group in memory and one real local IPC session, Inspector write/read/undo and refusal behavior, deterministic capture inputs and snapshot mismatch detection. Reference image updates need owner approval and a stated reason. The local MCP channel works through the agreed Engine/Inspector paths and is absent from publish/remote-release and exported runtime dependency closures. Record remaining LogicSandbox migration breaks without migrating it. No installer, additional export format, push or publication is authorized by this entry. PIXIE-TODO-043 remains independently open; this work does not complete physics or its owner acceptance.
**Breakdown:** [TASKLIST.md](TASKLIST.md#pixie-todo-051--shared-engine-export-and-local-mcp-development)

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
| `PIXIE-TODO-044` | particles in scenes without code | 2026-10-26 | [Completion](docs/archive/todo-044-particles-without-code-2026-10-26.md) |
| `PIXIE-TODO-045` | particle affectors in scenes | 2026-10-26 | [Completion](docs/archive/todo-045-particle-affectors-2026-10-26.md) |
| `PIXIE-TODO-046` | spawner distribution and particle shapes | 2026-10-26 | [Completion](docs/archive/todo-046-spawner-distribution-shapes-2026-10-26.md) |
| `PIXIE-TODO-047` | each particle carries its look | 2026-10-26 | [Completion](docs/archive/todo-047-particle-look-2026-10-26.md) |
| `PIXIE-TODO-048` | lightweight dynamic particles and native contact sparks room | 2026-10-27 | [Completion](docs/archive/todo-048-dynamic-contact-sparks-2026-10-27.md) |
| `PIXIE-TODO-049` | published Inspector 0.3.0 adoption in the editor | 2026-10-27 | [Completion](docs/archive/todo-049-inspector-030-adoption-2026-10-27.md) |
