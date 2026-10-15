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

Opened on 2026-10-15 with PIXIE-TODO-029 and PIXIE-TODO-030; PIXIE-TODO-029's is in its archive
record. Earlier breakdowns are in their archive records, routed from the
[archive router](docs/archive/README.md).

## `PIXIE-TODO-030` — A screen layer, text, player saves, and fragment metadata

**Parent:** PIXIE-TODO-030

- `PIXIE-TASK-136` — **Status:** open — `Project::DrawScreen` and the `ScreenCanvas`, in the session, the player, and the editor's viewport.
- `PIXIE-TASK-137` — **Status:** open — `stb_truetype` and the font recorded; the `TextRenderer`, in the world and on the screen.
- `PIXIE-TASK-138` — **Status:** open — `UserDataFolder`; `meta` in entity fragments and `ReadFragmentMeta`.
- `PIXIE-TASK-139` — **Status:** open — Tests, a project drawing text on its screen layer, and the documents.
