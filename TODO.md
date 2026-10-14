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

**Last promotion decision:** 2026-10-14

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

Promoted on 2026-10-14 from
[PIXIE-PROPOSAL-005](docs/proposals/projects-and-scenes.md) (projects created
in the editor, with scene serialization), in this order
([decision record](docs/archive/projects-and-scenes-decisions-2026-10-14.md)).
PIXIE-TODO-018 to PIXIE-TODO-020 are completed; see below.

### `PIXIE-TODO-021` — Code in a project

**Status:** promoted

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](docs/proposals/projects-and-scenes.md), parts 3 and 7

**Accepted scope:** the engine consumable through `add_subdirectory` from a CMake project outside its repository, given as `PIXIE_ENGINE_DIR`, without its samples or tests, with the consumer's binaries in the consumer's `bin/`. `pixie_game(<name> SOURCES ...)` creating a game's library, player, and C API library; the player's `main` owned by the engine, with the standard options now in `samples/common`, starting the project's start scene. Templates in the engine's repository for `CMakeLists.txt` and `src/<Name>Project.{hpp,cpp}`: a Project whose build loads the scene, and one example Behaviour declared with a field. In the editor, Project, Add code, which writes the templates into the project; building, which calls CMake (on `PATH`, or through `vswhere`) with the engine the editor runs from, into `<project>/build/`, and shows the output; and reopening on the project's library. A missing CMake or compiler is reported by name. The project never records the engine's location. Out: reloading code without restarting the editor, exporting a standalone build.

**Dependencies and gates:** 1. PIXIE-TODO-020

**Completion criteria:** (1) a project outside the repository gets code from the editor, builds, reopens, and its example Behaviour can be added to an entity, edited, saved, and played; (2) its player starts the saved scene; (3) a CTest test generates a project with code from the templates, builds it, and runs the C API test on its library; (4) the project folder holds no path to the engine; (5) the editor's README and the `runtime` module's `REFERENCE.md` describe code in a project and `pixie_game`; (6) zero warnings, and `ctest` passes.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-021--code-in-a-project)

### `PIXIE-TODO-022` — Samples as projects

**Status:** promoted

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](docs/proposals/projects-and-scenes.md), part 8

**Accepted scope:** boids, flocks, and particles each get a `.pixie` file and a scene with their authored entities (scenario, controls, attractors, spawners) and build through `pixie_game`, staying in the repository; entities generated from a count stay generated. hello-gl and shapes, which have no World, are left as they are. `samples/common` keeps only what the engine's player does not provide.

**Dependencies and gates:** 1. PIXIE-TODO-021

**Completion criteria:** (1) each of the three opens in the editor with `--project`, edits its scenario, saves, and rebuilds from the saved scene; (2) each player starts from its scene with the behaviour and gestures its README describes; (3) the samples' READMEs describe their projects; (4) zero warnings, and `ctest` passes.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-022--samples-as-projects)

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
