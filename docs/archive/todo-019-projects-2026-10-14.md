# PIXIE-TODO-019 Completed: Projects, Edit Mode, and Play Mode in the Engine

**Document ID:** ARCHIVE-TODO-019-PROJECTS-20261014

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-019: project folders, sessions built from a scene with edit and play modes, the generic C API library, and C API version 4

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-036

**Type:** completion

**Origin:** PIXIE-TODO-019 (PIXIE-TASK-085 to PIXIE-TASK-089)

**Author:** claude

**Reference date:** 2026-10-14

**Commits:** `38b397beda251dcce4105ab6a2ba4b85316cdd1c`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the `runtime` module's [reference](../modules/runtime/REFERENCE.md#projects-and-sessions) and the `capi` module's [reference](../modules/capi/REFERENCE.md#projects-and-modes)

## Entry as completed

**Status:** completed on 2026-10-14

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](../proposals/projects-and-scenes.md), parts 2, 5, and 6

**Accepted scope:** the project file (`<Name>.pixie`: `format` `"pixie-project"`, `version`, `name`, and a start scene, with paths relative to the project's folder and nothing specific to a machine), read and written by the `runtime` module. A `ProjectSession` built from a scene: it loads the scene's entities, then the project's code builds what is not saved (systems, pools, generated entities), and may read the scene's entities, such as a scenario. An edit mode where the World does not step; Play keeps the scene entities in memory, and Stop builds the World again from them; Rebuild builds it again from the current scene entities. A generic C API library whose project only loads the scene, with the engine's own component types, and the contents of a new project (one shape at the origin in `scenes/main.scene`). The C API, version 4: create a project in a folder, open a project, save, play, stop, the current mode, unsaved changes, create an entity, and add or remove a component by type name. Out: the editor's screens (PIXIE-TODO-020), code in a project (PIXIE-TODO-021).

**Dependencies and gates:** 1. PIXIE-TODO-018

**Completion criteria:** (1) the C API test creates a project in a temporary folder through the generic library, adds an entity and a component, saves, opens it again, and finds them; (2) it plays, steps, stops, and finds the World as it was before Play; (3) a project file holds no absolute path; (4) the project file and the session's modes are described in the `runtime` and `capi` modules' `REFERENCE.md`; (5) zero warnings, and `ctest` passes.

**Commits:** `38b397beda251dcce4105ab6a2ba4b85316cdd1c` (project folders, modes, the generic library, C API version 4, and tests); the module documents are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-085` — **Status:** done — The project file: reading and writing `<Name>.pixie` with relative paths only, and creating a new project's folder contents (the project file and `scenes/main.scene` with one shape at the origin).
- `PIXIE-TASK-086` — **Status:** done — A `ProjectSession` built from a scene: the scene's entities first, then the project's code; an edit mode that does not step; Play with the scene entities kept in memory, Stop and Rebuild that build the World again; an unsaved-changes flag.
- `PIXIE-TASK-087` — **Status:** done — The generic C API library, whose project only loads the scene with the engine's component types.
- `PIXIE-TASK-088` — **Status:** done — The C API, version 4: create, open, and save a project; play, stop, and the mode; unsaved changes; create an entity; add and remove a component by type name. The C API test drives a whole project through the generic library.
- `PIXIE-TASK-089` — **Status:** done — The project file and the modes in the `runtime` and `capi` modules' `REFERENCE.md`.

## Choices made while building it

- Components are added and removed by type index, like every other function of the C API, and
  `pixie_type_find` gives the index of a name, so "by type name" is one call away.
- Play builds the World again from the scene's entities instead of running on from the edit World,
  so play starts as the player would from the saved file, and a scenario edited in edit mode
  applies. Play and Stop keep the camera; opening a project and a session without one reset it.
- In edit mode the session pans and zooms the camera itself (`PanAndZoom`), since the game's
  `BeforeStep`, which does it in the samples, does not run.
- Only edits to scene entities mark the scene unsaved: writing a field of, or destroying, an entity
  the game generated does not, since it would not be saved anyway.
- With a scene, the World's seed is the scene's; `pixie_session_open_project` takes no seed.
- The generic library is named `pixie-scene-capi`, and its project reports the project file's
  name. The editor's list of sibling games will show it as `pixie-scene` until PIXIE-TODO-020
  changes the start of the editor.
- Writing a file through a temporary file moved from `SaveSceneFile` into `SaveTextFile`, which the
  project file uses too.
- The editor expects C API version 4, so it keeps loading the games; it uses none of the new
  functions yet.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, Visual Studio 2026 (MSVC 14.51), .NET SDK 10.0.401;
commit `38b397b`.

- Debug and Release build every target with zero warnings; `ctest` passes 6 of 6 in both, with
  `runtime-tests` at 55 tests and 0 failed checks.
- `pixie-scene-capi` (CTest, and by hand on a folder named `My-Game`): created a project, refused
  to create it again, opened it in edit mode with its one shape drawn and no step run, created
  `mover` with a transform and a velocity of 100 units per second, saved, played 24 steps and read
  the position at 40 without the scene marked unsaved, stopped and read it at 0, opened the project
  again and read the saved velocity, and refused a folder that is not a project. The project file
  it wrote:

  ```json
  {
    "format": "pixie-project",
    "version": 1,
    "name": "My-Game",
    "startScene": "scenes/main.scene"
  }
  ```

- `boids-capi`, `flocks-capi`, and `particles-capi` pass with the new checks: a session without a
  project plays and refuses play, stop, and save; an entity is created, gets a shape, loses it, and
  is destroyed.
- The editor (Release) builds with no errors and runs `--game particles --quit-after 3`: 180 steps
  in 3 s, exit 0.
- Not run: Linux; a project opened in the editor (PIXIE-TODO-020).
