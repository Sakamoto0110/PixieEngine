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

Opened on 2026-10-16 with PIXIE-TODO-036. Earlier breakdowns, PIXIE-TODO-032 to PIXIE-TODO-035's
among them, are in their archive records, routed from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-036` — The logic sandbox, stage 2: chips and an 8-bit adder

**Parent:** PIXIE-TODO-036

- `PIXIE-TASK-157` — **Status:** open — The engine's `visible` flag on shapes, sprites, texts, and paths, in drawing, picking, descriptors, the reference, and tests.
- `PIXIE-TASK-158` — **Status:** open — In the game: pieces with several outputs (`Wire::fromPin`), In and Out ports, a Clock, and labels.
- `PIXIE-TASK-159` — **Status:** open — Chips: a placed template as a box with labelled pins, its content running inside, semantic zoom, chips inside chips.
- `PIXIE-TASK-160` — **Status:** open — Exploding a chip; saving, copying, deleting, and templates with chips.
- `PIXIE-TASK-161` — **Status:** open — The anchor: half adder, full adder, and 8-bit adder templates, the demo scene, tests, and the README.
