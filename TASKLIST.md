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

Opened on 2026-10-14 with PIXIE-TODO-024 and PIXIE-TODO-025; the breakdown of PIXIE-TODO-024 and
earlier ones are in their archive records, routed from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-025` — Moving, undo and redo, duplicating, deleting, and renaming in the editor

**Parent:** PIXIE-TODO-025

- `PIXIE-TASK-111` — **Status:** open — The history: steps by scene ID, Ctrl + Z, Ctrl + Y, Ctrl + Shift + Z, an Edit menu naming the step; inspector edits recorded one step per press-to-release; nothing recorded while playing.
- `PIXIE-TASK-112` — **Status:** open — Dragging to move in Select mode, the whole selection when it was selected, Shift for one axis, and the Snap toggle with its grid; each drag one step.
- `PIXIE-TASK-113` — **Status:** open — Duplicate (Ctrl + D and the viewport's menu) and Delete (the key, from the viewport or the entity list), each one step; creating, adding, and removing components recorded too.
- `PIXIE-TASK-114` — **Status:** open — Renaming in the inspector, one step per rename.
- `PIXIE-TASK-115` — **Status:** open — Verification (real input or scripted options, as allowed) and the editor's README.
