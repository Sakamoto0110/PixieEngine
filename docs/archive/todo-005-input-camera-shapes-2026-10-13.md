# PIXIE-TODO-005: Input, 2D Camera, and Instanced Shapes

**Document ID:** ARCHIVE-TODO-005-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-005, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-017

**Type:** completion

**Origin:** PIXIE-TODO-005 (PIXIE-TASK-020 to PIXIE-TASK-025)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `d2c5205b9170e1ea4b848dd8d1520d9b934c8410`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the public headers under `engine/include/pixie/`, [`samples/shapes/README.md`](../../samples/shapes/README.md), and the [development guide](../development.md)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** [PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md), anchors 1 to 3

**Accepted scope:** in the provisional engine library: input state (keys, mouse buttons, cursor position, scroll) fed by the player's GLFW window and readable by the App, with presses that are not lost between fixed updates; a 2D camera (position and zoom, screen-to-world and world-to-screen, an orthographic projection) on PixieLib math; small OpenGL helpers (shader programs, buffers); an instanced renderer for triangles, circles, and quads with per-instance position, rotation, scale, and color. A `shapes` sample draws thousands of shapes, pans and zooms the camera with the mouse, and marks the cursor's world position. The `platform` and `render` boundaries stay undecided, and this code stays provisional.

**Dependencies and gates:** 1. PIXIE-TODO-003 (PixieLib math)

**Completion criteria:** (1) Debug and Release build with zero warnings; (2) the `shapes` sample draws 10,000 instanced shapes, pans and zooms, and its cursor marker stays under the cursor at every zoom; (3) its measured frame and update rates are recorded with machine, configuration, and commit; (4) its `README.md` describes it.

**Commits:** `d2c5205b9170e1ea4b848dd8d1520d9b934c8410` (engine, samples, and tests); the sample README and the guides are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-020` — **Status:** done — Input state fed by GLFW in the player loop: keys, mouse buttons, cursor position, and scroll, with presses and releases kept until the next fixed update reads them.
- `PIXIE-TASK-021` — **Status:** done — OpenGL helpers: a shader program built from sources, with errors logged, and owning wrappers for buffers and vertex arrays.
- `PIXIE-TASK-022` — **Status:** done — `Camera2D` on PixieLib math: position, zoom, viewport size, view-projection matrix, and screen-to-world and world-to-screen conversion.
- `PIXIE-TASK-023` — **Status:** done — Instanced shape renderer: triangles, circles (distance-based edge in the fragment shader), and quads, with per-instance position, rotation, scale, and color, drawn with one call per shape kind.
- `PIXIE-TASK-024` — **Status:** done — `shapes` sample: 10,000 moving shapes, panning by dragging and zooming around the cursor with the scroll wheel, a marker at the cursor's world position; its `README.md`.
- `PIXIE-TASK-025` — **Status:** done — Verification: both configurations, the cursor marker at several zoom levels, and measured rates recorded with machine, configuration, and commit.

## Verification

Machine: Windows 11, AMD Radeon RX 7600 (driver 32.0.31041.1004), monitor at
100 Hz, Visual Studio 2026 (MSVC 14.51), CMake 4.3.1; commit `d2c5205`.

- Debug and Release build with `/W4 /WX`: zero warnings in the project's own
  targets (`pixie_engine`, `pixie_sample_common`, `shapes`, `hello-gl`,
  `engine-tests`).
- `ctest` in Debug and Release: `engine-tests` passes, 10 tests. Each was seen
  failing on a deliberately broken copy, restored afterwards: with the clip
  space Y flip removed and `ZoomAt` no longer moving the center, the camera
  tests `GpuMatrixAgreesWithScreenToWorld` and `ZoomAtKeepsThePointUnderTheCursor`
  fail; with `EndUpdate` no longer clearing presses, two input tests fail.
- Real input, through the operating system and GLFW's callbacks: a script
  placed the cursor at client pixel (896, 252) of the Release `shapes` window
  and turned the wheel twelve notches. In the saved last frame the view is
  zoomed in about 5.3 times and the cursor ring sits at (896, 252). A second
  run held the left button at the middle of the window and dragged it by
  (+300, +150): the world's border moved from (297, 17) to (597, 167), and the
  ring followed the cursor to (940, 510). Triangles, discs, rings, and squares
  are all visible in the zoomed frame.
- Measured rates of the Release `shapes` sample, from the engine's log, in
  6-second runs, at 60 updates per second:

  | Configuration | Shapes | Frames per second |
  |---|---|---|
  | vsync on | 10,000 | 100.0 |
  | vsync off | 10,000 | 6,070 to 6,180 |
  | vsync off | 100,000 | 858 to 882 |

  Updates per second stay at 60, within about one between one-second windows.
- `hello-gl --quit-after 2` still runs at about 100 frames and 60 updates per
  second and exits 0.
- Added during the work, as an aid to verification: `SaveFramebufferPng` and
  `RunConfig::screenshotPath` (the samples' `--screenshot`), and the samples'
  shared option parser. The option for the tests is `PIXIE_ENGINE_BUILD_TESTS`,
  because PixieLib already defines `PIXIE_BUILD_TESTS`.
- Not run: another GPU or machine, a display with system scaling (the cursor
  conversion from window to framebuffer pixels is untested there), and the
  middle mouse button.
