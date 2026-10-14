# PixieEngine Task List

**Document ID:** GLOBAL-TASKLIST

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** task breakdowns of promoted TODO.md entries

**Surface:** task-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

## Purpose

This file is a child scheduler of [`TODO.md`](TODO.md). Each section breaks
one promoted `TODO.md` entry into tasks, so that large entries, or entries made
of many small tasks, do not pollute `TODO.md`.

- Every section names its parent in `Parent`, and the parent links back here
  in `Breakdown`.
- Tasks decompose the parent's accepted scope and never extend it.
- When the parent completes, its section moves to
  [`docs/archive/`](docs/archive/README.md).

The full rules are in the
[documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd),
and the section shape is in the
[template](docs/templates/TASKLIST-SECTION.md).

## Open breakdowns

Opened on 2026-10-14 with PIXIE-TODO-018 to PIXIE-TODO-022; the breakdown
of PIXIE-TODO-018 is in its archive record. Earlier breakdowns are in their archive records, routed from the
[archive router](docs/archive/README.md).

## `PIXIE-TODO-019` — Projects, edit mode, and play mode in the engine

**Parent:** PIXIE-TODO-019

- `PIXIE-TASK-085` — **Status:** open — The project file: reading and writing `<Name>.pixie` with relative paths only, and creating a new project's folder contents (the project file and `scenes/main.scene` with one shape at the origin).
- `PIXIE-TASK-086` — **Status:** open — A `ProjectSession` built from a scene: the scene's entities first, then the project's code; an edit mode that does not step; Play with the scene entities kept in memory, Stop and Rebuild that build the World again; an unsaved-changes flag.
- `PIXIE-TASK-087` — **Status:** open — The generic C API library, whose project only loads the scene with the engine's component types.
- `PIXIE-TASK-088` — **Status:** open — The C API, version 4: create, open, and save a project; play, stop, and the mode; unsaved changes; create an entity; add and remove a component by type name. The C API test drives a whole project through the generic library.
- `PIXIE-TASK-089` — **Status:** open — The project file and the modes in the `runtime` and `capi` modules' `REFERENCE.md`.

## `PIXIE-TODO-020` — New and open projects in the editor

**Parent:** PIXIE-TODO-020

- `PIXIE-TASK-090` — **Status:** open — The start screen with New project, Open project, and the recent projects, kept in the editor's per-user settings; `--project <dir>`.
- `PIXIE-TASK-091` — **Status:** open — A folder picker drawn in ImGui; New project in an empty or new folder, named after it; Open project on a folder with a `.pixie` file, with a plain message when there is none.
- `PIXIE-TASK-092` — **Status:** open — Save with Ctrl + S, the project's name and an unsaved mark in the title; Play and Stop, with the play state shown plainly and the time bar active in play.
- `PIXIE-TASK-093` — **Status:** open — Creating an entity, and adding or removing a component in the inspector from the descriptors' list.
- `PIXIE-TASK-094` — **Status:** open — Verification with real input (new, edit, save, reopen, play, stop, a copied folder) and the editor's README.

## `PIXIE-TODO-021` — Code in a project

**Parent:** PIXIE-TODO-021

- `PIXIE-TASK-095` — **Status:** open — The engine through `add_subdirectory` from outside its repository, with `PIXIE_ENGINE_DIR`, without samples or tests, and the consumer's binaries in the consumer's `bin/`.
- `PIXIE-TASK-096` — **Status:** open — `pixie_game(<name> SOURCES ...)` with the game's library, player, and C API library; the player's `main` in the engine, with the standard options moved from `samples/common`, starting the project's start scene.
- `PIXIE-TASK-097` — **Status:** open — Templates in the engine's repository: `CMakeLists.txt`, and `src/<Name>Project.{hpp,cpp}` with a Project that loads the scene and one example Behaviour with a field.
- `PIXIE-TASK-098` — **Status:** open — In the editor: Project, Add code; building with CMake from `PATH` or `vswhere` and the engine the editor runs from, with the output shown; reopening on the project's library; a missing CMake or compiler reported by name.
- `PIXIE-TASK-099` — **Status:** open — A CTest test that generates a project with code from the templates, builds it, and runs the C API test on its library; verification in the editor on a project outside the repository; the editor's README and the `runtime` module's `REFERENCE.md`.

## `PIXIE-TODO-022` — Samples as projects

**Parent:** PIXIE-TODO-022

- `PIXIE-TASK-100` — **Status:** open — Boids, flocks, and particles each get a `.pixie` file and a scene with their authored entities; generated entities stay generated.
- `PIXIE-TASK-101` — **Status:** open — The three build through `pixie_game`; `samples/common` keeps only what the engine's player does not provide.
- `PIXIE-TASK-102` — **Status:** open — Verification in the editor and the players, and the samples' READMEs.
