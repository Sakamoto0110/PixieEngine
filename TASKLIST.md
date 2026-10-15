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

Opened on 2026-10-15 with PIXIE-TODO-026. Earlier breakdowns are in their
archive records, routed from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-026` — Continuous integration

**Parent:** PIXIE-TODO-026

- `PIXIE-TASK-116` — **Status:** open — The `ci` presets (Visual Studio 2022) and the workflow: documentation, engine (Debug and Release, built and tested), and editor.
- `PIXIE-TASK-117` — **Status:** open — Mesa's llvmpipe for the tests that open an OpenGL 4.5 window, pinned and checked; or, if it cannot serve them, a label that skips them in the CI with the reason in the log.
- `PIXIE-TASK-118` — **Status:** open — Whatever the runners' compiler or environment reports, fixed in the code, until the run is green.
- `PIXIE-TASK-119` — **Status:** open — The badge, `docs/development.md`, and the documentation policy's note on CI.
