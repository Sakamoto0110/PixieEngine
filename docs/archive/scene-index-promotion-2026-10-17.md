# Reading Many Small Sets of Entities into a Large World Promoted: PIXIE-TODO-038

**Document ID:** ARCHIVE-SCENE-INDEX-PROMOTION-20261017

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** promotion of a scene-ID index kept with the World, so that reading a set of entities costs what its text holds, as a TODO.md entry with its TASKLIST.md breakdown

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-072

**Type:** promotion

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-17

**Commits:** with this record (parent `905d7caa025ab93f501aa20f85f638faee1c31ca`)

**Current state:** [`TODO.md`](../../TODO.md) and [`TASKLIST.md`](../../TASKLIST.md)

The owner is building a gate-level calculator in the logic sandbox, and asked for its load to be
fast. Loading a saved circuit reads each chip's circuit with `ReadEntities` into the main World. The
session working on the game optimized its side (chips no longer place every chip's contents, they
are rewired within their level, circuit texts are cached), measured what remained, and handed the
engine's part to a session in this repository. Promoted, with a section in `TASKLIST.md`:

| Entry | Block | Tasks |
|---|---|---|
| PIXIE-TODO-038 | Reading many small sets of entities into a large World | PIXIE-TASK-165 to 167 |

Measured beforehand by that session, on the calculator demo (2272 chips, about 30,000 pieces): about
104 s in `ReadEntities` alone, against under 1 s for everything else. The cause: every call of
`ReadEntities` (and of `ReadScene`) builds a map of every scene entity in the World before it checks
the set's IDs, so reading many small sets into a large World is quadratic.

Decided by Claude: the map lives in the registry's context, built the first time a read needs it and
kept current from then on by hooks on `SceneId` (added, replaced, removed), so a World that is never
read into pays nothing; a read uses it in place, with its own set's IDs looked up first, so a set's
references still name its new entities; `SceneIds::Keep` and `New` and their checks are unchanged.

Started by the procedure of the [development guide](../development.md#void-agent-workplace): with
VAW at `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1` and the explicit registry, the routes `runtime`
and `documentation` resolved `bound_scoped` for `pixieengine`/1, and the Claude catalog listed
`project-binding`.

Also changed: `ROADMAP.md` (block 35 in the current direction).
