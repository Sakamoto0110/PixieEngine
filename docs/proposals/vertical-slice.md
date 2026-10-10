# Vertical Slice First

**Document ID:** GLOBAL-PROPOSAL-VERTICAL-SLICE

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** order of the first implementation steps

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-003

**State:** idea

**Idea:** Build the whole edit-to-deploy loop thinly before deepening any
module:

0. Repository build skeleton: the player opens a window and clears it.
1. Minimal 2D rendering: shader, textured quad, camera.
2. Scene: an entity with a transform and a sprite, saved and loaded from disk;
   the player loads it.
3. `capi` and the editor: a viewport in an editor host shows the same scene.
4. Inspector: selecting the entity binds InteractiveEditor to its proxy, and
   editing a value updates the viewport.
5. Play, stop, and export: a World from the Scene; export produces a folder
   with the player and the scene.

**Rationale:** Integration problems between the native runtime, the bridge,
and the editor surface early, while each piece is still small.

**Constraints:** Depends on PIXIE-PROPOSAL-001, PIXIE-PROPOSAL-002, and the
open build-system, language-standard, and viewport decisions.

**Promotion target:** owner decision required; promoted steps enter `TODO.md`,
with their breakdown in `TASKLIST.md`

**Source:** [inception record](../archive/inception-2026-10-09.md)
