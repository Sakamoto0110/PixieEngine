# The Logic Sandbox's 8-Bit Adder Promoted: PIXIE-TODO-036

**Document ID:** ARCHIVE-LOGIC-SANDBOX-ADDER-PROMOTION-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** promotion of a new anchor for the logic sandbox, an 8-bit adder built out of templates that become chips, as a TODO.md entry with its TASKLIST.md breakdown

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-066

**Type:** promotion

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** with this record (parent `6739abcc0b9072b631868db8e918dea243290d0d`)

**Current state:** [`TODO.md`](../../TODO.md) and [`TASKLIST.md`](../../TASKLIST.md)

After PIXIE-PROPOSAL-009 was carried out, the owner set a new anchor: build an 8-bit adder in the
sandbox. When a circuit becomes a template, it becomes a box showing its pins, and zooming into it
shows its logic gates; anything needed for it, clocks for instance, is to be built, as many
templates as needed, and the adder saved at the end. The owner allowed changes anywhere: the engine,
the C API, and the editor; asked for commits along the way and one push at the end; and asked
Claude to follow its own recommendations on any question, working without interruptions. Promoted,
with a section in `TASKLIST.md`:

| Entry | Block | Tasks |
|---|---|---|
| PIXIE-TODO-036 | The logic sandbox, stage 2: chips and an 8-bit adder | PIXIE-TASK-157 to 161 |

Answered by the owner before the work: a chip's pins are its sources and lamps **and** new port
pieces; its content shows by semantic zoom; a chip is read-only and can be exploded back into loose
pieces; Claude pushes both repositories at the end and checks the engine's CI.

Decided by Claude: the engine gains a `visible` flag on its drawables, so the game hides a closed
chip's content, which the engine otherwise draws; a chip's content keeps its template coordinates in
an `Inside` component, and its world transform follows the chip; the plan is the game's
`docs/plans/chips-and-adder.md`.

Also changed: `ROADMAP.md` (block 33 in the current direction).
