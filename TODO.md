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

Promoted on 2026-10-15 from [PIXIE-PROPOSAL-007](docs/proposals/sprites.md) (sprites), in this
order ([decision record](docs/archive/sprites-decisions-2026-10-15.md)). PIXIE-TODO-026 and
everything before it are completed; see below.

### `PIXIE-TODO-027` — Images, sprites, and text fields in the engine

**Status:** promoted

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-007](docs/proposals/sprites.md), part 1

**Accepted scope:** `stb_image` vendored at the pinned `nothings/stb` commit, PNG only; in the `engine`, images decoded to premultiplied RGBA8, textures with mipmaps, and a `SpriteRenderer` (instanced quads sorted by layer, batched by texture, a sampler per filter). In the `runtime`, the `Sprite` component (`image`, `region`, `pivot`, `color`, `layer`, `filter`), drawn at one world unit per image pixel times the transform's scale, before particles, shapes, and the overlay; the session's image cache (paths against the project folder or the working directory, sizes, reloading on a changed modification time at most twice a second, a magenta checkerboard and one logged reason for a missing or broken file); picking and view bounds by the sprite's rotated rectangle; `FieldKind::Text` with `Asset` metadata, saved as a JSON string. In the C API, version 7: text fields read and written, a field's asset kind, and an image's texture and size. Out: the editor's use of them (PIXIE-TODO-028), animation, atlases, other formats.

**Dependencies and gates:** none

**Completion criteria:** (1) runtime tests: a PNG decodes with premultiplied alpha, a missing file gives its reason; a text field goes through a scene and a snapshot unchanged; a sprite's region, pivot, and rotated rectangle are picked as described; the cache reloads a file whose modification time changed; (2) the C API test reads and writes a text field and renders a project with a sprite, finding the image's color in the texture; (3) the `engine`, `runtime`, and `capi` documents and `DEPENDENCIES.md` describe them; (4) zero warnings, and `ctest` passes locally and in the CI.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-027--images-sprites-and-text-fields-in-the-engine)

### `PIXIE-TODO-028` — Sprites in the editor, and a sprites sample

**Status:** promoted

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-007](docs/proposals/sprites.md), part 2

**Accepted scope:** dropping PNG files onto the viewport makes one sprite per file where they fall, named after the file, selected, as one step; a file outside the project is copied into the project's `images/` first, said in the status line. "New sprite..." in the viewport's menu picks a PNG and places it at the click. An asset field in the inspector shows its path, a `...` button picking a file of its kind from the project folder, and a preview of the image; editing it is a step. `samples/sprites`, a project with only a scene, shows the neko mark from the shared resources tinted, layered, rotated, with both filters, and with a region. Out: rotating and scaling by hand, animation.

**Dependencies and gates:** 1. PIXIE-TODO-027

**Completion criteria:** (1) in the editor, dropping, the menu, the asset field, and its undo work, verified through options for scripted runs, since the owner asked that computer use not be used, with what could not be run stated; (2) the sample opens in the editor and runs in the player; (3) the editor's README and the samples' documents describe them; (4) zero warnings.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-028--sprites-in-the-editor-and-a-sprites-sample)

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
