# A Logic Sandbox, Proposed

**Document ID:** ARCHIVE-LOGIC-SANDBOX-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's proposal of a logic-circuit sandbox as the first game on the engine, Claude's assessment of what the engine lacks for it, and the decisions taken in the discussion, recorded as PIXIE-PROPOSAL-008, 009, and 010

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-050

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** with this record (parent `00d44eb7ec0d6ca2085e4e5aede442102672a5c1`)

**Current state:** [PIXIE-PROPOSAL-008](../proposals/interactive-2d-games.md), [PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md), and [PIXIE-PROPOSAL-010](../proposals/shared-engine-and-export.md)

## How it arrived

After PIXIE-PROPOSAL-007, the owner sent a proposal with sketches: a sandbox where the player
builds logic circuits to light a lamp (stage 1), then saves circuits and reuses selections as
templates (stage 1.5), and asked what was still missing. Claude answered that the game fits on the
engine as a project with code, and that the engine lacks four things: wires drawn with rounded
corners and a moving signal, a layer for an interface on the screen, text, and folders and metadata
for the player's saves; and that the gate sprites were missing from the shared resources.

## Decisions taken in the discussion

- **Wires bend by points, not by nodes** (Claude's suggestion, asked for by the owner as "something
  more elegant"): a wire is its two ends and a list of bends, routed horizontally and vertically with
  rounded corners; a click on empty space while wiring adds a bend. The relay (the small circle with
  one input and one output) stays as a piece placed on purpose. The engine needs list fields for it.
- **Scope split** (the owner's suggestion): the engine's part is PIXIE-PROPOSAL-008, the game is
  PIXIE-PROPOSAL-009.
- **The game is an executable**, in C++, in a project folder of its own. The owner accepts the
  engine's library and dependencies beside the exported executable, not a game made into a library,
  and does not want each game to carry a copy of the engine in a library of its own. Recorded as
  PIXIE-PROPOSAL-010 (one shared engine library, and exporting), since it reverses the static C++
  runtime decision of 2026-10-11.
- **Gate sprites drawn by a script**, at the owner's request, and registered in the shared resources
  by the session that keeps them (`scripts/logic-gates/logic_gates.py`, tag `logic-gates`; sizes 32,
  64, 128, 256, and 512; white strokes for tinting). The owner pointed out a missing ray of the lit
  lamp, fixed before registering.
- **Bodies without pin stubs** (Claude's suggestion, agreed): the game draws the pins, so one sprite
  serves any number of inputs. The owner asked how a player would know where to click: pins are drawn
  as a stub ending in a dot (output) or a ring (input), clicked within about 10 view pixels, growing
  on hover, with valid targets lit while wiring.
- **Pins by side** (the owner's format): `gates.json` lists `[side, sets]`, sides numbered 0 top,
  1 left, 2 bottom, 3 right, with `sets[n - 1]` the anchors of `n` inputs and `[side, []]` the side
  of the output. Rotating or mirroring a gate moves its anchors with it, so no turned images are
  needed; gates turn in steps of 90 degrees. Each sprite also names its files (`png` with `{size}`,
  `svg`), so the `gate-` prefix is not an unwritten rule.
- **`gates.json` belongs in the shared resources** with the sprites, since it measures them and is
  made in the same run; the game's rules stay in its code.
- Also decided by Claude in the assessment: an output feeds any number of wires and an input takes
  one; Esc or the right button cancels a wire; signals travel down wires at a fixed speed, a gate
  evaluating one step after an input changes; the hotbar gets a Templates page switched with Tab;
  a template is pasted as its expanded circuit.
