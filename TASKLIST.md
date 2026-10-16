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

Opened on 2026-10-15 with PIXIE-TODO-032 and PIXIE-TODO-033, and on 2026-10-16 with PIXIE-TODO-034 and
PIXIE-TODO-035.
PIXIE-TODO-032's, PIXIE-TODO-034's, and PIXIE-TODO-035's breakdowns, and earlier ones, are in their archive records, routed from the
[archive router](docs/archive/README.md).

## `PIXIE-TODO-033` — The logic sandbox, stage 1.5: saves and templates

**Parent:** PIXIE-TODO-033

- `PIXIE-TASK-149` — **Status:** done — Saving and loading the whole circuit with Ctrl + S and Ctrl + O in the `UserDataFolder`; tests. Done on 2026-10-16 in the game's `ef64d11` (Sakamoto0110/LogicSandbox).
- `PIXIE-TASK-150` — **Status:** done — Templates from a box selection, with a name and counts in the fragment's metadata; pasting with new IDs at the mouse; tests. Done on 2026-10-16 in the game's `33bcc7c` (Sakamoto0110/LogicSandbox): the box comes from PIXIE-TODO-035, pasting goes through its copy and paste, and the name is typed from key presses, since the engine's input has no typed text.
- `PIXIE-TASK-151` — **Status:** open — The hotbar's Templates page (Tab, 1 to 9, 0 for the menu, hover details), and the README.
