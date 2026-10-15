# PIXIE-TODO-027 Completed: Images, Sprites, and Text Fields in the Engine

**Document ID:** ARCHIVE-TODO-027-SPRITES-ENGINE-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-027: PNG images through stb_image, the SpriteRenderer, the Sprite component and the session image cache, text fields in the reflection, and C API version 7

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-048

**Type:** completion

**Origin:** PIXIE-TODO-027 (PIXIE-TASK-120 to PIXIE-TASK-125)

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** `4cff39decb4799eca7caf1d6a8d0dde0446ed710`, `d523beaf853e8db1b5f1bac7f4caf8a17ac52603`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [runtime reference](../modules/runtime/REFERENCE.md#sprites) and [C API reference](../modules/capi/REFERENCE.md)

## Entry as completed

**Status:** completed on 2026-10-15

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-007](../proposals/sprites.md), part 1

**Accepted scope:** `stb_image` vendored at the pinned `nothings/stb` commit, PNG only; in the `engine`, images decoded to premultiplied RGBA8, textures with mipmaps, and a `SpriteRenderer` (instanced quads sorted by layer, batched by texture, a sampler per filter). In the `runtime`, the `Sprite` component (`image`, `region`, `pivot`, `color`, `layer`, `filter`), drawn at one world unit per image pixel times the transform's scale, before particles, shapes, and the overlay; the session's image cache (paths against the project folder or the working directory, sizes, reloading on a changed modification time at most twice a second, a magenta checkerboard and one logged reason for a missing or broken file); picking and view bounds by the sprite's rotated rectangle; `FieldKind::Text` with `Asset` metadata, saved as a JSON string. In the C API, version 7: text fields read and written, a field's asset kind, and an image's texture and size. Out: the editor's use of them (PIXIE-TODO-028), animation, atlases, other formats.

**Dependencies and gates:** none

**Completion criteria:** (1) runtime tests: a PNG decodes with premultiplied alpha, a missing file gives its reason; a text field goes through a scene and a snapshot unchanged; a sprite's region, pivot, and rotated rectangle are picked as described; the cache reloads a file whose modification time changed; (2) the C API test reads and writes a text field and renders a project with a sprite, finding the image's color in the texture; (3) the `engine`, `runtime`, and `capi` documents and `DEPENDENCIES.md` describe them; (4) zero warnings, and `ctest` passes locally and in the CI.

**Commits:** `4cff39decb4799eca7caf1d6a8d0dde0446ed710` (stb_image vendored), `d523beaf853e8db1b5f1bac7f4caf8a17ac52603` (images, textures, the sprite renderer, the `Sprite` component, the image cache, picking, text fields, and C API version 7, with their tests); the `runtime` and `capi` references and `DEPENDENCIES.md` are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-120` — **Status:** done — `stb_image` vendored (PNG only) and recorded in `DEPENDENCIES.md`; images with premultiplied alpha and textures with mipmaps in the `engine`.
- `PIXIE-TASK-121` — **Status:** done — The `SpriteRenderer`: instanced quads, sorted by layer, batched by texture, a sampler per filter, region and pivot.
- `PIXIE-TASK-122` — **Status:** done — `FieldKind::Text` and `Asset` metadata, saved as a JSON string; the `Sprite` component and its descriptor.
- `PIXIE-TASK-123` — **Status:** done — The session's image cache (project-relative paths, sizes, reload on change, the checkerboard), drawing order, picking and view bounds by the rotated rectangle.
- `PIXIE-TASK-124` — **Status:** done — The C API, version 7: text fields, asset kinds, an image's texture and size; runtime and C API tests.
- `PIXIE-TASK-125` — **Status:** done — The `engine`, `runtime`, and `capi` documents.

## Choices made while building it

- `Image.hpp` in the engine decodes PNG from memory (`STBI_NO_STDIO`), so file names, which may be
  UTF-8 on Windows, never reach `fopen`; the screenshot's PNG writer moved there as `EncodePng`,
  which the tests also use to write their images.
- Textures carry no filter: the renderer binds one of two sampler objects per batch and unbinds it
  afterwards, so the editor's ImGui textures on unit 0 are not affected; it also restores the usual
  blend function.
- The drawing order within a layer is the World's (oldest first), the same order picking uses to
  find the sprite on top. Shapes are picked before sprites, since they are drawn over them.
- `SpriteImages` reads images on the CPU, so sizes, picking, and the reasons an image cannot be
  read work without GL (the runtime tests use them so); textures are made on first draw.
- Shape and sprite drawing share `Interpolate` (`Transform2D.hpp`), which also gives the C API the
  transform as last drawn.
- The engine library has no module document yet; its new pieces (`Image`, `Texture`,
  `SpriteRenderer`) are described where the runtime uses them, in the runtime reference's Sprites
  section, and in the runtime manifest's list of what it links.

## Verification

Machine: Windows 11, AMD Radeon RX 7600 (GL 4.5 core), Visual Studio 18 (2026), at `d523bea`.

- Debug and Release built with zero warnings; `ctest` passed in both, 10 of 10 (with
  `project-code`, which built a project with code against the changed engine, in 133 s).
- `runtime-tests`: 71 tests, 12 of them new (PNG decoding with premultiplied alpha and its errors;
  the `Sprite` descriptor; text through a scene, a snapshot, and a wrong value; sizes and problems
  without GL; reading a changed, removed, and appearing file again; sizes from the region, the
  image, and the missing size; the rectangle around the pivot, turned and mirrored; the drawing
  order; picking the sprite on top and shapes first).
- `capi-test` on `pixie-scene-capi`: the sprite drawn from a 4 x 4 PNG written into the project read
  back from the render as exactly (40, 200, 120, 255); picking, corners, bounds, the image's texture
  and size, the error of a missing image, and the path after saving and opening again, as the C API
  reference describes.
- The continuous integration runs on the push that carries these commits; its run is checked then,
  as the documentation policy says.
