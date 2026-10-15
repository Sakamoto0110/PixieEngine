# PIXIE-TODO-022 Completed: Samples as Projects

**Document ID:** ARCHIVE-TODO-022-SAMPLES-PROJECTS-20261014

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-022: boids, flocks, and particles as project folders with code and scenes, and the end of PIXIE-PROPOSAL-005

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-040

**Type:** completion

**Origin:** PIXIE-TODO-022 (PIXIE-TASK-100 to PIXIE-TASK-102)

**Author:** claude

**Reference date:** 2026-10-14

**Commits:** `e121214e424170deb79455c87f69b478f75d2c59`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the samples' READMEs ([boids](../../samples/boids/README.md#the-project), [flocks](../../samples/flocks/README.md#the-project), [particles](../../samples/particles/README.md#the-project))

## Entry as completed

**Status:** completed on 2026-10-14

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](../proposals/projects-and-scenes.md), part 8

**Accepted scope:** boids, flocks, and particles each get a `.pixie` file and a scene with their authored entities (scenario, controls, attractors, spawners) and build through `pixie_game`, staying in the repository; entities generated from a count stay generated. hello-gl and shapes, which have no World, are left as they are. `samples/common` keeps only what the engine's player does not provide.

**Dependencies and gates:** 1. PIXIE-TODO-021

**Completion criteria:** (1) each of the three opens in the editor with `--project`, edits its scenario, saves, and rebuilds from the saved scene; (2) each player starts from its scene with the behaviour and gestures its README describes; (3) the samples' READMEs describe their projects; (4) zero warnings, and `ctest` passes.

**Commits:** `e121214e424170deb79455c87f69b478f75d2c59` (the samples as projects, their scenes, `SceneEntityWith`, `PROJECT_DIR`, `capi-test --open`, and the editor's scripting options); the samples' READMEs, the editor's README, and the module references are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-100` — **Status:** done — Boids, flocks, and particles each get a `.pixie` file and a scene with their authored entities; generated entities stay generated.
- `PIXIE-TASK-101` — **Status:** done — The three build through `pixie_game`; `samples/common` keeps only what the engine's player does not provide.
- `PIXIE-TASK-102` — **Status:** done — Verification in the editor and the players, and the samples' READMEs.

## Choices made while building it

- Each sample folder is the project: `<sample>.pixie` and `scenes/main.scene` sit next to its
  `CMakeLists.txt`, which works both ways. In the engine's build it is a subdirectory, as before;
  opened in the editor, the folder is a project of its own (`project()` and
  `cmake/PixieProject.cmake` when it is the top level), built into its own `build/` and `bin/`,
  which git ignores. The editor so opens a sample exactly as it opens a user's project.
- The authored entities are the scenario and the controls (and, in particles, the particle system
  with its controls, the attractor, and the spawner); boids and particles stay generated from the
  scenario's count. A project finds its entities with `SceneEntityWith<T>` (new in `Scene.hpp`),
  which makes them as scene entities when the scene has none, so `--game` and the C API test, which
  build without a scene, get the same World as before.
- `ParticlesScenario` loses `attractors`: attractors are scene entities now, placed and saved like
  any other; a World without a scene starts with one in the middle.
- The scene files were written by the editor itself: each sample started from an empty scene, the
  editor opened it with `--project`, the project made its defaults, and `--save-after` saved them.
- `pixie_game` gains `PROJECT_DIR`, for a game built in another tree than its project: the samples'
  players land in the engine's `bin/` and open their folder, and each sample gets a CTest test,
  `<sample>-project`, that opens the project with the game's library, plays, and stops, changing
  nothing (`capi-test --open`).
- The samples' own players, with `--count`, `--seed`, `--attractors`, and `--interaction`, are
  gone: the engine's player plays the scene, whose scenario holds those values. `samples/common`
  had already gone in PIXIE-TODO-021, so PIXIE-TASK-101 found nothing left to keep there.
- For verification without the mouse, the editor gains `--save-after` and `--recent off` (scripted
  runs leave the user's recent projects alone, after PIXIE-TODO-021's runs had touched them), and its
  scripted times count from when the project opens, so they never cut a build short.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, Visual Studio 2026 (MSVC 14.51), .NET SDK 10.0.401;
commit `e121214`. At the owner's request, nothing was verified with computer use in this block:
everything ran from the command line.

- Engine Debug and Release with zero warnings; `ctest` passes 10 of 10 in both, with
  `boids-project`, `flocks-project`, and `particles-project` opening 1,002, 5,002, and 4 entities,
  playing 12 steps, and stopping back. The editor builds in Debug and Release with zero warnings.
- `PixieEditor --project samples/<sample> --recent off --save-after 1 --quit-after 2`, for each
  sample: the editor built the folder as a project of its own (the engine with it), opened it with
  `<sample>-capi.dll` from the folder's `bin/Debug/`, and saved the scene: boids and flocks with
  `scenario` and `controls` or `flock`, particles with `scenario`, `particles`, `spawner` (its
  `system` the `particles` entity's ID), and `attractor`.
- For each sample, `--select scenario --set <Scenario>.count=<n> --set <Scenario>.rebuild=go
  --save-after 2`, then the project opened again: boids at 300, flocks at 800, and particles at 500,
  both after the rebuild and after reopening, and the scene file held the count. The original scenes
  were put back afterwards.
- `bin/Release/boids`, `flocks`, and `particles`, started from another working directory with
  `--quit-after 2 --screenshot`, opened their sample folder and played the scene (boids wandering,
  particles in orbit around the middle attractor fed by the spawner on the left), exit 0.
- The user's editor settings were unchanged by these runs (`--recent off`).
- Not run: the samples' mouse and key gestures in their players with real input (no computer use
  in this block; the gesture code is unchanged); Linux.
