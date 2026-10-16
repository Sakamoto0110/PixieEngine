# The Logic Sandbox's Box Selection and Toolbar Promoted: PIXIE-TODO-035

**Document ID:** ARCHIVE-LOGIC-SANDBOX-TOOLBAR-PROMOTION-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** promotion of a box selection and a toolbar (delete, copy, paste, save, load) in the logic sandbox, as a TODO.md entry with its TASKLIST.md breakdown, before the templates of PIXIE-TODO-033

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-063

**Type:** promotion

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** with this record (parent `9e5c196a066e7ce9bed1b015db8d68153d6ab389`)

**Current state:** [`TODO.md`](../../TODO.md) and [`TASKLIST.md`](../../TASKLIST.md)

After the saves of PIXIE-TASK-149, the owner asked for the game's interface to gain a click and drag
selection, and buttons along the top for things such as delete, copy, save, and load, with
placeholders where saves were not ready (they were). Promoted, with a section in `TASKLIST.md`:

| Entry | Block | Tasks |
|---|---|---|
| PIXIE-TODO-035 | The logic sandbox: box selection and a toolbar | PIXIE-TASK-154 to 156 |

It comes before the templates of PIXIE-TODO-033 (PIXIE-TASK-150 and 151), which gains it as a
dependency: templates are made from a box selection, and pasted with new IDs, as copy and paste
will be. The scope of PIXIE-TODO-033 does not change. The work is in the game's repository,
[`Sakamoto0110/LogicSandbox`](https://github.com/Sakamoto0110/LogicSandbox); nothing in the engine
changes.

Decided by Claude, as with PIXIE-TODO-034 (the owner follows Claude's recommendations there):

- A box selects the pieces whose centers are in it, as the editor's box does, and the wires whose
  two ends are then selected.
- Paste goes with Copy, which alone would do nothing. The copy stays in the game's memory, not the
  system clipboard. Ctrl + V pastes at the cursor; the button, whose click is on the toolbar, pastes
  32 units to the side of the copy, further at each paste, as the editor's Duplicate does.
- The toolbar sits along the top of the view, beside the hotbar, so its place does not depend on the
  view's width.

Also changed: `ROADMAP.md` (block 32 in the current direction).
