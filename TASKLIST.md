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

Opened on 2026-10-16 with PIXIE-TODO-037. Earlier breakdowns, PIXIE-TODO-032 to PIXIE-TODO-036's
among them, are in their archive records, routed from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-037` — Drawing only what is on screen

**Parent:** PIXIE-TODO-037

- `PIXIE-TASK-162` — **Status:** open — The bounds tests (`Culling.hpp`): a padded visible area, and whether a sprite, a text, a path, or a shape meets it.
- `PIXIE-TASK-163` — **Status:** open — Culling in `SubmitSprites` (with texts), `SubmitPaths`, and `SubmitShapes`, and the session passing its camera's visible area; the renderers count what is pending.
- `PIXIE-TASK-164` — **Status:** open — Tests, and the runtime reference.
