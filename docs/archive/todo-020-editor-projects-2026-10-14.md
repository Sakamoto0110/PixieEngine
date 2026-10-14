# PIXIE-TODO-020 Completed: New and Open Projects in the Editor

**Document ID:** ARCHIVE-TODO-020-EDITOR-PROJECTS-20261014

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-020: the editor's start screen, its folder picker, saving, play and stop, and creating entities and components

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-037

**Type:** completion

**Origin:** PIXIE-TODO-020 (PIXIE-TASK-090 to PIXIE-TASK-094)

**Author:** claude

**Reference date:** 2026-10-14

**Commits:** `ac895878252b1bc6c94d40d5696cc5735138ec50`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [editor's README](../../editor/README.md#projects)

## Entry as completed

**Status:** completed on 2026-10-14

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-005](../proposals/projects-and-scenes.md), parts 1, 5, and 6

**Accepted scope:** the editor started without `--game` or `--project` opens a start screen with New project, Open project, and the recent projects (kept in the editor's per-user settings, never in a project). New project asks for a folder, empty or not yet existing, names the project after it, creates it through the C API, and opens it; Open project asks for a folder with a `.pixie` file. The folder picker is drawn in ImGui, so it works on every editor host. `--project <dir>` opens a project directly; `--game` keeps working. In a project: Save (Ctrl + S), the project's name and an unsaved mark in the window's title, Play and Stop with the play state shown plainly, the time bar active in play, creating an entity, and adding or removing a component in the inspector from the descriptors' list. The editor creates no version-control files. Out: native dialogs, undo, code (PIXIE-TODO-021).

**Dependencies and gates:** 1. PIXIE-TODO-019

**Completion criteria:** (1) with real input: New project in a new folder, add an entity with a shape, save, close, reopen it from the recent list, and find it; (2) Play moves things and Stop restores the scene; (3) the same folder, copied to another path, opens and shows the same scene; (4) the editor's README describes the start screen, the modes, and the new options; (5) zero warnings.

**Commits:** `ac895878252b1bc6c94d40d5696cc5735138ec50` (the start screen, the folder picker, settings, save, play and stop, and authoring); the editor's README is updated with this record.

## Breakdown as completed

- `PIXIE-TASK-090` — **Status:** done — The start screen with New project, Open project, and the recent projects, kept in the editor's per-user settings; `--project <dir>`.
- `PIXIE-TASK-091` — **Status:** done — A folder picker drawn in ImGui; New project in an empty or new folder, named after it; Open project on a folder with a `.pixie` file, with a plain message when there is none.
- `PIXIE-TASK-092` — **Status:** done — Save with Ctrl + S, the project's name and an unsaved mark in the title; Play and Stop, with the play state shown plainly and the time bar active in play.
- `PIXIE-TASK-093` — **Status:** done — Creating an entity, and adding or removing a component in the inspector from the descriptors' list.
- `PIXIE-TASK-094` — **Status:** done — Verification with real input (new, edit, save, reopen, play, stop, a copied folder) and the editor's README.

## Choices made while building it

- The start screen runs in the editor's own window before any library is loaded; choosing a
  project loads `pixie-scene-capi` and opens the session in the same window. Another project means
  another editor process (File, New or open project), as another game already did, since P/Invoke
  binds one library per process.
- New project takes a location and a name, rather than asking for an empty folder: the folder is
  `<location>/<name>`, which may exist only if empty. The page says why Create is disabled before
  it can be pressed.
- The recent projects and the last location live in `settings.json` under the user's application
  data folder; a recent project that no longer exists is shown as not found, with Forget.
- In edit mode the Game button and the time bar are disabled, and the mouse selects; Play switches
  the viewport to Game and Stop back to Select. Play is shown three ways: the title, the toolbar's
  "PLAYING - changes are not kept", and an orange frame around the viewport.
- Create entity adds a transform and a circle of 24 units, since a shape's default size of one
  unit is too small to see or click, and names the entity `entity <n>` with the first free number.
- Closing the window, or opening another project, with unsaved changes asks Save, Don't save, or
  Cancel. `--quit-after` closes without asking, for scripted runs.
- Ctrl + click on empty space in edit mode still asks the game to create something; a project
  without code creates nothing there and says so. Creating at the cursor waits for a way to turn
  view pixels into world units in the C API.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, .NET SDK 10.0.401, the engine at `38b397b` built in
Release; the editor built in Debug and Release with zero warnings, at `ac89587`.

With real input (computer use, on the editor's window):

- The start screen, with no recent projects; New project: the location typed into the picker's
  path and confirmed with Enter, the name `Orbits`, Create. The editor opened the project, titled
  `PixieEditor - Orbits`, with its circle in the viewport; the folder held `Orbits.pixie`
  (`"startScene": "scenes/main.scene"`, no absolute path) and `scenes/main.scene`.
- Reopened from the recent list: the default dock layout, after a fix (below). Create entity,
  position x 150 and size 20 typed in the inspector, Add component, Velocity2D, linear y -40: the
  title gained `*` and the session panel "(not saved)". Ctrl + S: "Saved Orbits", the `*` gone, and
  the scene file held the velocity.
- Play: the title `Orbits - playing`, the orange frame, the toolbar's notice, the mode on Game, and
  the entity at y -90 after 2.25 s. Stop: y back to 0, steps 0, the mode on Select, nothing unsaved.
- Removing Velocity2D with the header's close button marked the scene unsaved; closing the window
  asked first; Cancel kept the editor open; closing again and Don't save closed it, and the file
  kept the saved velocity.
- The folder copied to `elsewhere/Orbits Copy` opened with `--project` and showed the same scene.
- Open project from the start screen: the picker started at the last location, marked `Orbits Copy`
  as a project, kept Open disabled until inside it, and opened it. File, New or open project, with
  an unsaved entity, asked first; Don't save opened a new editor on the start screen, with both
  projects in the recent list.
- `--game boids --select scenario --set BoidsScenario.count=300 --set BoidsScenario.rebuild=go`
  rebuilt the boids with 300 and exit 0; the games to switch to are flocks and particles only.
  `--game flocks --mode select --paused on` ran without a step.

Found and fixed while verifying, before the commit: the default dock layout was skipped because
ImGui writes `imgui.ini` while the start screen shows (now decided before the first frame); the
name field updated only on Enter, as Hexa's `InputText` with `EnterReturnsTrue` writes back only
then (now it follows every key); a created entity's circle was one unit wide; and two created
entities could share a name.

Not run: Linux; text pasted with a newline into the picker's path, which does not count as Enter
(a pasted newline is text, not a key).
