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

**Last promotion decision:** 2026-10-13

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

Promoted on 2026-10-13 from the owner's review of the anchors and the editor,
recorded in the
[decision record](docs/archive/editor-review-decisions-2026-10-13.md), with the
owner's authorization to execute them, in this order. The first nine entries,
PIXIE-TODO-003 to PIXIE-TODO-011, are completed; see below. Scene serialization
stays postponed and is not promoted.

### `PIXIE-TODO-016` — Selection and editing in the viewport

**Status:** blocked

**Source:** owner decision on 2026-10-13 (request C, answers 34, 40, and 41)

**Accepted scope:** the viewport gets a "Game" and a "Select" mode. In Select mode: a left click on an entity (a boid, an attractor, a spawner) selects it, Shift + left click adds it to the selection, and a drag on empty space selects by box; Ctrl + left click on empty space asks the project to create something there (`Project::SpawnAt`: a random boid in boids and flocks, a particle at rest in particles), and Ctrl + left click on a boid or a particle deletes it. Particles are never selected. The entity list takes Ctrl and Shift clicks too. The inspector shows the components every selected entity has, bound to the first and with `AddBind` to the rest. Selected entities are outlined in the viewport. The C API gains picking (a point, a box), destruction, particle removal, and spawning.

**Dependencies and gates:** 1. PIXIE-TODO-012; 2. PIXIE-TODO-015; 3. PixieInspector.ImGui 0.2.1, for criterion (2): 0.2.0 draws a mixed number that scrubs with the first object's value and no marker; 0.2.1 draws every mixed row's label in amber with a tooltip saying the objects differ ([Sakamoto0110/PixieInspector#8](https://github.com/Sakamoto0110/PixieInspector/pull/8), awaiting merge and publication)

**Completion criteria:** (1) each gesture works in each anchor as described, with real input; (2) editing a value with several attractors selected changes all of them, and different values show as mixed; (3) in Game mode the mouse still goes to the game; (4) `capi-test` covers the new functions; (5) zero warnings.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-016--selection-and-editing-in-the-viewport)

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
