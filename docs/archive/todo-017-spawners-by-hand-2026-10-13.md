# PIXIE-TODO-017: Spawners Placed by Hand in the Particles Game

**Document ID:** ARCHIVE-TODO-017-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-017, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-031

**Type:** completion

**Origin:** PIXIE-TODO-017 (PIXIE-TASK-078 to PIXIE-TASK-079)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `71a61ce9c221dc2ecd5071350bf9b15105097929`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [particles README](../../samples/particles/README.md#mouse-and-keys)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13 (spawners handled as attractors are, Alt to add, a new spawner at rate 0)

**Accepted scope:** in the particles game (its player, and the editor's Game mode), spawners are added, moved, and removed with the mouse as attractors are: Alt + left click on empty space adds a spawner there and drags it while held; a left drag on a spawner moves it; a right click on a spawner removes it. A new spawner is a copy of the scene's (a ring of radius 16, 8-second lifetime, speed 60 to 110, the shape's normal, spread 0.6 radians) feeding the `particles` system, with a rate of 0. Up to 16 spawners; each change is logged.

**Dependencies and gates:** 1. PIXIE-TODO-015

**Completion criteria:** (1) each gesture works with real input in the player and in the editor's Game mode; (2) the attractor gestures are unchanged; (3) the particles README lists the gestures; (4) zero warnings.

**Commits:** `71a61ce9c221dc2ecd5071350bf9b15105097929` (the particles project and scene); the particles README is updated with this record.

## Breakdown as completed

- `PIXIE-TASK-078` — **Status:** done — The particles project: Alt + left click adds a spawner at rate 0, a left drag moves one, a right click removes one, up to 16, logged.
- `PIXIE-TASK-079` — **Status:** done — Verification with real input in the player and the editor's Game mode; the particles README.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, monitor at 100 Hz, Visual Studio 2026
(MSVC 14.51), .NET SDK 10.0.401; commit `71a61ce`.

- Debug and Release build with zero warnings; `ctest` passes 5 of 5.
- Real input through the operating system in the particles player (Release,
  1280 x 720, 2,000 particles): Alt + left click at (1000, 150) and at
  (1100, 600) added two spawners ("2 spawners", "3 spawners"); a left drag
  from (1100, 600) to (900, 620) moved the second; a right click at
  (1000, 150) removed the first ("2 spawners"); a left click at (300, 600)
  added an attractor and a right click there removed it ("2 attractors",
  "1 attractors"), as before; a left drag from the scene's spawner at
  (220, 360) to (220, 200) moved it. The saved frame shows the scene's
  spawner at its new place still emitting, and the new one at (900, 620)
  emitting nothing, at rate 0.
- The same in the editor's Game mode, with the viewport focused by its tab:
  two Alt + clicks added spawners, a drag moved one to (1150, 750), and a
  right click removed the other; the entity list shows the new spawner.
- Not run: the limit of 16 spawners, Linux.
