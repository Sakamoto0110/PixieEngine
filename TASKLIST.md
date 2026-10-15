# PixieEngine Task List

**Document ID:** GLOBAL-TASKLIST

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** task breakdowns of promoted TODO.md entries

**Surface:** task-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

## Purpose

This file is a child scheduler of [`TODO.md`](TODO.md). Each section breaks
one promoted `TODO.md` entry into tasks, so that large entries, or entries made
of many small tasks, do not pollute `TODO.md`.

- Every section names its parent in `Parent`, and the parent links back here
  in `Breakdown`.
- Tasks decompose the parent's accepted scope and never extend it.
- When the parent completes, its section moves to
  [`docs/archive/`](docs/archive/README.md).

The full rules are in the
[documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd),
and the section shape is in the
[template](docs/templates/TASKLIST-SECTION.md).

## Open breakdowns

Opened on 2026-10-15 with PIXIE-TODO-027 and PIXIE-TODO-028. Earlier breakdowns are in their
archive records, routed from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-027` — Images, sprites, and text fields in the engine

**Parent:** PIXIE-TODO-027

- `PIXIE-TASK-120` — **Status:** open — `stb_image` vendored (PNG only) and recorded in `DEPENDENCIES.md`; images with premultiplied alpha and textures with mipmaps in the `engine`.
- `PIXIE-TASK-121` — **Status:** open — The `SpriteRenderer`: instanced quads, sorted by layer, batched by texture, a sampler per filter, region and pivot.
- `PIXIE-TASK-122` — **Status:** open — `FieldKind::Text` and `Asset` metadata, saved as a JSON string; the `Sprite` component and its descriptor.
- `PIXIE-TASK-123` — **Status:** open — The session's image cache (project-relative paths, sizes, reload on change, the checkerboard), drawing order, picking and view bounds by the rotated rectangle.
- `PIXIE-TASK-124` — **Status:** open — The C API, version 7: text fields, asset kinds, an image's texture and size; runtime and C API tests.
- `PIXIE-TASK-125` — **Status:** open — The `engine`, `runtime`, and `capi` documents.

## `PIXIE-TODO-028` — Sprites in the editor, and a sprites sample

**Parent:** PIXIE-TODO-028

- `PIXIE-TASK-126` — **Status:** open — Asset fields in the inspector: path, `...` picker from the project folder, preview, a step per edit.
- `PIXIE-TASK-127` — **Status:** open — Dropping PNG files onto the viewport, copying outside files into `images/`, and "New sprite..." in the viewport's menu, each one step.
- `PIXIE-TASK-128` — **Status:** open — `samples/sprites`, with the neko mark from `vdr`.
- `PIXIE-TASK-129` — **Status:** open — Scripted verification, the editor's README, and the samples' documents.
