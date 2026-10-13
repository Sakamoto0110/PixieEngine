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

Opened on 2026-10-13 for PIXIE-TODO-003 to PIXIE-TODO-011, in the order of
their parents. Completed breakdowns are in their archive records, as the one of
PIXIE-TODO-002 is in its
[archive record](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md).

## `PIXIE-TODO-011` — C# ImGui editor over the anchors

**Parent:** [PIXIE-TODO-011](TODO.md#pixie-todo-011--c-imgui-editor-over-the-anchors)

- `PIXIE-TASK-052` — **Status:** open — `editor/` project and solution: `PixieInspector.ImGui`, the GLFW and OpenGL3 backends of Hexa.NET.ImGui, an OpenGL 4.5 core window, and docking.
- `PIXIE-TASK-053` — **Status:** open — P/Invoke bindings over `capi` with source-generated `LibraryImport`, loading the library of the chosen anchor.
- `PIXIE-TASK-054` — **Status:** open — Viewport: the World's texture in a docked window with `ImGui.Image`, and the mouse forwarded in viewport coordinates.
- `PIXIE-TASK-055` — **Status:** open — Entity list and inspector: hand-written proxies whose properties call into `capi`, bound by PixieInspector.
- `PIXIE-TASK-056` — **Status:** open — Pause, resume, single step, and reset.
- `PIXIE-TASK-057` — **Status:** open — Its `README.md`; verification with each of the three anchors.
