# PIXIE-TODO-033: The Logic Sandbox, Stage 1.5: Saves and Templates

**Document ID:** ARCHIVE-TODO-033-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-033, saving and loading the circuit and making and placing templates in the LogicSandbox game, which carries out PIXIE-PROPOSAL-009

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-065

**Type:** completion

**Origin:** PIXIE-TODO-033

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** none in this repository; the work is the game's commits `ef64d11` (saves), `33bcc7c` (templates), and `e8d4b92f80062876c0a901c5c0910494741b7ff2` (the Templates page) in [`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox), and this record is committed with parent `64627e2bc12eb586a2c20526b8b6ec8b9419d8a3`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the game's `README.md`, in its repository, and [PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md)

## Entry as completed

### `PIXIE-TODO-033` — The logic sandbox, stage 1.5: saves and templates

**Status:** completed

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md), stage 1.5

**Accepted scope:** the whole circuit saved and loaded with Ctrl + S and Ctrl + O in the game's `UserDataFolder`; a box selection saved as a named template, its fragment's metadata holding the name, the inputs (input pins not fed from inside), and the final outputs (output pins feeding nothing inside); the hotbar's Templates page, switched with Tab, keys 1 to 9 for the first nine and 0 for a menu of all, a slot's hover showing name, inputs, and outputs; placing a template pastes its circuit with new IDs at the mouse. Out: a template as a closed box with pins of its own.

**Dependencies and gates:** 1. PIXIE-TODO-032; 2. PIXIE-TODO-034, put before it by the owner on 2026-10-16; 3. PIXIE-TODO-035, before its templates (PIXIE-TASK-150 and 151), on 2026-10-16

**Completion criteria:** (1) the game's tests: a circuit saved and loaded gives the same pieces, wires, and states; a template's metadata counts its inputs and outputs; a pasted template gets new IDs and keeps its inner wires; the Templates page lists saved templates; (2) zero warnings in Debug and Release; (3) the README describes saving and templates.

**Breakdown:** below

## Breakdown as completed

- `PIXIE-TASK-149` — **Status:** done — Saving and loading the whole circuit with Ctrl + S and Ctrl + O in the `UserDataFolder`; tests. Done on 2026-10-16 in the game's `ef64d11` (Sakamoto0110/LogicSandbox).
- `PIXIE-TASK-150` — **Status:** done — Templates from a box selection, with a name and counts in the fragment's metadata; pasting with new IDs at the mouse; tests. Done on 2026-10-16 in the game's `33bcc7c` (Sakamoto0110/LogicSandbox): the box comes from PIXIE-TODO-035, pasting goes through its copy and paste, and the name is typed from key presses, since the engine's input has no typed text.
- `PIXIE-TASK-151` — **Status:** done — The hotbar's Templates page (Tab, 1 to 9, 0 for the menu, hover details), and the README. Done on 2026-10-16 in the game's `e8d4b92` (Sakamoto0110/LogicSandbox).

## What was built

In the game, against PixieEngine `f425cb6` (no engine change), after PIXIE-TODO-034 and
PIXIE-TODO-035:

- **Saves** (`ef64d11`, as the game's `docs/plans/saves.md` planned, with the owner's decisions
  there): Ctrl + S writes every piece and wire to `circuit.json` in the game's `UserDataFolder`, as a
  set of entities with the meta `{"kind": "circuit", ...}`; Ctrl + O checks it, keeps the circuit in
  memory, replaces it under the same scene IDs, and brings the kept one back if the read fails. The
  loaded circuit is prepared as at Build (`PrepareCircuit`).
- **Templates** (`33bcc7c`): T or the toolbar's Template button asks for a name, typed from key
  presses (the engine's `Input` has no typed text), and saves the selected pieces and the wires
  between them to `templates/<name>.json`, with the meta `{"kind": "template", "name", "inputs",
  "outputs", "pieces", "wires"}` (`CountPins`). `PlaceTemplate` pastes one with new scene IDs at a
  point, through PIXIE-TODO-035's paste.
- **The Templates page** (`e8d4b92`): Tab switches the hotbar; slots 1 to 9 show the first nine
  templates (the start of the name and inputs > outputs), a hovered slot its whole label, and slot 0
  a menu of all. A chosen template is placed at the mouse by a click on empty space, again at each
  click, until Esc or a right click.

## Verification

- `cmake --build build --config Debug` and `--config Release`: zero warnings, at each commit.
- `ctest --test-dir build -C Debug` and `-C Release`: `logic-tests` passes, 32 tests at `e8d4b92`,
  0 failed checks; among them the round trip of a saved circuit, a source saved off, loading nothing
  or something else, the save's meta, a template's counts, typing a name, a template's refusals,
  placing a template, and the Templates page with and without templates. The tests keep their files
  in a temporary folder (`PIXIE_USER_DATA`), never the player's.
- The player showed the toolbar, the help lines, and the page's name under the hotbar.
- Not run: real input, which only the owner can judge; nor the Templates page and the name prompt on
  screen, which no input reaches without a person.
