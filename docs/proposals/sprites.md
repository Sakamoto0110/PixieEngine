# Sprites

**Document ID:** GLOBAL-PROPOSAL-SPRITES

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** images drawn as sprites: PNG loading, a Sprite component, its renderer, text fields in the reflection, and placing sprites in the editor

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-007

**State:** promoted

**Idea:** After PIXIE-PROPOSAL-006, a scene is built by hand, but everything in it is a shape. This
phase draws images. Two parts.

1. **What the engine provides.**
   - **PNG images.** `stb_image`, vendored at the commit already pinned for `stb_easy_font`,
     decodes PNG only (`STBI_ONLY_PNG`): lossless, with alpha, and enough for pixel art. Other
     formats come when they are asked for. Images are kept with premultiplied alpha, so filtering
     leaves no dark fringes.
   - **The `Sprite` component**, reflected and saved like the others:
     - `image`: the path of a PNG, relative to the project folder, with `/`;
     - `region`: the part of the image drawn, in pixels; empty means the whole image (a sprite
       sheet's frame is a region);
     - `pivot`: where the entity's position falls in the sprite, from 0 to 1 on each axis; the
       center by default, `(0.5, 1)` stands a character on its feet;
     - `color`: a tint multiplied with the image, alpha included;
     - `layer`: the drawing order, higher on top; within a layer, the scene's order;
     - `filter`: `Smooth` (with mipmaps, so a large image drawn small does not alias) or `Pixel`
       (crisp pixel art).
     One image pixel is one world unit, times the transform's scale: a sprite has no size of its
     own, and appears at its file's size at zoom 1.
   - **Drawing.** A `SpriteRenderer` draws instanced quads, sorted by layer and batched by
     texture; the two filters are two sampler objects, so one image can be drawn both ways. Sprites
     are drawn first, then particles, then shapes, then the project's overlay, so a background
     sprite never hides particles. A layer shared by shapes and sprites is a later step.
   - **The session's images.** A cache in the session resolves paths against the project folder
     (the working directory without a project), knows each image's size, and loads a file again
     when its modification time changes, checked at most twice a second, in the editor and the
     player alike. A file that is missing or cannot be decoded is drawn as a magenta checkerboard,
     and its reason is logged once; the scene still opens and saves it unchanged.
   - **Picking.** A sprite is picked by its rectangle, rotated with the entity; the selection box
     follows it.
   - **Text fields.** The reflection gains its first non-numeric kind, `FieldKind::Text`, and
     metadata `Asset("png")` saying the text is a file of that kind. A scene saves it as a JSON
     string. The C API, version 7, reads and writes text fields, names a field's asset kind, and
     gives the texture and size of an image of the session, for a tool's preview.
2. **What the editor does with it.**
   - **Drop.** Dropping PNG files from the system onto the viewport makes one sprite per file
     where they fall, named after the file, selected, as one step of the history. A file outside
     the project is copied into `images/` in the project first, and the status line says so, so a
     project stays whole on another machine.
   - **Menu.** "New sprite..." in the viewport's menu picks a PNG and places it at the click.
   - **Inspector.** An asset field shows its path, a `...` button that picks a file of its kind,
     starting in the project folder, and a preview of the image. Editing it is a step of the
     history.
   - **A sample.** `samples/sprites`, a project with only a scene, draws the neko mark from the
     shared resources (`vdr`) tinted, layered, rotated, with both filters, and a region.

Out of this phase: animation, packing atlases, text, other image formats, a layer shared by shapes
and sprites, and exporting a game.

**Rationale:** Images are what make a scene look like a game, and every later step (animation,
exporting a game with its files) needs a project to own its images. Placing sprites by dropping
files, and seeing an image change on disk without reopening anything, keep authoring as direct as
moving entities became in PIXIE-PROPOSAL-006. Sizing sprites by their pixels keeps the scale the
engine already uses, where the views are a few hundred world units wide.

**Constraints:** The engine never depends on the editor: images, sprites, text fields, and the
cache are features of the `engine`, the `runtime`, and the C API, used by the player and tests as
well. A project references only files inside its folder.

**Promotion target:** promoted to `TODO.md` on 2026-10-15 as PIXIE-TODO-027 and PIXIE-TODO-028.

**Source:** [decision record](../archive/sprites-decisions-2026-10-15.md)

**Outcome:** promoted on 2026-10-15, as recorded in the
[promotion record](../archive/sprites-promotion-2026-10-15.md), and carried out the same day:
PIXIE-TODO-027 and PIXIE-TODO-028 are completed (PIXIE-ARCHIVE-048 and 049).
