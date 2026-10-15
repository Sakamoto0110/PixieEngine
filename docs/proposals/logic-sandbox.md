# A Sandbox of Logic Circuits

**Document ID:** GLOBAL-PROPOSAL-LOGIC-SANDBOX

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** the owner's first game on the engine: a sandbox where the player places logic gates, power sources, and lamps, wires them, and lights a lamp; then saves circuits and reuses parts of them as templates

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-009

**State:** idea

**Idea:** A game in C++, in a project folder of its own (made by the editor, with code, versioned by
the owner, outside the engine's repository), on the engine features of
[PIXIE-PROPOSAL-008](interactive-2d-games.md). Two stages.

**Stage 1: light the lamp.**

- **Pieces.** Logic gates (AND, OR, NOT, XOR, NAND, NOR, XNOR, BUF), a power source (an output that
  is always 1 while on; a click toggles it), a lamp (one input; lit while it receives 1), and a
  relay (a small circle with one input and one output, to carry a wire elsewhere; more kinds of node
  later). A gate has one output and, for now, 1 to 4 inputs; NOT and BUF have one.
- **Sprites.** From the shared resources: the `logic-gates` set (`vdr cp [logic,sprite] ./assets/`
  and `vdr cp gates.json ./assets/`), drawn by script, white, tinted by the game. The hotbar uses the
  PNG of its exact size (32 or 64); the world uses the 256 with the `Smooth` filter. The sizes are
  drawn separately and are not redundant: the 512 reduced to 32 differs from the native 32 by 10/255
  per channel on average.
- **Pins.** The sprites show the body only; the game draws each pin at the anchors of `gates.json`
  (`sides`, `output`), turned and mirrored with the gate through `SpritePoint`: a short stub ending in
  a filled dot for the output and a ring for an input, so the shape says which pins start a wire.
  A pin is clicked within about 10 view pixels of its end at any zoom; under the mouse it grows and
  brightens. The body, by the `body` box, selects and moves the gate; pins come first.
- **Wires.** A wire starts only at an output and ends at an input. Between them, the game routes
  horizontal and vertical runs with rounded corners; a click on empty space while wiring adds a bend
  the route passes through; Esc or the right button cancels. While wiring, valid inputs light up and
  invalid ones dim, and the preview snaps to a valid input near the mouse. Not allowed: a wire from
  an input; a wire from a gate's output to one of its own inputs; a second wire into an input (an
  output may feed any number of wires); a wire that crosses or overlaps another. A wire is a `Path`
  of PIXIE-PROPOSAL-008 plus a component naming its two ends; moving a gate routes its wires again,
  keeping their bends.
- **Signals.** A change of an output travels down each of its wires at a fixed speed, drawn by the
  path's fill; when it reaches an input, the gate evaluates in the next step. Loops therefore
  oscillate at a visible pace instead of settling in one instant.
- **Hotbar.** Vertical, on the left, on the screen layer: one slot per piece with its symbol, the
  name on hover, keys 1 to 9 to choose; the chosen piece is placed with a click. R turns the
  selected pieces 90 degrees and F mirrors them; wires follow.
- **Goal.** Light a lamp. The game says so when one lights, and the sandbox goes on.

**Stage 1.5: saves and templates.**

- The whole circuit is saved and loaded (Ctrl + S, Ctrl + O) in the game's folder of player files
  (`UserDataFolder`).
- A selection (a box drag) is saved as a template, with a name. Its fragment's metadata holds the
  name, the number of inputs (input pins not fed from inside the selection), and the number of final
  outputs (output pins that feed nothing inside it).
- The hotbar gets a second page, Templates, switched with Tab: keys 1 to 9 choose the first nine,
  0 is "(...)", a menu with all of them; on hover, a slot shows the name, inputs, and outputs.
  Placing a template pastes its circuit, with new IDs, at the mouse.

Out of this proposal: a template as a closed box with its own pins; more devices and kinds of node;
undo inside the game; sound; exporting the game, which waits for
[PIXIE-PROPOSAL-010](shared-engine-and-export.md).

**Rationale:** A circuit sandbox is small, but it exercises what a real game needs from the engine:
placing things, connecting them, an interface, text, saving, and reusing what was built. Lighting a
lamp is a clear first goal; templates turn building into a toolbox.

**Constraints:** The game is an executable built against the engine; its code is not part of the
engine. Its assets come from the shared resources through `vdr cp` and are copied into the
project, so the game never depends on the store at run time. Rules (which sides a gate may use,
how many inputs) live in the game's code; `gates.json` only measures the drawings.

**Promotion target:** `TODO.md`, as one block per stage, after PIXIE-PROPOSAL-008's blocks.

**Source:** the owner's proposal of 2026-10-15 and the discussion in the
[decision record](../archive/logic-sandbox-decisions-2026-10-15.md).

**Outcome:** none yet.
