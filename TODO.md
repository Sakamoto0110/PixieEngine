# PixieEngine Promoted Work

**Document ID:** GLOBAL-TODO

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** formally promoted work, gates, dependencies, and completion criteria

**Surface:** work-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

**Last promotion decision:** 2026-10-15

## Purpose and admission rule

This file is the only scheduler. It holds work that the owner has formally
promoted, with enough scope to execute and close it. It does not hold ideas
([`docs/proposals/`](docs/proposals/README.md)), direction
([`ROADMAP.md`](ROADMAP.md)), or completion narratives
([`docs/archive/`](docs/archive/README.md)).

An entry whose scope is large, or that decomposes into many small tasks, keeps
one entry here and its breakdown in [`TASKLIST.md`](TASKLIST.md). The rules are
in the [documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd).

Unchecked or `promoted` entries are not automatically authorized to start; an
entry may keep an explicit gate after promotion.

## Current promoted work

Promoted on 2026-10-15 from [PIXIE-PROPOSAL-008](docs/proposals/interactive-2d-games.md) (engine
features for interactive 2D games), in this order
([decision record](docs/archive/logic-sandbox-decisions-2026-10-15.md)). PIXIE-TODO-028 and
everything before it are completed; see below.

### `PIXIE-TODO-029` — Paths, list fields, and points of a sprite

**Status:** promoted

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-008](docs/proposals/interactive-2d-games.md), items 1, 2, and 5

**Accepted scope:** in the `engine`, a `PathRenderer`: polylines drawn as antialiased segments with round joins and caps from instanced quads, with two colors split at a length along the path. In the `runtime`, a `Path` component (points in the entity's space, width, corner radius, color, fill color, fill fraction) drawn by the session at its transform, after sprites of negative layers and before the other sprites, its corners rounded to the radius; picking within half its width (shapes, then sprites of non-negative layers, then paths, then the other sprites); `FieldKind::PointList` for `std::vector<pxPoint>` members, saved as `[[x, y], ...]`; `SpritePoint` and `SpriteLocal`. In the C API, version 8: point lists read and written, and a path's outline in the view. In the editor: point lists in the inspector (rows of X and Y, add and remove, one step per change) and paths outlined when selected. Out: routing wires, the screen layer, text (PIXIE-TODO-030).

**Dependencies and gates:** none

**Completion criteria:** (1) runtime tests: rounded corners and lengths; a point list through a scene and a snapshot; picking a path and the picking order; `SpritePoint` and `SpriteLocal` turned and mirrored, each the other's inverse; (2) the C API test writes a path's points, renders it, finds both colors on either side of the fill, picks it, and reads its outline; (3) in the editor, a path is added, its points edited, added, and removed, each undone and redone, verified by scripted options, without computer use; (4) the `runtime`, `capi`, and editor documents describe them; (5) zero warnings, and `ctest` passes locally and in the CI.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-029--paths-list-fields-and-points-of-a-sprite)

### `PIXIE-TODO-030` — A screen layer, text, player saves, and fragment metadata

**Status:** promoted

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-008](docs/proposals/interactive-2d-games.md), items 3, 4, 6, and 7

**Accepted scope:** `Project::DrawScreen(ScreenCanvas&)`, drawn after the World in view pixels with sprites, shapes, paths, and text, in the player and the editor's viewport; `stb_truetype` from the pinned `nothings/stb` commit and a `TextRenderer` (atlas per font and pixel size, UTF-8, measuring and drawing lines) in the world and on the screen; `UserDataFolder(projectName)`; an optional `meta` object in entity fragments, written by `WriteEntities`, returned by `ReadEntities`, and read alone by `ReadFragmentMeta`. Out: the logic sandbox (PIXIE-PROPOSAL-009).

**Dependencies and gates:** 1. PIXIE-TODO-029; 2. a font with an open license registered in the shared resources (`vdr`)

**Completion criteria:** (1) tests: text measured and drawn into a render with content where it should be; a fragment's metadata written, read back, and read alone; the user data folder made; (2) a project with code draws a screen layer with text in the player and in the editor's viewport; (3) the documents describe them, and `DEPENDENCIES.md` records `stb_truetype` and the font; (4) zero warnings, and `ctest` passes locally and in the CI.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-030--a-screen-layer-text-player-saves-and-fragment-metadata)

## Completed

`PIXIE-TODO-001` (documentation foundation) completed on 2026-10-11. Its
entry, the policy review, and its verification are preserved in the
[archive record](docs/archive/todo-001-documentation-foundation-2026-10-11.md).

`PIXIE-TODO-002` (main loop and the hello-gl sample) completed on 2026-10-11.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md).

`PIXIE-TODO-003` (PixieLib, GLM, and EnTT vendored) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-003-vendor-pixielib-glm-entt-2026-10-13.md).

`PIXIE-TODO-004` (documentation verifier) completed on 2026-10-13. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-004-documentation-verifier-2026-10-13.md).

`PIXIE-TODO-005` (input, 2D camera, and instanced shapes) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-005-input-camera-shapes-2026-10-13.md).

`PIXIE-TODO-006` (runtime module: World, entities, Behaviours, and phases)
completed on 2026-10-13. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-006-runtime-module-2026-10-13.md).

`PIXIE-TODO-007` (anchor 1: boids steered by the user) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-007-anchor-1-boids-2026-10-13.md).

`PIXIE-TODO-008` (spatial grid and anchor 2: flocks) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-008-spatial-grid-flocks-2026-10-13.md).

`PIXIE-TODO-009` (particles and anchor 3: attractors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-009-particles-attractors-2026-10-13.md).

`PIXIE-TODO-010` (engine side for editor hosts: host loop, render target,
reflection, and `capi`) completed on 2026-10-13. Its entry, breakdown, and
verification are preserved in the
[archive record](docs/archive/todo-010-host-loop-reflection-capi-2026-10-13.md).

`PIXIE-TODO-011` (C# ImGui editor over the anchors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-011-imgui-editor-2026-10-13.md).

`PIXIE-TODO-012` (single source of field metadata) completed on 2026-10-13. Its
entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-012-single-source-metadata-2026-10-13.md).

`PIXIE-TODO-013` (scenario values, scene rebuild, and time control) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-013-scenario-time-control-2026-10-13.md).

`PIXIE-TODO-014` (attractor size and death on contact) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-014-attractor-size-2026-10-13.md).

`PIXIE-TODO-015` (particle systems and spawners apart) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-015-particle-spawners-2026-10-13.md).

`PIXIE-TODO-017` (spawners placed by hand in the particles game) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-017-spawners-by-hand-2026-10-13.md).

`PIXIE-TODO-016` (selection and editing in the viewport) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-016-viewport-selection-2026-10-13.md).

`PIXIE-TODO-018` (save and load scenes) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-018-scenes-2026-10-14.md).

`PIXIE-TODO-019` (projects, edit mode, and play mode in the engine) completed
on 2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-019-projects-2026-10-14.md).

`PIXIE-TODO-020` (new and open projects in the editor) completed on
2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-020-editor-projects-2026-10-14.md).

`PIXIE-TODO-023` (editor polish, the editor's icon, and a viewport menu) was
promoted and completed on 2026-10-14. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-023-editor-polish-2026-10-14.md).

`PIXIE-TODO-021` (code in a project) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-021-project-code-2026-10-14.md).

`PIXIE-TODO-022` (samples as projects) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-022-samples-projects-2026-10-14.md).

`PIXIE-TODO-024` (entity snapshots, scene-ID lookup, and names in the engine)
completed on 2026-10-14. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-024-snapshots-2026-10-14.md).

`PIXIE-TODO-025` (moving, undo and redo, duplicating, deleting, and renaming in
the editor) completed on 2026-10-15. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-025-authoring-2026-10-15.md).

`PIXIE-TODO-026` (continuous integration) was promoted and completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-026-ci-2026-10-15.md).

`PIXIE-TODO-027` (images, sprites, and text fields in the engine) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-027-sprites-engine-2026-10-15.md).

`PIXIE-TODO-028` (sprites in the editor, and a sprites sample) completed on
2026-10-15. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-028-sprites-editor-2026-10-15.md).
