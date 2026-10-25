# Native Player Debug Controls Promoted: PIXIE-TODO-039

**Document ID:** ARCHIVE-PLAYER-DEBUG-PROMOTION-20261020

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** owner promotion of F3 FPS/UPS and a top-docked command bar in the native player

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-074

**Type:** promotion

**Origin:** owner decision

**Author:** codex

**Reference date:** 2026-10-20

**Commits:** `a18fe6ba9ecadb94b148f6cce7d23019bd277c70`, which adds this record; the promoted work is in `9d022c1639383f3397cfe7ac52057db1b20860be`

**Current state:** [TODO.md](../../TODO.md)

The owner requested implementation of F3 for a read-only debug panel, initially FPS and UPS,
and F3+C for a text command bar at the top of the window. The accepted commands change VSync
and the frame cap at runtime: `-vsync true|false`, `-fps-cap true <integer >= 15>`, and
`-fps-cap false`. Enter submits, Escape closes, and game input is captured while typing.
The implementation belongs to the native `pixie::Run` host so consumers inherit it without
game-specific UI. C API hosts and the C# editor retain their own presentation/timing controls.
Other debug commands are recommendations, not implementation scope.

This decision promotes PIXIE-TODO-039 without a further approval gate. No commit, push, or
package publication was requested. Existing untracked sample/session files are outside scope.

The explicit local VAW binding resolved `bound_scoped` for engine, build, and documentation
with profile `pixieengine` revision 1; the Codex discovery catalog listed project-binding.
The VAW checkout was `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`.
Binding and discovery grant no additional authority. Repository source and documentation
remain authoritative, under the [development guide](../development.md).
