# PIXIE-TODO-036: The Logic Sandbox, Stage 2: Chips and an 8-Bit Adder

**Document ID:** ARCHIVE-TODO-036-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-036: the engine's visible flag, and in the LogicSandbox game ports, clocks, labels, several outputs, templates placed as chips opened by zoom, and an 8-bit adder built of chips

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-067

**Type:** completion

**Origin:** PIXIE-TODO-036

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** `dc758e3d942cec8214c3383f604617fcaae92bf2` (the visible flag) in this repository, and this record with parent `dc758e3d942cec8214c3383f604617fcaae92bf2`; in [`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox), `ad3f041835327357509fecb94708ddc724b03717` (chips, ports, clocks, labels), `db141146990ef5c4662c0b8d180608cea025989b` (the adders and their tests), `f5abd1906f83210b019562d7fd2699a6cd58c398` (the saved adder and play-test fixes), and `76a692e96839955a2977376610af919ca19b6a82` (the README)

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the game's `README.md`, and [the runtime reference](../modules/runtime/REFERENCE.md#built-in-components) for the visible flag

## Entry as completed

### `PIXIE-TODO-036` — The logic sandbox, stage 2: chips and an 8-bit adder

**Status:** completed

**Source:** owner decision on 2026-10-16

**Accepted scope:** in the engine, a `visible` flag on `Shape`, `Sprite`, `Text`, and `Path` (hidden: not drawn, not picked, still saved). In the game: In and Out ports, a Clock, and labels; pieces with several outputs; a placed template becomes a chip, a box whose pins are its sources, lamps, and ports, ordered top to bottom and labelled, whose content runs inside it and shows by semantic zoom (closed far, opened when large on screen), chips inside chips; chips read-only, exploded back into loose pieces on demand; saving, copying, deleting, and templates take a chip whole. The anchor: a half adder, a full adder of two half-adder chips, and an 8-bit ripple adder of eight full-adder chips, built and saved as templates, with a demo circuit as the project's start scene. Anything else the work needs in the engine, the C API, or the editor is in scope.

**Dependencies and gates:** 1. PIXIE-TODO-033

**Completion criteria:** (1) engine tests for the hidden drawables; (2) the game's tests: the half and full adders' truth tables through chips, sums of the 8-bit adder, explode, copy, save and load with chips, the clock; (3) zero warnings in Debug and Release; (4) the game's README describes ports, clocks, labels, chips, and the adder.

**Breakdown:** below

## Breakdown as completed

- `PIXIE-TASK-157` — **Status:** done — The engine's `visible` flag on shapes, sprites, texts, and paths, in drawing, picking, descriptors, the reference, and tests.
- `PIXIE-TASK-158` — **Status:** done — In the game: pieces with several outputs (`Wire::fromPin`), In and Out ports, a Clock, and labels.
- `PIXIE-TASK-159` — **Status:** done — Chips: a placed template as a box with labelled pins, its content running inside, semantic zoom, chips inside chips.
- `PIXIE-TASK-160` — **Status:** done — Exploding a chip; saving, copying, deleting, and templates with chips.
- `PIXIE-TASK-161` — **Status:** done — The anchor: half adder, full adder, and 8-bit adder templates, the demo scene, tests, and the README.

## What was built

In the engine, at `dc758e3`: `visible` on `Shape`, `Sprite`, `Text`, and `Path`; hidden ones are not
drawn, not picked by `PickAt` or `PickIn`, keep their place, and are saved; the descriptors carry it
and the runtime reference says so.

In the game, as its `docs/plans/chips-and-adder.md` planned:

- **Pieces:** In and Out ports (boxes with an input and an output; an unwired In reads its switch),
  a Clock (flips every half period while it runs), and Chip; `Piece` gains `label`, `period`, and
  `outputs`; pieces may have several outputs (`PieceGeometry::outputs`, `PieceState::outputs`,
  `Wire::fromPin`, old scenes reading 0). The hotbar gains the ports and the clock; L types a label;
  a click switches an In port or runs a clock; + and - change a clock's period.
- **Chips** (`Chips.hpp`): placing a template makes a chip, its pins its sources and In ports
  (inputs) and lamps and Out ports (outputs), top to bottom, labelled; the template's entities stay
  inside with an `Inside` component (their transform in the chip's content space, the template's
  coordinates), and `PlaceContents` puts them in the World at the chip's place and scale, hiding them
  with `visible` until the chip is 160 pixels tall on screen. Chips nest. Signals run inside: an
  input port reads its chip's input, an output port's change reaches the chip's output a step later,
  and a wire's signal runs its length in world units. X (and the toolbar's Explode) takes a chip
  apart. Templates count their ports as their pins; saving, copying, deleting, and templates take a
  chip with its content; labels on the screen name chips, their pins, and ports.
- **The anchor** (`tests/Adder.hpp`, `logic-tests --write-adder`): a half adder, a full adder of two
  half-adder chips and an OR, and an 8-bit ripple adder of eight full-adder chips, saved as templates
  (in the player's folder and the game's `examples/templates/`), and the demo, the 8-bit adder chip
  between 17 switches and 9 lights at 100 + 55 = 155, saved as `scenes/adder8.scene`, the project's
  start scene.

## Choices made while building it

- The pins of a template are its ports (sources, In ports, lamps, Out ports), as the owner chose, so
  PIXIE-TODO-033's rule (unfed inputs, outputs feeding nothing inside) no longer counts them.
- A chip's content keeps its own entities (not a hidden copy), so the engine draws, saves, and
  inspects it as any entity, and the game's signals step runs it unchanged.
- On a small box, a click inside its body acts on the body, and a pin only within 4 pixels of its
  end: found by playing it, since a port 14 pixels wide lay within its pins' reach.
- Pin stubs and relays scale with their piece: found by playing it, a gate's stub stuck out of a
  chip's box.
- Each frame routes only the wires at the top; those inside chips keep their routes in their
  content space.

## Verification

- PixieEngine at `dc758e3`: `cmake --build --preset vs2026-debug` and `vs2026-release` with zero
  warnings; `ctest` 11 of 11 in both (runtime-tests gains `HiddenShapesAndPathsAreNotPicked`).
- The game at `f5abd19`: zero warnings in Debug and Release; `logic-tests` passes 42 tests in both,
  among them the half and full adders' truth tables through their chips; the 8-bit adder's sums
  (100 + 55, 0 + 0, 1 + 1, 200 + 100, 255 + 255 + 1, 255 + 0 + 1, 170 + 85), read once the circuit is
  quiet; an exploded full adder still adding; a circuit with chips saved and loaded whole and a chip
  copied with its content; routes inside chips meeting their pins at every depth; zooming into a
  chip showing its gates; a clock's ticks; labels; a click on a small port.
- Played with computer use (the owner's permission, on the Release player): the start scene showed
  155; the wheel opened the 8-bit adder, a full adder, and a half adder down to its XOR and AND, the
  signals running at each depth; a click on A0 gave 156. The player draws about 2,150 frames per
  second with vsync off; the window showed about 26 with vsync, at every zoom, a limit of the
  desktop, not of the game.
- The editor opened the project (440 entities) and inspected the 8-bit adder chip.
- Not run: how the chips' zoom and labels feel to a person; nothing on macOS or Linux.
- Left open: typed text in the engine's input (the game types names from key presses, as on a US
  layout), offered to the owner as a proposal to record.
