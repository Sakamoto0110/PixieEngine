# Text in the Scene

**Document ID:** GLOBAL-PROPOSAL-SCENE-TEXT

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** a Text component: text that lives in the World as data, at an entity's transform, drawn in the sprites' layer order, saved, picked, and edited, apart from the screen layer's text drawn by code

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-011

**State:** promoted

**Idea:** The engine draws text two ways today, and both are code: on the screen layer
(`ScreenCanvas::Text`, in view pixels, over everything), and through the `TextRenderer`'s
`SubmitWorld`, which nothing in a scene uses. Text that belongs to the World, such as the labels of a
logic gate's pins or a sign in a level, has no place in a scene. This proposal draws the line between
the two kinds and adds the second:

| | Screen text | Text in the scene |
|---|---|---|
| Lives in | code, `DrawScreen`, each frame | a `Text` component on an entity, saved with the scene |
| Placed in | view pixels, no camera | the entity's transform, in world units |
| Zoom | fixed size | scales with the World, as a sprite does |
| Order | over everything | in the sprites' layer order |
| Editor | not shown in the inspector | edited, selected, picked, undone |
| For | interfaces, menus, tooltips, nameplates | labels of gates, signs, titles in a level |

There is no third kind: a label that follows an entity at a fixed size on screen (a nameplate) stays
code on the screen layer, through `WorldToScreen`, because when it shows and what it says are the
game's rules.

1. **The component.** `Text` holds the text (UTF-8, line breaks allowed), the font (a text field of
   asset kind `ttf`, relative to the project folder), its size in world units (the em size), a color,
   an alignment of its lines (`Left`, `Center`, `Right`), a pivot (0 to 1 over the measured block, as
   a sprite's), and a layer.
2. **Drawn as sprites, in their order.** Each glyph is a quad in the `SpriteRenderer`, so a `Text`
   takes its place among sprites by layer and, within a layer, in the World's order
   (`ForEachSpriteInDrawOrder`): a label made after its gate lies over it; one on a negative layer
   lies under paths, with the backgrounds. The size multiplies by the transform's scale, and the text
   turns and mirrors with the transform: glyphs are laid out in the entity's space and transformed,
   each quad turned. The atlas is chosen from the ladder by the size on screen (`SubmitWorld`), so
   text stays sharp zoomed in and does not burden the atlases zoomed out. It is interpolated between
   steps like a sprite.
3. **Aligned lines.** The layout measures each line, so `Center` and `Right` align lines within the
   block; measuring and drawing still agree.
4. **Picked by its measured block**, turned, in the sprites' picking order; the C API's view quad
   gives its outline, so the editor outlines a selected text as it does a sprite.
5. **Fonts as assets.** The font field has asset kind `ttf`, as an image field has `png`; the editor's
   chooser for it lists `.ttf` files and copies one from outside the project into its `fonts/`
   folder. A text without a readable font draws nothing, and the inspector shows the problem, as for
   an image; its block, for picking and the outline, is then one em square. When a `Text` is added in
   the editor, its font is the first `.ttf` in the project, if any. The engine still ships no font.

Out of this proposal: wrapping lines at a width, rich text, kerning, outlines and shadows, text bound
to a component's value, creating a text by dropping a `.ttf` on the viewport, and editing several
lines in the inspector (its text box edits one line; line breaks come from code or the scene file).

**Rationale:** The logic sandbox ([PIXIE-PROPOSAL-009](logic-sandbox.md)) labels its gates, pins,
and lamps; those labels are part of the drawing, zoom with it, lie under a gate dragged over them,
and are saved with the circuit. Built on the screen layer they would float over everything and be
rebuilt by the game's code each frame. The text path is fresh from PIXIE-TODO-030, so the component
costs little now.

**Constraints:** The engine never depends on the editor. A `Text` is data in the scene, so it is
saved, snapshotted, undone, and picked like any component. Fonts come from the shared resources; the
engine generates no asset. The asset kind is a string in the field descriptors, so the C API needs no
new version for it.

**Promotion target:** `TODO.md`, as one block, before PIXIE-PROPOSAL-009.

**Source:** the owner's question of whether static text and text bound to an entity should be told
apart, and the discussion recorded in the
[decision record](../archive/scene-text-decisions-2026-10-15.md).

**Outcome:** promoted on 2026-10-15 as PIXIE-TODO-031, as recorded in the
[promotion record](../archive/scene-text-promotion-2026-10-15.md).
