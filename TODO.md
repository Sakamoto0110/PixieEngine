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

**Last promotion decision:** 2026-10-22

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

### `PIXIE-TODO-046` — Spawner distribution and particle shapes

**Status:** promoted
**Source:** owner request on 2026-10-26: a spawner at a high rate stacks its particles on one spot, and their interaction makes them explode; particles should have a shape. Approved as proposed ("go ahead with TODO-046").
**Accepted scope:** (1) a spawner gives each particle it emits in a step its own moment within the step: the particle starts at its birth point and travels only the part of the step it lived, with its age to match, so a fast stream comes out continuous instead of in stacks; always on, no field. (2) `ParticleSpawner.jitter`, in world units: a Gaussian offset around the point the emission shape chose, most particles within `jitter` of it; 0 by default, so saved scenes do not change. (3) `ParticleStyle.shape`, per system, edited in the inspector and saved in scenes: `Disc` (today's), `Square`, `Ring` (hollow) and `Streak`, stretched behind the particle along its motion over the last 0.05 s. Per-spawner color, size and shape, and dynamic particles, stay in the roadmap direction.
**Dependencies and gates:** PIXIE-TODO-045; leave PIXIE-TODO-043's in-progress work untouched; `Particle` stays 32 bytes.
**Completion criteria:** (1) runtime tests show a step's particles spaced along their path, born at the spawner, with ages matching, and the jitter's spread; existing emission tests unchanged in meaning; (2) the shape round-trips through a scene; (3) each shape seen in the editor through real input, and a high-rate Point spawner with interaction no longer exploding; (4) zero warnings, and ctest passes in Debug and Release.
**Breakdown:** [TASKLIST.md](TASKLIST.md#pixie-todo-046--spawner-distribution-and-particle-shapes)

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
