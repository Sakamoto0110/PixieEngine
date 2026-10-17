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

Opened on 2026-10-17 with PIXIE-TODO-038. Earlier breakdowns, PIXIE-TODO-032 to PIXIE-TODO-037's
among them, are in their archive records, routed from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-038` — Reading many small sets of entities into a large World

**Parent:** PIXIE-TODO-038

- `PIXIE-TASK-165` — **Status:** open — The World's scene entities by scene ID, in the registry's context, kept by hooks on `SceneId` (added, replaced, removed) and built on first use.
- `PIXIE-TASK-166` — **Status:** open — `ReadScene` and `ReadEntities` check and resolve IDs through it, with no copy per call; a set's own IDs are looked up first, so its references name its new entities.
- `PIXIE-TASK-167` — **Status:** open — Tests, and the runtime reference.
