# Drawing Only What Is on Screen Promoted: PIXIE-TODO-037

**Document ID:** ARCHIVE-VIEW-CULLING-PROMOTION-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** promotion of view culling in a session's drawing (sprites, texts, paths, shapes) as a TODO.md entry with its TASKLIST.md breakdown

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-068

**Type:** promotion

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** with this record (parent `f7dd9109372c6df4b52ae03a288a975bf7aa7386`)

**Current state:** [`TODO.md`](../../TODO.md) and [`TASKLIST.md`](../../TASKLIST.md)

The owner asked, while playing the logic sandbox's 8-bit adder, that what is not on screen as the
zoom grows not be drawn ("conforme for dando zoom, o que não está na tela não precisa ser
renderizado"), and had the engine's part done in this repository while another session does the
game's side. Promoted, with a section in `TASKLIST.md`:

| Entry | Block | Tasks |
|---|---|---|
| PIXIE-TODO-037 | Drawing only what is on screen | PIXIE-TASK-162 to 164 |

Measured beforehand on the 8-bit adder's scene (440 entities, 1280 by 720), by the game's
uncommitted `logic-shot --frames`: the player's drawing cost 0.57 ms at zoom 1 to 1.02 ms at zoom 30
in Release, 6.0 to 8.7 ms in Debug; so drawing everything costs little at that size, and the editor's
22 frames per second on that scene comes mostly from elsewhere (the editor, or the game's own
overlay and screen layer), to be profiled before it is assumed.

Decided by Claude: `SubmitSprites`, `SubmitPaths`, and `SubmitShapes` take an optional visible area
and keep drawing everything without one, so other callers and their results stay as they were; each
item is tested by a conservative bound (a circle that holds a sprite, a text, or a shape however it
is turned and pivoted; a path's points' box widened by half its width), so nothing on screen is ever
skipped; the session pads the camera's visible area by 32 view pixels.

Also changed: `ROADMAP.md` (block 34 in the current direction).
