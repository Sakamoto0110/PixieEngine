# Authoring in the Viewport, Next

**Document ID:** ARCHIVE-AUTHORING-20261014

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's choice of the next phase after PIXIE-PROPOSAL-005: authoring a scene in the viewport, before sprites and exporting

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-041

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-14

**Commits:** with this record (parent `ab5734cbc4e275d6c545e2b039be89b89efa737e`)

**Current state:** [PIXIE-PROPOSAL-006](../proposals/authoring-in-the-viewport.md) and [`TODO.md`](../../TODO.md)

## How the decision arrived

With PIXIE-PROPOSAL-005 carried out and pushed, the owner asked what the next step was, and whether
the engine depended on the editor for scenes and Worlds. Claude checked the code: nothing in
`engine/`, `runtime/`, or `capi/` refers to the editor; the scene format, edit and play, and building
a project's code belong to the engine and run without it (the `project-code` test builds a project
with no editor). What depends on the editor is the workflow: a project is made, and a scene is
built, most easily there.

Claude recommended making the editor a place to build a scene before adding sprites or exporting a
game: dragging to move, undo and redo, duplicating, deleting, and renaming, with the engine
providing entity snapshots, lookup by scene ID, and names. The owner accepted ("pode fazer").

## Decisions taken by Claude, accepted with the recommendation

- Undo lives in the editor, as steps over scene IDs; the engine gives snapshots in the scene format
  and finds entities by scene ID, so the World stays the only copy of the state and the engine
  keeps no history.
- A press-to-release gesture (a drag in the viewport, a scrub in the inspector) is one step.
- The history lasts one session and survives Play, Stop, and Save; nothing is recorded in play.
- Shift holds a drag to its main axis; Snap rounds to a grid of 8, 16, 32, or 64 units.
- Ctrl + D duplicates a little to the side; Delete deletes unless a text field has the keyboard.
- Next after this phase: sprites (`stb_image`), then exporting a game.
