# PIXIE-TODO-023 Completed: Editor Polish, the Editor's Icon, and a Viewport Menu

**Document ID:** ARCHIVE-TODO-023-EDITOR-POLISH-20261014

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** promotion and completion of PIXIE-TODO-023: a centered start screen, the neko icon on the editor, a right-click menu in the viewport, and view-to-world conversion in the C API

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-038

**Type:** completion

**Origin:** PIXIE-TODO-023 (PIXIE-TASK-103 to PIXIE-TASK-106)

**Author:** claude

**Reference date:** 2026-10-14

**Commits:** `5427c493bccad11366cf075fe11f8d7479b92a87`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown), promoted and completed together

**Current state:** the [editor's README](../../editor/README.md) and the `capi` module's [reference](../modules/capi/REFERENCE.md#picking-destruction-and-spawning)

## Entry as completed

**Status:** completed on 2026-10-14

**Source:** owner decision on 2026-10-14, after seeing the start screen of PIXIE-TODO-020: center it more; use the `neko_purple_light` icon of the shared asset store as the project's icon; consider a right-click context menu instead of Ctrl + click; and the C API needs a way to turn screen coordinates into world coordinates. Claude recommended the menu and specified its items; the owner's request covered building it.

**Accepted scope:** the start screen's column centered horizontally and vertically, with a margin, a larger title, and the title, tagline, buttons, and recent-projects heading centered; the editor's executable and window carry `neko-purple-light`, copied with `vdr cp` under its curated name; in Select mode, a right click opens a menu (on an entity: select it and delete the selection; on empty space: create a scene entity there, ask the game to create one there, and delete the nearest particle) and Ctrl + click goes away; the C API gains `pixie_view_to_world` and `pixie_world_to_view`, as version 5. Out: icons for the players of games, which belong to each game.

**Dependencies and gates:** 1. PIXIE-TODO-020

**Completion criteria:** (1) the start screen is centered in a wide and a narrow window; (2) the editor's executable and window show the icon; (3) each menu item works with real input in a project and in a game; (4) the C API test converts an entity's view point to its position and back; (5) zero warnings, and `ctest` passes.

**Commits:** `5427c493bccad11366cf075fe11f8d7479b92a87`; the editor's README and the `capi` and `runtime` references are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-103` — **Status:** done — The start screen centered both ways, with a larger title and the buttons stacked when they do not fit.
- `PIXIE-TASK-104` — **Status:** done — `neko-purple-light` as the executable's icon (`ApplicationIcon`) and the window's (its 32-bit frames decoded and given to GLFW).
- `PIXIE-TASK-105` — **Status:** done — `pixie_view_to_world` and `pixie_world_to_view`, C API version 5, with the C test.
- `PIXIE-TASK-106` — **Status:** done — The viewport's right-click menu in place of Ctrl + click, with creation at the cursor through a shared helper (`Ui/Authoring.cs`), verified with real input.

## Choices made while building it

- The window's icon goes through GLFW, which takes RGBA pixels, rather than Win32's `WM_SETICON`,
  so it works on every editor host; the icon's PNG frame (256 pixels) is skipped, and the 16 to
  128 pixel frames are given.
- On an entity outside the selection, a right click selects it alone before the menu opens, so
  "Delete" never removes something the user did not mean; inside a multiple selection, the menu
  deletes all of it and says how many.
- "Ask the game to create one here" is offered only for games: a project's generic library creates
  nothing. "Delete the particle here" is offered only when an entity has a particle system.
- A menu action the engine refuses shows in the toolbar instead of ending the editor, and the menu
  closes if what it was opened on is gone.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, Visual Studio 2026 (MSVC 14.51), .NET SDK 10.0.401;
commit `5427c49`.

- Engine Debug and Release with zero warnings; `ctest` passes 6 of 6 in both, with the new
  conversion checks in every game's C API test. The editor builds in Debug and Release with zero
  warnings.
- The start screen at 1600 x 900 (screenshot) and in a window dragged narrow: the column in the
  middle both ways, with its margin.
- Win32 `WM_GETICON` on the running editor's window returned both icons, saved and seen as the
  neko; the executable's associated icon is the neko too.
- With real input, in the Orbits project: right click on empty space offered only "Create entity
  here", which created `entity 2` at the cursor, at (-193.5, -216) in world units; right click on
  it offered "Delete" under its name, which deleted it.
- In particles, in Select mode and paused: right click among the particles offered all three items;
  "Delete the particle here" logged "Deleted a particle", and "Ask the game to create one here"
  logged "The game created one at (305, 369)".
- Found while verifying and fixed before the commit: deleting from the menu named the entity after
  destroying it, and the failed lookup ended the editor.
- Not run: Linux; the icon on another host.
