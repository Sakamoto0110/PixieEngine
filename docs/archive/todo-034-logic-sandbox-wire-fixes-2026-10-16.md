# PIXIE-TODO-034: The Logic Sandbox, Stage 1 Fixes

**Document ID:** ARCHIVE-TODO-034-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-034, selecting wires and their bends, removing them, and removing a wire with a right click on its input, in the LogicSandbox game

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-061

**Type:** completion

**Origin:** PIXIE-TODO-034

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** none in this repository; the work is the game's commit `8410b53ea33c8ecb71c2fa36336864c4dc76a733` in [`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox), and this record is committed with parent `5623a36c49a961b27a57d7c9fd1e8b3247975dbe`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the game's `README.md`, in its repository

## Entry as completed

### `PIXIE-TODO-034` — The logic sandbox, stage 1 fixes: selecting and removing wires

**Status:** completed

**Source:** owner decision on 2026-10-16, after playing stage 1 ([promotion record](logic-sandbox-fixes-promotion-2026-10-16.md))

**Accepted scope:** a click on a wire selects it, and Shift adds or removes it from the selection, as with pieces; a selected wire shows its bends, and a click on one selects that bend; Del removes a selected bend, and the wire routes again through the bends left (the crossing rule is not checked, as when a piece moves), or else the selected wires and pieces, marking the pieces those wires fed to evaluate again; a right click on an input that has a wire removes that wire, when no wire is being drawn and no piece is chosen (a right click keeps cancelling a wire being drawn, then putting down the chosen piece, first). Out: moving bends, and adding bends to a wire already made.

**Dependencies and gates:** 1. PIXIE-TODO-032

**Completion criteria:** (1) the game's tests, with synthetic input: a click selects a wire and Shift adds one; a click selects a selected wire's bend; Del removes the bend and the wire routes again without it; Del removes a selected wire, and the piece it fed evaluates again; a right click on a wired input removes its wire; with a piece chosen, the first right click only puts it down; (2) zero warnings in Debug and Release; (3) the README describes selecting and removing wires and bends, and the right click on an input.

**Breakdown:** below

## Breakdown as completed

- `PIXIE-TASK-152` — **Status:** done — Selecting wires (click, Shift) and a selected wire's bends; Del removes a bend, routing the wire again, or the selected wires and pieces; tests.
- `PIXIE-TASK-153` — **Status:** done — A right click on a wired input removes its wire, after cancelling a wire and putting down a piece; the README; tests.

## What was built

In the game, at `8410b53`, against PixieEngine `f425cb6` (no engine change):

- `Sandbox`: the selection holds pieces and wires. A click within 6 view pixels of a wire's route
  selects it (`WireAt`), and Shift adds or removes it, through the same `Select` as pieces. A selected
  wire is drawn with a line along its route and a ring on each bend; a click within 10 view pixels of
  one selects it (`BendAt`), drawn filled, and any other click forgets it. Del removes the selected
  bend alone, else the selected wires and then the selected pieces. A right click cancels a wire being
  drawn, else puts down the chosen piece, else removes the wire into the input under the cursor
  (`Unwire`) and says "Wire removed.".
- `Wires`: `RemoveWire` (marks the piece the wire fed to evaluate again; `RemovePiece` uses it) and
  `RemoveBend` (routes the wire again through the bends left).
- The help line became two lines, and the message moved above them.
- `tests/EditingTests.cpp`: three tests driving the gestures with synthetic input.

## Choices made while building it

- A click on a wire comes after pins, a selected wire's bends, and pieces, and before placing the
  chosen piece: with a piece chosen, a click on a wire selects it instead of placing there.
- Del with a bend selected removes only the bend and keeps the wire selected, so further bends can be
  picked and removed one by one.

## Verification

- `cmake --build build --config Debug` and `--config Release`: zero warnings.
- `ctest --test-dir build -C Debug` and `-C Release`: `logic-tests` passes, 19 tests (16 before, and
  `ClicksSelectWiresAndTheirBends`, `DelRemovesSelectedWiresAndPieces`,
  `ARightClickOnAWiredInputUnwiresIt`), 0 failed checks.
- The player (`bin/Release/LogicSandbox --quit-after 3 --screenshot`) showed the lit demo circuit,
  the two help lines, and the message above them, without overlap.
- Not run: real input, which only the owner can judge; the game's README lists it (how easy a wire
  is to hit, and how a selected wire, its bends, and the selected bend read).
