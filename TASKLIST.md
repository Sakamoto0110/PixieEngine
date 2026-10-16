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

Opened on 2026-10-15 with PIXIE-TODO-031. Earlier breakdowns are in their archive records, routed
from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-031` — Text in the scene

**Parent:** PIXIE-TODO-031

- `PIXIE-TASK-140` — **Status:** open — Aligned lines in the font's layout; the `Text` component, its descriptor with a `ttf` font, and its scene form.
- `PIXIE-TASK-141` — **Status:** open — Texts drawn among sprites at their transform, from the ladder's atlas, interpolated; their block, picking, and the view quad in the C API.
- `PIXIE-TASK-142` — **Status:** open — The editor: the `ttf` chooser copying into `fonts/`, a new `Text` taking the project's first font, and the outline; scripted verification.
- `PIXIE-TASK-143` — **Status:** open — Runtime and C API tests, and the `runtime`, `capi`, and editor documents.
