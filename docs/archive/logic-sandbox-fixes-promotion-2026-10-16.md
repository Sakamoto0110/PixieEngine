# The Logic Sandbox's Stage 1 Fixes Promoted: PIXIE-TODO-034

**Document ID:** ARCHIVE-LOGIC-SANDBOX-FIXES-PROMOTION-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** promotion of fixes to the logic sandbox's stage 1, selecting and removing wires, as a TODO.md entry with its TASKLIST.md breakdown, before stage 1.5

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-060

**Type:** promotion

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** with this record (parent `e9a2c9f9b22eab36cbfce167b057f24d45228a90`)

**Current state:** [`TODO.md`](../../TODO.md) and [`TASKLIST.md`](../../TASKLIST.md)

After playing stage 1 ([PIXIE-ARCHIVE-058](todo-032-logic-sandbox-stage-1-2026-10-15.md)), the owner
asked for two fixes: to select a wire and its bends and remove parts of it or the whole wire, and to
remove a wire with a right click on the input it ends at. The owner chose to do them before stage 1.5
(PIXIE-TODO-033), and to record them here as a new entry tied to the completed PIXIE-TODO-032 rather
than only in the game's repository. Promoted, with a section in `TASKLIST.md`:

| Entry | Block | Tasks |
|---|---|---|
| PIXIE-TODO-034 | The logic sandbox, stage 1 fixes: selecting and removing wires | PIXIE-TASK-152 and 153 |

PIXIE-TODO-033 gains PIXIE-TODO-034 as a dependency. The work is in the game's repository,
[`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox); nothing in the engine
changes.

Decided by Claude, and accepted by the owner ("segue suas recomendações"):

- The "parts" of a wire are its bends. Del removes a selected bend, and the wire routes again through
  the bends left without checking the crossing rule, as a moved piece routes its wires again.
- A right click keeps its order: it cancels a wire being drawn, else puts down the chosen piece, and
  only then removes the wire into the input under the cursor.

Also changed: `ROADMAP.md` (block 31 in the current direction).
