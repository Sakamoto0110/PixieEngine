# Text in the Scene, Proposed

**Document ID:** ARCHIVE-SCENE-TEXT-DECISIONS-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's question of whether to tell screen text from text bound to an entity, Claude's design of a Text component, and the owner's approval, recorded as PIXIE-PROPOSAL-011

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-054

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** with this record (parent `1d2c51eff000222b5c10614b473e5eb0f4d15721`)

**Current state:** [PIXIE-PROPOSAL-011](../proposals/scene-text.md)

## How it arrived

Right after PIXIE-TODO-030 closed PIXIE-PROPOSAL-008, the owner asked whether this was a good time to
tell static text apart from text layered on, or bound to, an entity. At that point text was drawn
only by code: on the screen layer, over everything, and through the `TextRenderer`'s `SubmitWorld`,
which no scene used. A label in the World could only be drawn on the screen layer through
`WorldToScreen`, at a fixed size, as the particles sample labels its spawners.

## Claude's answer

Yes, and now, because of the logic sandbox: its labels belong to the drawing, zoom with it, lie in
the sprites' order, and are saved with a circuit. Built on the screen layer, the game would rebuild
them in code each frame and they would float over everything.

The split Claude proposed: screen text stays code (interfaces, menus, tooltips, and nameplates, whose
showing is the game's rule); text in the scene becomes a `Text` component. It deliberately has no
third kind: a fixed-size label following an entity is a nameplate, already possible on the screen
layer.

The design, decided rather than asked: the component's fields (text, `ttf` font, size in world units,
color, line alignment, pivot, layer); drawn as sprites in their layer and World order; the size
multiplied by the transform's scale, and turned and mirrored with it; atlases from the ladder by the
size on screen; picked by its measured block in the sprites' order, outlined through the C API's view
quad; the font field an asset of kind `ttf`, chosen and copied into `fonts/` like images; a missing
font drawing nothing, with the problem in the inspector and a one-em block for picking; a new `Text`
in the editor taking the project's first `.ttf`; interpolated like a sprite. Out: wrapping, rich text,
kerning, outlines and shadows, binding to a component's value, dropping a `.ttf` to create a text,
and editing several lines in the inspector. The C API keeps version 8 unless the font's asset kind
needs a constant, which it does not: asset kinds are strings.

## Decision

The owner approved recording it and promoting it ("pode", to "Posso registrar a PIXIE-PROPOSAL-011 e
promovê-la como PIXIE-TODO-031?"). It is recorded here as
[PIXIE-PROPOSAL-011](../proposals/scene-text.md) and promoted with the next commit as PIXIE-TODO-031,
before the logic sandbox ([PIXIE-PROPOSAL-009](../proposals/logic-sandbox.md)).
