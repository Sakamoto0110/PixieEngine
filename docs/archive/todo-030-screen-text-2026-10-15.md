# PIXIE-TODO-030 Completed: A Screen Layer, Text, Player Saves, and Fragment Metadata

**Document ID:** ARCHIVE-TODO-030-SCREEN-TEXT-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-030: Project::DrawScreen and the ScreenCanvas, TrueType text through stb_truetype (Font and TextRenderer), UserDataFolder, and meta in entity fragments with ReadFragmentMeta; closes PIXIE-PROPOSAL-008

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-053

**Type:** completion

**Origin:** PIXIE-TODO-030 (PIXIE-TASK-136 to PIXIE-TASK-139)

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** `e724a62e42b0756c80101dc71ea3cb7277fbc106`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md#the-screen-layer), [`DEPENDENCIES.md`](../../DEPENDENCIES.md), and the [particles sample's README](../../samples/particles/README.md)

## Entry as completed

**Status:** completed on 2026-10-15

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-008](../proposals/interactive-2d-games.md), items 3, 4, 6, and 7

**Accepted scope:** `Project::DrawScreen(ScreenCanvas&)`, drawn after the World in view pixels with sprites, shapes, paths, and text, in the player and the editor's viewport; `stb_truetype` from the pinned `nothings/stb` commit and a `TextRenderer` (atlas per font and pixel size, UTF-8, measuring and drawing lines) in the world and on the screen; `UserDataFolder(projectName)`; an optional `meta` object in entity fragments, written by `WriteEntities`, returned by `ReadEntities`, and read alone by `ReadFragmentMeta`. Out: the logic sandbox (PIXIE-PROPOSAL-009).

**Dependencies and gates:** 1. PIXIE-TODO-029; 2. a font with an open license registered in the shared resources (`vdr`). Both were met: PIXIE-TODO-029 completed earlier the same day, and Inter 4.1 (OFL) was registered in the shared resources as `inter-regular`, with its license traveling with each copy.

**Completion criteria:** (1) tests: text measured and drawn into a render with content where it should be; a fragment's metadata written, read back, and read alone; the user data folder made; (2) a project with code draws a screen layer with text in the player and in the editor's viewport; (3) the documents describe them, and `DEPENDENCIES.md` records `stb_truetype` and the font; (4) zero warnings, and `ctest` passes locally and in the CI.

**Commits:** `e724a62e42b0756c80101dc71ea3cb7277fbc106` (the font, the text renderer, the screen layer, the user data folder, fragment meta, the particles sample's screen layer, and their tests); the runtime reference and manifest, `DEPENDENCIES.md`, and the particles sample's README are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-136` — **Status:** done — `Project::DrawScreen` and the `ScreenCanvas`, in the session, the player, and the editor's viewport.
- `PIXIE-TASK-137` — **Status:** done — `stb_truetype` and the font recorded; the `TextRenderer`, in the world and on the screen.
- `PIXIE-TASK-138` — **Status:** done — `UserDataFolder`; `meta` in entity fragments and `ReadFragmentMeta`.
- `PIXIE-TASK-139` — **Status:** done — Tests, a project drawing text on its screen layer, and the documents.

## Choices made while building it

The design was presented to the owner before writing and authorized as presented; the points
below are where the build departed from it or settled what it left open.

- **`DrawScreen(World&, ScreenCanvas&)`**, with the World, as `DrawOverlay` has it, rather than the
  scope's `DrawScreen(ScreenCanvas&)`: an interface almost always reads the World, and without it a
  game would keep copies of its state. Decided in the design the owner authorized.
- **The order on the screen layer is fixed**: shapes, then paths, then sprites and text together in
  the order drawn. A panel drawn as a rectangle lies under its labels without the game managing
  layers. Sprites and text share the sprite renderer, so they keep their drawing order between them.
- **Fonts are CPU objects; textures belong to the renderer.** A `Font` (engine) reads the file whole
  and rasterizes atlases with no GL context, so measuring and the atlas tests run without a window.
  The `TextRenderer` keys its textures by the font's never-reused `Id()` and pixel size. The session's
  `Fonts` (runtime) resolves paths against the project folder, as `SpriteImages` does; opening a
  project forgets the fonts and releases the text renderer's textures. Fonts are not read again when
  their file changes, unlike images.
- **Sizes are em sizes** (`STBTT_POINT_SIZE`), as CSS's `font-size`; the line height is the font's
  ascent, descent, and line gap. Text is placed by the top-left of its first line, its first baseline
  at the ascent. There is no kerning, so measuring is the sum of advances and agrees exactly with
  drawing.
- **Atlases hold Latin-1 and one replacement glyph.** The atlas packs U+0020 to U+00FF and U+FFFD;
  a font without U+FFFD packs its `.notdef` glyph there, so every other code point draws a visible
  box. Atlases start at 128 pixels square and double until everything fits (at most 4096), with
  2 pixels of padding, and are uploaded with mipmaps through the existing `Texture`.
- **Screen text is pixel-exact; world text uses a ladder.** `SubmitPixels` rounds the text's origin
  and each pen to whole pixels at exactly the asked size, so screen text is crisp. `SubmitWorld`
  bakes at the next of 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, or 256 pixels above the size on screen
  and scales down, so zooming makes at most eleven atlases per font. No scene component draws text
  yet: a game labels the World from its screen layer, with `WorldToScreen`.
- **The sample is particles, not sprites.** The design named the sprites sample, but it is a project
  without code, and the criterion asks for a project with code; the particles sample, which has code,
  draws the screen layer instead: a panel sized by `MeasureText` with its counts, and each spawner's
  rate centered below it. Inter was copied into `samples/particles/fonts/` with `vdr cp
  inter-regular`, which brought its license along; the engine's text tests read the same file.
- **The user data folder** comes from `SHGetKnownFolderPath(FOLDERID_RoamingAppData)` on Windows
  (linking `shell32` and `ole32`) and `XDG_DATA_HOME`, else `~/.local/share`, elsewhere.
  `PIXIE_USER_DATA` replaces both roots, which the tests use. Names are made folder names by
  `UserDataFolderName`; an empty path, logged, means no folder.
- **Meta is JSON text at the API**, since no public header includes nlohmann/json: `WriteEntities`
  gains an overload taking it, writes it after the format and version, and leaves out, with a
  warning, what is not a JSON object. `ReadEntities` and `ReadFragmentMeta` return it compact; a
  `meta` that is not an object is ignored with a warning. The C API does not carry meta yet.
- **The screen layer is drawn in edit mode too**, so the viewport shows the game as it plays; a
  switch to hide it would need the C API and was left out.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, .NET SDK 10.0.401, at `e724a62`.

- Debug and Release built with zero warnings; the editor too (Release). `ctest` passed in both, 11 of
  11, with `project-code` (a project with code built against the changed engine).
- `engine-tests`: 19 tests, 6 of them new (UTF-8 decoding and its replacements; files that are not
  fonts refused; measuring by advances and lines, sizes, and Latin-1 against the replacement glyph;
  atlases clamped, premultiplied, and laid out; the ladder; and "H" drawn at 32 pixels into a
  96-pixel render target in a hidden window: 162 pixels lit, none outside the glyph's rectangle).
- `runtime-tests`: 81 tests, 4 of them new (a fragment's meta written at the top, read back with the
  entities, and read alone with the count; meta that is not an object left out on writing and ignored
  on reading; names made folder names; the folder made under `PIXIE_USER_DATA`, and again, and none
  for `..`).
- The particles player, with `--quit-after` and `--screenshot`: the panel in the top-left corner read
  "particles", the particle count, "1 attractors, 1 spawners", and "interaction on" in Inter, and
  "150/s" stood below the spawner.
- The editor, with `--project samples/particles --quit-after 8 --screenshot`, without computer use:
  the code built, the project opened in edit mode, and the viewport showed the same panel and label
  over the World.
- `scripts/check_docs.py`: no problems.
- Not run: Linux, where `UserDataFolder`'s roots are written and untested; drawing world text in a
  game, which no sample does yet (`SubmitWorld` is covered by its ladder and by sharing the layout
  with `SubmitPixels`).
- The continuous integration runs on the push that carries these commits; its run is checked then.
