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

Opened on 2026-10-13 for PIXIE-TODO-012 to PIXIE-TODO-016, in the order of
their parents. Completed breakdowns are in their archive records, routed from
the [archive router](docs/archive/README.md).

## `PIXIE-TODO-016` — Selection and editing in the viewport

**Parent:** [PIXIE-TODO-016](TODO.md#pixie-todo-016--selection-and-editing-in-the-viewport)

- `PIXIE-TASK-074` — **Status:** done — C API: picking at a point and in a box, entity destruction, particle removal at a point, `pixie_spawn_at`, and the view bounds of an entity; `Project::SpawnAt` in each anchor.
- `PIXIE-TASK-075` — **Status:** done — Viewport modes, Game and Select, with the click, Shift, Ctrl, and box gestures, and outlines of the selection.
- `PIXIE-TASK-076` — **Status:** in-progress — Multi-selection in the entity list, and inspectors over the components common to the selection, with `AddBind`.
- `PIXIE-TASK-077` — **Status:** in-progress — Verification with real input in each anchor; `capi-test` for the new functions; the documents.
