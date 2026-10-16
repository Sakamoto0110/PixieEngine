# PIXIE-TODO-037: Drawing Only What Is on Screen

**Document ID:** ARCHIVE-TODO-037-20261016

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-037: a session submits only the sprites, texts, paths, and shapes that may show in its camera's padded visible area

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-069

**Type:** completion

**Origin:** PIXIE-TODO-037

**Author:** claude

**Reference date:** 2026-10-16

**Commits:** `119f9c16e1eb3cdd6dda69182d3705eb37b532db`, and this record with parent `119f9c16e1eb3cdd6dda69182d3705eb37b532db`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [the runtime reference](../modules/runtime/REFERENCE.md#drawing)

## Entry as completed

### `PIXIE-TODO-037` — Drawing only what is on screen

**Status:** completed

**Source:** owner decision on 2026-10-16 ("conforme for dando zoom, o que não está na tela não precisa ser renderizado")

**Accepted scope:** a session draws only the sprites, texts, paths, and shapes whose bounds meet the camera's visible area (`Camera2D::VisibleWorld`), padded by a few view pixels, the rest skipped before they are submitted; the drawing order, and what is drawn of what is on screen, stay as they were. `SubmitSprites`, `SubmitPaths`, and `SubmitShapes` take the visible area, and draw everything without one, as before; the bounds tests are public (`Culling.hpp`). Picking is unchanged. Out: particles, and the culling of what a game draws itself (its overlay and screen layer), which is the game's.

**Dependencies and gates:** none.

**Completion criteria:** (1) runtime tests: something off screen is not submitted, and what is on screen is submitted as without culling, for shapes and paths, and the bounds of sprites, texts, paths, and shapes; (2) zero warnings in Debug and Release, ctest passing locally and in CI; (3) the runtime reference describes it.

**Breakdown:** below

## Breakdown as completed

- `PIXIE-TASK-162` — **Status:** done — The bounds tests (`Culling.hpp`): a padded visible area, and whether a sprite, a text, a path, or a shape meets it.
- `PIXIE-TASK-163` — **Status:** done — Culling in `SubmitSprites` (with texts), `SubmitPaths`, and `SubmitShapes`, and the session passing its camera's visible area; the renderers count what is pending.
- `PIXIE-TASK-164` — **Status:** done — Tests, and the runtime reference.

## What was built

At `119f9c1`: `Culling.hpp` (`PaddedView`, and `SpriteInView`, `TextInView`, `PathInView`, and
`ShapeInView`, each a conservative bound); `SubmitShapes`, `SubmitPaths`, and `SubmitSprites` take an
optional `view` and keep submitting everything without one; `ProjectSession::Render` passes
`PaddedView(camera, 32)`; `ShapeRenderer`, `PathRenderer`, and `SpriteRenderer` gain
`PendingCount`. Picking, particles, and what a project draws itself are unchanged.

## Verification

- `cmake --build --preset vs2026-debug` and `vs2026-release`: zero warnings; `ctest` 11 of 11 in both.
  runtime-tests passes 90 tests, among them `PaddedViewWidensTheVisibleAreaByViewPixels`,
  `BoundsKeepWhatMayShowAndSkipWhatCannot`, `OffScreenShapesAndPathsAreNotSubmitted`, and
  `HiddenAndCulledAreBothSkipped`.
- Reported by the owner, through the session that asked for it: the editor ran the logic sandbox's
  8-bit adder at 99.9 frames per second (59.9 updates) with culling, against 22 before; the game's own
  culling of its overlay and screen layer was done at the same time in its repository (LogicSandbox
  `e1db521`), so the two share that gain.
- Not run here: a measurement of the engine's culling alone, and the sprite and text paths through
  `SubmitSprites` with images and fonts loaded (their bounds are tested, not their submission), which
  need a GL context.
