# PIXIE-TODO-031 Completed: Text in the Scene

**Document ID:** ARCHIVE-TODO-031-SCENE-TEXT-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-031, and with it PIXIE-PROPOSAL-011: aligned lines, the Text component drawn and picked among sprites, its outline in the C API, and fonts as assets in the editor

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-056

**Type:** completion

**Origin:** PIXIE-TODO-031 (PIXIE-TASK-140 to PIXIE-TASK-143)

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** `62e9789500c403eaf29af03b30a136af3090a160`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md#text-in-the-scene), [C API reference](../modules/capi/REFERENCE.md), and the [editor's README](../../editor/README.md)

## Entry as completed

**Status:** completed on 2026-10-15

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-011](../proposals/scene-text.md)

**Accepted scope:** in the `engine`, lines aligned `Left`, `Center`, or `Right` in the font's layout, measured line by line. In the `runtime`, a `Text` component (UTF-8 text, a font as a text field of asset kind `ttf`, a size in world units, a color, an alignment, a pivot over the measured block, and a layer) saved with the scene, drawn by the session as sprites in their layer and World order, its size multiplied by the transform's scale and its glyphs turned and mirrored with it, from the ladder's atlas for its size on screen, interpolated like a sprite; picked by its turned block in the sprites' picking order; a text without a readable font draws nothing and has a one-em block. In the C API, a text's outline through the view quad, at version 8. In the editor: a `ttf` field chosen among `.ttf` files and copied into the project's `fonts/` folder, a new `Text` taking the project's first font, and a selected text outlined. Out: wrapping, rich text, kerning, outlines and shadows, text bound to a component's value, dropping a `.ttf` to create a text, editing several lines in the inspector, and the logic sandbox (PIXIE-PROPOSAL-009).

**Dependencies and gates:** none; PIXIE-TODO-030 is completed

**Completion criteria:** (1) runtime tests: aligned lines measured and laid out; a `Text` through a scene and a snapshot; its block at a turned, scaled, and mirrored transform, and with no readable font; picking and the drawing order among sprites; (2) the C API test draws a text into the render with content inside its outline and none outside it, over a sprite of a lower layer and under one of a higher, picks it, and reads its outline; (3) in the editor, a `Text` is added with the project's font, its text, size, and alignment edited, each undone and redone, verified by scripted options and a screenshot, without computer use; (4) the `runtime`, `capi`, and editor documents describe them; (5) zero warnings, and `ctest` passes locally and in the CI.

**Commits:** `62e9789500c403eaf29af03b30a136af3090a160` (aligned lines, the `Text` component drawn and picked among sprites, its outline through the view quad, the editor's asset fields of any kind, and the default font, with their tests); the `runtime`, `capi`, and editor documents are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-140` — **Status:** done — Aligned lines in the font's layout; the `Text` component, its descriptor with a `ttf` font, and its scene form.
- `PIXIE-TASK-141` — **Status:** done — Texts drawn among sprites at their transform, from the ladder's atlas, interpolated; their block, picking, and the view quad in the C API.
- `PIXIE-TASK-142` — **Status:** done — The editor: the `ttf` chooser copying into `fonts/`, a new `Text` taking the project's first font, and the outline; scripted verification.
- `PIXIE-TASK-143` — **Status:** done — Runtime and C API tests, and the `runtime`, `capi`, and editor documents.

## Choices made while building it

- This block was built in a different session from the one that built PIXIE-TODO-030 and promoted it,
  on that block's text code, at the owner's word ("pode fazer o 031 aqui").
- One drawing order for sprites and texts: `ForEachDrawnInOrder` walks the entities by their
  `Transform2D` (oldest first), then sorts by layer, so a text made after a sprite on the same layer
  lies over it. Sprites alone keep the same relative order as before.
- A text's block is measured at one atlas size (64) and scaled, since advances grow linearly with the
  size; drawing measures at the atlas it draws with, so the glyphs and the outline agree to a
  fraction of a pixel.
- Glyphs are sprite quads with their pivot at their top-left, placed in the entity's space and
  transformed, so a negative scale mirrors them in place.
- The C API stays at version 8, as the proposal said: the outline goes through the existing view
  quad. Without a new function, the inspector cannot ask the engine why a font cannot be read; it
  says when the font's file is missing, and the engine logs the exact reason.
- The editor's image-only pieces became general: `ProjectAssets` copies a file of any asset kind into
  its kind's folder (`images/`, `fonts/`), and `AssetChooser` lists the files of the field's kind.
  Adding a `Text` component goes through `Authoring.AddComponent`, which gives a `Text` the project's
  first `.ttf` (in `fonts/` first) in the same step.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, .NET SDK 10.0.401, at `62e9789`.

- Debug and Release built with zero warnings; the editor too, in both. `ctest` passed in both, 11 of
  11, with `project-code`.
- `engine-tests` (20): the new test lays out two lines Left, Center, and Right and finds the second
  line where the widest's width says.
- `runtime-tests` (85): four new tests: the `Text` descriptor and a text through a scene and a
  snapshot; its block doubled with its size and its lines, around its pivot, turned, mirrored, and
  scaled; one em square without a font or characters; the drawing order of sprites and texts by
  layer, and picking the one on top, without texts when no fonts are given.
- `capi-test` on `pixie-scene-capi`: "HELLO", 48 units, magenta, lit 1,234 pixels inside its view
  quad and none outside it, centered where its transform puts it; it was picked at a lit pixel; a
  sprite at the same place lay under it on layer -1 and over it on layer 1 (and was picked there);
  its text came back after saving and opening anew.
- The editor, by `--do` and `--set` on a copy of the sprites sample with Inter copied in by
  `vdr cp inter` (which brought its license): `create`, `remove=Shape`, `add=Text` (its font became
  `fonts/inter-regular.ttf`), undone and redone; `font=` of Inter Bold from the shared resources
  (copied into `fonts/`, one step), undone; `save`; then, opened again, `--set Text.text=Olá, mundo`,
  `size=48`, `align=Center`, and a position, through the inspector, undone and redone, and saved
  (the file holds the accent as UTF-8). The screenshot showed the text with its accent, centered, its
  outline on its block, over the sprite beneath it, and the inspector's font field with its `...`.
- Not run: clicking `...` and choosing in the file list, which need real input; they call the code
  the `font=` command ran. Linux.
- The continuous integration runs on the push that carries this commit; its run is checked then.
