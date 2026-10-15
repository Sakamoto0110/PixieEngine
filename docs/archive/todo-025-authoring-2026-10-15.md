# PIXIE-TODO-025 Completed: Moving, Undo and Redo, Duplicating, Deleting, and Renaming in the Editor

**Document ID:** ARCHIVE-TODO-025-AUTHORING-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-025, and with it PIXIE-PROPOSAL-006: building a scene by hand in the editor, with a history of every change

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-044

**Type:** completion

**Origin:** PIXIE-TODO-025 (PIXIE-TASK-111 to PIXIE-TASK-115)

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** `9bc57367e1fa1354a94fe74da87ac9e795db5663`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [editor's README](../../editor/README.md)

## Entry as completed

**Status:** completed on 2026-10-15

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-006](../proposals/authoring-in-the-viewport.md), part 2

**Accepted scope:** in Select mode, dragging an entity moves it, and the selection with it when it was selected; Shift holds the drag to its main axis; a Snap toggle with a grid of 8, 16, 32, or 64 world units rounds positions while dragging; a click without movement still only selects. Undo (Ctrl + Z) and redo (Ctrl + Y, Ctrl + Shift + Z), with an Edit menu naming the step, over every change to the scene in edit mode: inspector edits (one step per press-to-release), moves, creating, deleting, duplicating, adding and removing components, and renaming; steps refer to entities by scene ID; the history lasts the session, through Play, Stop, and Save; nothing is recorded while playing, and undo and redo wait for Stop. Duplicate (Ctrl + D, and in the viewport's menu) copies the selection a little to the side and selects the copies; Delete deletes the selection unless a text field has the keyboard. The inspector renames the selected entity. Out: rotating and scaling by hand, history on disk, copying between projects.

**Dependencies and gates:** 1. PIXIE-TODO-024

**Completion criteria:** (1) in the editor, each gesture works, and each change it makes is undone and redone, also after Play and Stop and after Save; (2) the editor's README describes the gestures, the shortcuts, and the history; (3) zero warnings. The verification runs with real input where the owner allows computer use, and otherwise through options the editor gains for scripted runs, with what could not be run stated.

**Commits:** `9bc57367e1fa1354a94fe74da87ac9e795db5663` (the history, the recorder, moving with axis lock and snap, duplicate, delete, rename, the Edit menu, and `--do`); the editor's README is updated with this record.

## Breakdown as completed

- `PIXIE-TASK-111` — **Status:** done — The history: steps by scene ID, Ctrl + Z, Ctrl + Y, Ctrl + Shift + Z, an Edit menu naming the step; inspector edits recorded one step per press-to-release; nothing recorded while playing.
- `PIXIE-TASK-112` — **Status:** done — Dragging to move in Select mode, the whole selection when it was selected, Shift for one axis, and the Snap toggle with its grid; each drag one step.
- `PIXIE-TASK-113` — **Status:** done — Duplicate (Ctrl + D and the viewport's menu) and Delete (the key, from the viewport or the entity list), each one step; creating, adding, and removing components recorded too.
- `PIXIE-TASK-114` — **Status:** done — Renaming in the inspector, one step per rename.
- `PIXIE-TASK-115` — **Status:** done — Verification (real input or scripted options, as allowed) and the editor's README.

## Choices made while building it

- Two kinds of step (`Editing/History.cs`): a `FieldStep` writes fields back and forth by scene ID
  (inspector edits and moves, which happen every frame of a gesture and would be costly as
  snapshots); a `ReplaceStep` destroys the entities by scene ID and restores a snapshot with the
  same IDs (creating, deleting, duplicating, components, names). Undo and redo return the entities
  to select, since restored entities have new handles.
- Every inspector write goes through `ComponentHandle.Set` to an `EditRecorder`, which remembers each
  field's value at its first write in a gesture and records one step when no control is held any
  more; a `--set` from the command line is one step the same way.
- A move snaps the dragged entity, not each entity, to the grid, so a group keeps its shape; Shift's
  axis is the one the cursor moved most along.
- A duplicate keeps the name of what it copies; it lands 16 world units right and down.
- Recording is on only in a project's edit mode; in play, and in a game opened without a project,
  the same gestures change the World and nothing is recorded. Entities without a scene ID are not
  snapshotted, so they are neither duplicated nor brought back.
- For verification without a mouse, `--do` runs the editing commands as the gestures do (the move
  command calls the same code as a drag), one per frame, logging the state after each.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, .NET SDK 10.0.401, the engine at PIXIE-TODO-024's commit;
the editor built in Debug and Release with zero warnings, at `9bc5736`. At the owner's request,
no computer use: the editor was driven with `--set` and `--do` on a project without code holding
`a` (a shape), `b` (a particle system), and `c` (a spawner feeding `b`).

- `--set Transform2D.positionX=50` through the inspector recorded "edit of Transform2D.position";
  undo put `a` back at (0, 0), redo at (50, 0).
- `move=10,20`: (60, 20), undone to (50, 0), redone.
- `duplicate`: a copy at (76, 36), 4 entities; undone to 3, redone to 4 with the same copy.
- `delete` of the copy: 3 entities; undone: 4, the copy back at (76, 36) and selected.
- `rename=alpha`, undone to `a`, redone; `create` (entity 1), undone.
- `play`, then `undo`: refused, "Undo waits for Stop", the history unchanged; `stop`, then undo and
  redo of the rename worked.
- `add=Velocity2D` and `remove=Shape`, each undone and redone; `save` in the middle cleared the
  unsaved mark, the saved file held the added Velocity2D, and undo and redo went on after it,
  marking the scene unsaved again.
- `select=b`, `select+=c`, `duplicate`: two copies at (116, 16) and (-84, 16); undone and redone
  (the copied spawner feeding the copied system is checked by the C API test of PIXIE-TODO-024).
- `--game boids` with `--set` and `--do`: edits and a duplicate worked, and nothing was recorded, as
  in a game without a project. The user's editor settings were unchanged (`--recent off`).
- Not run: the mouse gestures themselves (dragging to move, Shift's axis, Snap, the Edit menu, the
  rename field, the Delete and Ctrl + D keys), which need real input; they call the code the
  commands above ran. Linux.
