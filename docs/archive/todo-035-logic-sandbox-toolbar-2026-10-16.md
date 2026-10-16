# PIXIE-TODO-035: The Logic Sandbox's Box Selection and Toolbar

**Document ID:** ARCHIVE-TODO-035-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-035, a box selection, copy and paste, and a toolbar in the LogicSandbox game

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-064

**Type:** completion

**Origin:** PIXIE-TODO-035

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** none in this repository; the work is the game's commit `9316172aeaf9e238897da3641db376cb45ede248` in [`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox), and this record is committed with parent `20b118d503439a8b18b62df78b07705f7a07b76d`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the game's `README.md`, in its repository

## Entry as completed

### `PIXIE-TODO-035` — The logic sandbox: box selection and a toolbar

**Status:** completed

**Source:** owner decision on 2026-10-16 ([promotion record](logic-sandbox-toolbar-promotion-2026-10-16.md))

**Accepted scope:** a drag on empty space, with no piece chosen and no wire being drawn, draws a box and selects the pieces whose centers are in it and the wires between selected pieces (Shift keeps the selection, a click without a drag still clears it); a toolbar along the top of the view, beside the hotbar, with Delete (as Del), Copy (Ctrl + C: the selected pieces and the wires between them, in the game's memory), Paste (Ctrl + V at the cursor; the button 32 units to the side of the copy, further at each paste), with new scene IDs and the pasted entities selected, Save (as Ctrl + S), and Load (as Ctrl + O), each dimmed when it has nothing to act on and naming its key on hover; clicks on the toolbar never reach the circuit. Out: the system clipboard, and templates (PIXIE-TASK-150 and 151), which build on the box selection and on pasting with new IDs.

**Dependencies and gates:** 1. PIXIE-TASK-149, saves

**Completion criteria:** (1) the game's tests, with synthetic input: a box drag selects the pieces in it and the wires between them, and Shift keeps the rest; Delete, Copy, Paste, Save, and Load on the toolbar do what their keys do; a paste gets new scene IDs, keeps its inner wires, and is selected; Ctrl + V pastes at the cursor; a click on the toolbar places nothing; (2) zero warnings in Debug and Release; (3) the README describes the box, the toolbar, and copy and paste.

**Breakdown:** below

## Breakdown as completed

- `PIXIE-TASK-154` — **Status:** done — The box selection: a drag on empty space selects the pieces in it and the wires between them; Shift keeps the rest; tests.
- `PIXIE-TASK-155` — **Status:** done — Copy and paste: Ctrl + C, Ctrl + V at the cursor, new scene IDs, inner wires kept, the paste selected; tests.
- `PIXIE-TASK-156` — **Status:** done — The toolbar (Delete, Copy, Paste, Save, Load), dimmed and named on hover, catching its clicks; the README; tests.

## What was built

In the game, at `9316172`, against PixieEngine `f425cb6` (no engine change):

- `Sandbox`: a press on empty space with no piece chosen starts a box (a click without a drag still
  clears the selection); on release it selects the pieces whose centers are in it, then the wires
  whose two ends are selected, on top of the previous selection with Shift. The box is drawn faintly
  filled and outlined. The toolbar's buttons (`ButtonRect`) sit along the top beside the hotbar;
  Delete acts in the Sandbox (`DeleteSelection`, shared with Del), and the others become a `Command`
  that the project takes after each update, since the project holds the component types. Ctrl + S,
  Ctrl + O, Ctrl + C, and Ctrl + V make the same commands.
- `Saves`: `CopyPieces` writes the pieces and the wires between them with the meta
  `{"kind": "copy", ...}`; `PasteCopy` reads it with new scene IDs, moves it (its pieces' middle to
  the cursor, snapped, or by an offset), and gives each piece its look and each wire its route, dark.
- `LogicSandboxProject::Run` saves, loads, copies, and pastes; the Paste button's offset grows by 32
  units at each paste after a copy.
- `tests/ToolbarTests.cpp`: three tests.

## Verification

- `cmake --build build --config Debug` and `--config Release`: zero warnings.
- `ctest --test-dir build -C Debug` and `-C Release`: `logic-tests` passes, 26 tests (23 before, and
  `ABoxSelectsPiecesAndTheWiresBetweenThem`, `CopyAndPasteMakeNewPiecesWiredAmongThemselves`,
  `TheToolbarDoesWhatItsKeysDo`), 0 failed checks. A first run of the new tests stopped on an EnTT
  assertion in Debug: the test deleted the lamp left selected by the rig and then read it; the test
  now clears the selection first.
- The player (`bin/Release/LogicSandbox --quit-after 2 --screenshot`) showed the toolbar beside the
  hotbar, with Delete, Copy, and Paste dimmed and Save and Load lit.
- Not run: real input, which only the owner can judge (the box's feel, the toolbar's readability).
