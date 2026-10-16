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

Opened on 2026-10-15 with PIXIE-TODO-032 and PIXIE-TODO-033. Earlier breakdowns are in their
archive records, routed from the [archive router](docs/archive/README.md).

## `PIXIE-TODO-032` — The logic sandbox, stage 1: light the lamp

**Parent:** PIXIE-TODO-032

- `PIXIE-TASK-144` — **Status:** open — The project: made with code in `C:\Users\Sakamoto\dev\LogicSandbox`, the sprites, `gates.json`, and Inter copied in with `vdr cp`; its components described for the editor; its tests' target.
- `PIXIE-TASK-145` — **Status:** open — The logic: gates and their truth tables, the power source, the lamp, the relay, signals travelling down wires, evaluation one step after an input changes; tests.
- `PIXIE-TASK-146` — **Status:** open — Pins at the `gates.json` anchors, turned and mirrored; wires routed through bends with rounded corners; the wiring rules; moving pieces re-routes their wires; tests.
- `PIXIE-TASK-147` — **Status:** open — The hotbar on the screen layer, hover names, keys 1 to 9, placing, selecting and moving, R and F, toggling the source, the lamp's message; gestures tested with synthetic input.
- `PIXIE-TASK-148` — **Status:** open — A saved circuit with a lit lamp, the player's screenshot, the editor opening the project, the README, and the list of what real input alone can check.

## `PIXIE-TODO-033` — The logic sandbox, stage 1.5: saves and templates

**Parent:** PIXIE-TODO-033

- `PIXIE-TASK-149` — **Status:** open — Saving and loading the whole circuit with Ctrl + S and Ctrl + O in the `UserDataFolder`; tests.
- `PIXIE-TASK-150` — **Status:** open — Templates from a box selection, with a name and counts in the fragment's metadata; pasting with new IDs at the mouse; tests.
- `PIXIE-TASK-151` — **Status:** open — The hotbar's Templates page (Tab, 1 to 9, 0 for the menu, hover details), and the README.
