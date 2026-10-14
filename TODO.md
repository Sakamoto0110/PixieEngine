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

### `PIXIE-TODO-018` — Save and load scenes

**Status:** promoted

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](docs/proposals/projects-and-scenes.md), part 4, which carries part 3 of [PIXIE-PROPOSAL-004](docs/proposals/scene-model-and-anchors.md)

**Accepted scope:** in the `runtime` module, a World's scene entities written to and read from a JSON scene file through nlohmann/json 3.12.0 (vendored): `format`, `version`, the World's seed, and a flat `entities` list with a stable ID (random u64 as a hex string, held in a `SceneId` component), `name`, `parent` (absent until a hierarchy exists), and `components` by type name, each with its own `v`. Components are read and written through the field descriptors, which gain adding and removing their component on an entity by type name; a Behaviour declared with fields is saved like a component. `Entity` fields are written as the referenced entity's stable ID. Entities without a `SceneId` are not saved. Unknown components and unknown fields are kept and written back unchanged; a missing field keeps its default. Out: hierarchy, prefabs, nested scenes, a binary format, the C API and the editor (PIXIE-TODO-019 and PIXIE-TODO-020).

**Dependencies and gates:** none

**Completion criteria:** (1) nlohmann/json is vendored unmodified, in its own commit, and recorded in `DEPENDENCIES.md` as `ok`; (2) tests show a round trip of a World with every built-in component, a Behaviour with fields, and an `Entity` field gives the same components and values; (3) a test shows a file with an unknown component and an unknown field is written back with both unchanged; (4) entities without a `SceneId` are not written; (5) the scene format is described in the `runtime` module's `REFERENCE.md`; (6) zero warnings in Debug and Release, and `ctest` passes.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-018--save-and-load-scenes)

### `PIXIE-TODO-019` — Projects, edit mode, and play mode in the engine

**Status:** promoted

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](docs/proposals/projects-and-scenes.md), parts 2, 5, and 6

**Accepted scope:** the project file (`<Name>.pixie`: `format` `"pixie-project"`, `version`, `name`, and a start scene, with paths relative to the project's folder and nothing specific to a machine), read and written by the `runtime` module. A `ProjectSession` built from a scene: it loads the scene's entities, then the project's code builds what is not saved (systems, pools, generated entities), and may read the scene's entities, such as a scenario. An edit mode where the World does not step; Play keeps the scene entities in memory, and Stop builds the World again from them; Rebuild builds it again from the current scene entities. A generic C API library whose project only loads the scene, with the engine's own component types, and the contents of a new project (one shape at the origin in `scenes/main.scene`). The C API, version 4: create a project in a folder, open a project, save, play, stop, the current mode, unsaved changes, create an entity, and add or remove a component by type name. Out: the editor's screens (PIXIE-TODO-020), code in a project (PIXIE-TODO-021).

**Dependencies and gates:** 1. PIXIE-TODO-018

**Completion criteria:** (1) the C API test creates a project in a temporary folder through the generic library, adds an entity and a component, saves, opens it again, and finds them; (2) it plays, steps, stops, and finds the World as it was before Play; (3) a project file holds no absolute path; (4) the project file and the session's modes are described in the `runtime` and `capi` modules' `REFERENCE.md`; (5) zero warnings, and `ctest` passes.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-019--projects-edit-mode-and-play-mode-in-the-engine)

### `PIXIE-TODO-020` — New and open projects in the editor

**Status:** promoted

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](docs/proposals/projects-and-scenes.md), parts 1, 5, and 6

**Accepted scope:** the editor started without `--game` or `--project` opens a start screen with New project, Open project, and the recent projects (kept in the editor's per-user settings, never in a project). New project asks for a folder, empty or not yet existing, names the project after it, creates it through the C API, and opens it; Open project asks for a folder with a `.pixie` file. The folder picker is drawn in ImGui, so it works on every editor host. `--project <dir>` opens a project directly; `--game` keeps working. In a project: Save (Ctrl + S), the project's name and an unsaved mark in the window's title, Play and Stop with the play state shown plainly, the time bar active in play, creating an entity, and adding or removing a component in the inspector from the descriptors' list. The editor creates no version-control files. Out: native dialogs, undo, code (PIXIE-TODO-021).

**Dependencies and gates:** 1. PIXIE-TODO-019

**Completion criteria:** (1) with real input: New project in a new folder, add an entity with a shape, save, close, reopen it from the recent list, and find it; (2) Play moves things and Stop restores the scene; (3) the same folder, copied to another path, opens and shows the same scene; (4) the editor's README describes the start screen, the modes, and the new options; (5) zero warnings.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-020--new-and-open-projects-in-the-editor)

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
