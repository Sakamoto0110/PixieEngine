# PIXIE-TODO-032 Completed: The Logic Sandbox, Stage 1

**Document ID:** ARCHIVE-TODO-032-LOGIC-SANDBOX-STAGE-1-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-032: the LogicSandbox project, its pieces, pins, wires, signals, hotbar, and gestures, tested with synthetic input, with a saved circuit whose lamp lights

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-058

**Type:** completion

**Origin:** PIXIE-TODO-032 (PIXIE-TASK-144 to PIXIE-TASK-148)

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** none in this repository: the game is in `C:\Users\Sakamoto\dev\LogicSandbox`, outside it and not under version control, which the owner keeps; this record is committed with parent `d577557ea762e6c9c74423397cb930614fdc6066`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the project's `README.md` in `C:\Users\Sakamoto\dev\LogicSandbox`, and [PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md)

## Entry as completed

**Status:** completed on 2026-10-15

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md), stage 1

**Accepted scope:** a project with C++ code, `LogicSandbox`, made by the engine's tools in `C:\Users\Sakamoto\dev\LogicSandbox`, with the `logic-gates` sprites, `gates.json`, and Inter copied in through `vdr cp`. Pieces: the eight gates (1 to 4 inputs; NOT and BUF one), a power source toggled by a click, a lamp, and a relay. Pins drawn by the game at the anchors of `gates.json` through `SpritePoint` (a dot for the output, a ring for an input), clicked within about 10 view pixels, growing under the mouse; bodies select and move pieces. Wires as `Path`s from an output to an input, routed horizontally and vertically with rounded corners through the bends clicked on empty space, Esc or the right button cancelling, valid inputs lit and the preview snapping to them; refused: from an input, from a gate to itself, a second wire into an input, a wire crossing or overlapping another; moving a piece routes its wires again. Signals travel down wires at a fixed speed, drawn by the fill, and a gate evaluates one step after an input changes. A vertical hotbar on the left of the screen layer, its symbols from the 32 or 64 PNGs, the name on hover, keys 1 to 9; R turns the selected pieces 90 degrees and F mirrors them. A lamp lit says so, and the sandbox goes on. The circuit is the project's scene, so the editor opens and saves it. Out: saves of the player and templates (PIXIE-TODO-033), undo inside the game, sound.

**Dependencies and gates:** none

**Completion criteria:** (1) the game's own tests, built with the project, pass: every gate's truth table; the wiring rules; a signal taking its time down a wire and a gate answering one step after; and gestures driven with synthetic input (placing pieces, wiring through a bend, toggling the source) ending with the lamp lit; (2) the project builds in Debug and Release with zero warnings against this engine, through the editor's build and on its own; (3) its player shows a saved circuit with a lit lamp in a screenshot, and the editor opens the project; (4) the project's README says how to build, play, and test it; (5) what real input alone can check (feel, hover, snapping) is listed for the owner, since computer use is not used.

## Breakdown as completed

- `PIXIE-TASK-144` — **Status:** done — The project: made with code in `C:\Users\Sakamoto\dev\LogicSandbox`, the sprites, `gates.json`, and Inter copied in with `vdr cp`; its components described for the editor; its tests' target.
- `PIXIE-TASK-145` — **Status:** done — The logic: gates and their truth tables, the power source, the lamp, the relay, signals travelling down wires, evaluation one step after an input changes; tests.
- `PIXIE-TASK-146` — **Status:** done — Pins at the `gates.json` anchors, turned and mirrored; wires routed through bends with rounded corners; the wiring rules; moving pieces re-routes their wires; tests.
- `PIXIE-TASK-147` — **Status:** done — The hotbar on the screen layer, hover names, keys 1 to 9, placing, selecting and moving, R and F, toggling the source, the lamp's message; gestures tested with synthetic input.
- `PIXIE-TASK-148` — **Status:** done — A saved circuit with a lit lamp, the player's screenshot, the editor opening the project, the README, and the list of what real input alone can check.

## What was built

The project was made by the engine's tools without the editor's start screen:
`pixie_project_create` of `pixie-scene-capi` (called through Python's `ctypes`) made the folder, the
project file, and its scene, and `cmake/PixieAddCode.cmake` gave it code from the templates. `vdr cp`
brought the 60 `logic-gates` sprites, `gates.json`, and Inter with its license into `assets/`.

- **Pieces** (`Piece`: kind, inputs, the source's switch) are entities with a `Transform2D` and a
  `Sprite` of the 256-pixel art, drawn 64 units wide; a relay is a `Shape` circle. Gates take inputs
  on their left; the lamp on its bottom, the only side `gates.json` gives it. Pins are computed, not
  stored: each anchor of `gates.json` goes through `SpritePoint`, with a stub of 10 units along the
  side's normal, turned and mirrored with the piece.
- **`gates.json` is built into the game**: `CMakeLists.txt` reads it at configure time into a
  generated header of raw string pieces (MSVC limits one literal's length), parsed once with the
  engine's nlohmann/json, so the game never looks for it at run time and configuring again follows a
  change to it.
- **Wires** (`Wire`: from, to, the input, the bends, saved; the route in a `Path` on the same
  entity): a route leaves the output 16 units straight, runs horizontally and vertically through the
  bends (each a corner unless the route already runs along it), and enters the input 16 units
  straight; the `Path` rounds its corners. A wire may not touch another anywhere, unless both come
  from the same output, so a fan-out may share its first run.
- **Signals** step in two parts that never read what the other wrote in the same step: pieces marked
  earlier evaluate from what has arrived; then changes run down wires at 240 units per second, and
  an arrival marks the piece for the next step. A change that overtakes one still on its way
  replaces it. The `Path`'s fill draws the front: the new value behind it, the arrived value ahead.
  A NOT looped through a relay oscillates at a visible pace.
- **The hotbar and gestures** live in `Sandbox`, read in `BeforeStep` (play only) and drawn in
  `DrawOverlay` (pins, selection, the wire being drawn) and `DrawScreen` (the bar, the piece's ghost,
  messages, a help line). Keys 1 to 9 choose the first nine pieces; BUF and the relay come from the
  bar. Beyond the scope's list, the game also takes Del (remove the selection and its wires), + and
  - (inputs of the selected gates), and Shift to add to the selection, since a sandbox without them
  cannot be edited.
- **The scene** is two sources into an AND and the AND into a lamp through a bend, made by
  `logic-tests --write-scene` with the same gestures the tests use. Wires start dark when the World is
  built, so the editor's edit mode does not show the colors of the run that saved it.

## Choices made while building it

- The input under the cursor wins over snapping: while wiring, a click on a taken input says it is
  taken instead of snapping to the free input 12 units away.
- Moving a piece routes its wires again even when they then cross another; the rule is checked when
  a wire is made. Listed for the owner.
- In edit mode the game routes every wire again as it draws (`DrawOverlay`), so a piece moved in the
  editor takes its wires along; wires whose ends a tool removed are kept there, so the editor's undo
  brings the piece back wired, and are dropped only when the World is built.
- **A limit of the engine found on the way**: `ScreenCanvas` flushes its shapes kind by kind (discs
  before rectangles), so a disc drawn on a panel lies under it. The game draws the relay's dot in the
  bar as a path a hair long instead. Not changed in the engine; a candidate for a later proposal.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, against the engine at `d577557`.

- `logic-tests`: 16 tests, all passing in Debug and Release (`ctest --test-dir build -C <config>`):
  every gate's truth table for 1 to 4 inputs; sources, lamps, and relays; `gates.json` built in; pins
  turned and mirrored; the body; a signal arriving after `ceil(length / 4)` steps at 60 per second
  and the lamp lighting one step later, with the front drawn mid-way; a loop oscillating; routes
  along the axes through bends and into a lamp from below; touching polylines; the wiring rules
  (itself, a missing input, a taken input, a crossing, and a fan-out allowed); a moved piece's
  wires; fewer inputs dropping their wires; and the gestures: the demo built from the hotbar's keys,
  three wires with a bend, the lamp lit with its message, a source switched off (lamp dark) and on
  (lit again); the hotbar by key and by slot; refusals (from an input, a taken input, its own input)
  and the snap from 14 pixels; moving on the grid, R, F, + and -, and Del.
- The project builds in Debug and Release with zero warnings on its own (`cmake -S . -B build
  -DPIXIE_ENGINE_DIR=...`), and through the editor (`PixieEditor --project
  C:/Users/Sakamoto/dev/LogicSandbox --recent off --quit-after 10 --screenshot`), which built the
  code, opened the project in edit mode, and showed the circuit, the bar, and the help line.
- The player (`bin/Release/LogicSandbox --quit-after 3 --screenshot`) showed the saved circuit lit:
  both sources, the AND, the wires in the signal's color with rounded corners, the lamp lit, and "A
  lamp is lit! The sandbox goes on: keep building."
- Not run: real input, which only the owner can judge; the project's README lists it (pin reach and
  snapping, hover, dragging, readability of the bar, routes after moves, the signal's pace).
