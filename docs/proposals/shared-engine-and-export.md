# One Shared Engine Library, and Exporting a Game

**Document ID:** GLOBAL-PROPOSAL-SHARED-ENGINE-EXPORT

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** building the engine as one shared library that every game and the editor load, so a game's own code is small, and exporting a game as its executable, that library, and its assets

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-010

**State:** idea

**Idea:** Today every game links the whole engine twice: into its player (`<game>.exe`, about 1.5 MB
in Release) and into its C API library (`<game>-capi.dll`, about 1.3 MB), which only the editor
loads. Each sample repeats that. The owner accepts an exported game as its executable, the engine's
library, and the libraries it depends on, but not a game made into a library of its own, and wants
the copies of the engine gone before they cost disk.

- The engine (`engine`, `runtime`, and `capi`) builds as one shared library, `pixie.dll`
  (`libpixie.so` elsewhere), the same for every game.
- A game's executable links `pixie.dll` and holds only the game's code.
- For the editor, a game builds a small module with only its code, which `pixie.dll` loads, in place
  of today's `<game>-capi.dll` with a whole engine inside; the module is for development and is not
  exported.
- Exporting a game makes a folder with its executable, `pixie.dll`, its dependencies, its project
  file and scenes, and its assets, and nothing else; the editor gains an Export command.

C++ types would cross between `pixie.dll` and a game's code, so both must share one C++ runtime: the
static runtime chosen on 2026-10-11 ([record](../archive/static-msvc-runtime-2026-10-11.md))
becomes the shared one (`/MD`), with the Visual C++ runtime shipped beside the game or installed.
That reverses a recorded decision, which is why this is its own proposal.

**Rationale:** The original 2026-10-15 motivation was to reduce repeated engine copies when
exporting games while keeping the executable as what a player runs. This remains an unpromoted
distribution proposal; it is not the next approved execution step. Current direction and gates
are in [ROADMAP.md](../../ROADMAP.md#conditional-investigations).

**Constraints:** The editor keeps running any game without being rebuilt; the C API stays the
boundary the editor sees.

**Promotion target:** `TODO.md`, when the owner decides it, before a game is shipped.

**Source:** the owner's remarks of 2026-10-15 in the
[decision record](../archive/logic-sandbox-decisions-2026-10-15.md).

**Outcome:** none yet.
