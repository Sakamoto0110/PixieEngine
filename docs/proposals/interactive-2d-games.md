# Engine Features for Interactive 2D Games

**Document ID:** GLOBAL-PROPOSAL-INTERACTIVE-GAMES

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** what the engine needs before a game built on it can have wires, an interface on the screen, text, and saves of its own: paths with rounded corners, list fields, a screen layer, text, points of a sprite, player save folders, and metadata in entity fragments

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-008

**State:** promoted

**Idea:** The first game the owner wants to build on the engine, a sandbox of logic circuits
([PIXIE-PROPOSAL-009](logic-sandbox.md)), needs things the engine does not have. Each is general,
useful to any 2D game, and none is about logic circuits.

1. **Paths.** A `Path` component, drawn by the session: a polyline in world units with its corners
   rounded to a radius, a width, and two colors split at a fill fraction measured along its length
   (0 draws it all in the first color, 1 all in the second; between, a front moves along it, as a
   signal travelling down a wire). A `PathRenderer` in the engine draws it as antialiased segments
   with round joins and caps, cut from instanced quads in the shader, so a wire stays smooth at any
   zoom. Paths are drawn after sprites of negative layers (backgrounds) and before the other
   sprites, so wires pass under what they connect. Picking covers a path within half its width;
   the editor outlines a selected one.
2. **List fields.** The reflection gains its first list kind, `std::vector<pxPoint>`
   (`FieldKind::PointList`), saved in scenes as `[[x, y], ...]`, read and written by the C API as an
   array of doubles with a count, and edited in the inspector as rows of X and Y with buttons to add
   and remove a point, each change one step of the history. A path's points are such a field.
3. **A screen layer.** `Project::DrawScreen(ScreenCanvas&)` runs after the World is drawn, in pixels
   of the view (origin at the top-left, Y down, no camera): the canvas draws sprites (by image path,
   through the session's images), shapes, paths, and text. It is how a game draws its interface (a
   hotbar, tooltips, a menu) in the player and in the editor's viewport alike. Deciding what the
   mouse is over stays with the game, which has the cursor in the same pixels.
4. **Text.** `stb_truetype`, from the `nothings/stb` commit already pinned, rasterizes a TrueType
   font into an atlas texture per font and pixel size; a `TextRenderer` measures and draws strings
   (UTF-8, left-aligned lines, a color) through the sprite renderer, in the world or on the screen
   layer. Fonts are assets: one with an open license (OFL), registered in the shared resources
   (`vdr`), copied into a project like any image.
5. **Points of a sprite.** `SpritePoint(sprite, transform, pixelSize, uv)` turns a point given in
   the sprite's own units (0 to 1, Y down, as the logic gates' `gates.json` gives its anchors) into
   world units, and `SpriteLocal` turns a world point back; both follow the transform's rotation and
   mirroring, the math `SpriteCorners` already does. A game anchors pins, handles, or effects to a
   sprite with them.
6. **Saves of the player.** `UserDataFolder(projectName)` gives a folder for the game's own files,
   made on first use: `%APPDATA%/<project>/` on Windows, `$XDG_DATA_HOME/<project>/` elsewhere. A
   game saves a whole World with `SaveSceneFile` there, and a part of it with `WriteEntities`.
7. **Metadata in fragments.** `WriteEntities` takes an optional JSON object, `meta`, written at the
   top of the fragment, and `ReadEntities` returns it; `ReadFragmentMeta(text)` reads it alone,
   without building anything, so a list of saved fragments (a game's templates) shows their names
   and counts quickly.

The C API goes to version 8 with list fields; the editor shows paths, list fields, and the screen
layer.

Out of this proposal: the logic sandbox itself (PIXIE-PROPOSAL-009); routing wires (where a path
bends is the game's decision); hierarchy of entities; audio; one engine library shared by games, and
exporting a game ([PIXIE-PROPOSAL-010](shared-engine-and-export.md)).

**Rationale:** Wires, an interface, text, and saves are what make a sandbox a game rather than a
scene to watch; they are also what every later game will ask for. Building them in the engine, with
the editor showing them, keeps the engine the place where a game's look and its data live, as
sprites did, and keeps the game's own code about its rules.

**Constraints:** The engine never depends on the editor. A path is data in the scene, so it is saved,
snapshotted, undone, and picked like any component. Fonts and images come from the shared resources;
the engine generates no asset.

**Promotion target:** `TODO.md`, as one block for the engine (paths, list fields, points of a sprite,
and C API version 8) and one for the screen layer, text, player saves, and fragment metadata, in that
order.

**Source:** the owner's proposal of a logic sandbox, and the discussion recorded in the
[decision record](../archive/logic-sandbox-decisions-2026-10-15.md).

**Outcome:** promoted on 2026-10-15 as PIXIE-TODO-029 and PIXIE-TODO-030, as recorded in the
[promotion record](../archive/interactive-games-promotion-2026-10-15.md), and carried out the same
day: both are completed (PIXIE-ARCHIVE-052 and 053). `DrawScreen` takes the World as well as the
canvas, and the C API does not carry fragment metadata (see PIXIE-ARCHIVE-053).
