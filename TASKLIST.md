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

Opened on 2026-10-14 with PIXIE-TODO-018 to PIXIE-TODO-022; the breakdowns
of PIXIE-TODO-018 to PIXIE-TODO-021 are in their archive records. Earlier breakdowns are in their archive records, routed from the
[archive router](docs/archive/README.md).

## `PIXIE-TODO-022` — Samples as projects

**Parent:** PIXIE-TODO-022

- `PIXIE-TASK-100` — **Status:** open — Boids, flocks, and particles each get a `.pixie` file and a scene with their authored entities; generated entities stay generated.
- `PIXIE-TASK-101` — **Status:** open — The three build through `pixie_game`; `samples/common` keeps only what the engine's player does not provide.
- `PIXIE-TASK-102` — **Status:** open — Verification in the editor and the players, and the samples' READMEs.
