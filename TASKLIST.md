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

Opened on 2026-10-14 with PIXIE-TODO-018 to PIXIE-TODO-022; the breakdowns
of PIXIE-TODO-018 to PIXIE-TODO-020 are in their archive records. Earlier breakdowns are in their archive records, routed from the
[archive router](docs/archive/README.md).

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
