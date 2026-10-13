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

Opened on 2026-10-13 for PIXIE-TODO-012 to PIXIE-TODO-016, in the order of
their parents. Completed breakdowns are in their archive records, routed from
the [archive router](docs/archive/README.md).

## `PIXIE-TODO-012` — Single source of field metadata

**Parent:** [PIXIE-TODO-012](TODO.md#pixie-todo-012--single-source-of-field-metadata)

- `PIXIE-TASK-058` — **Status:** open — Field metadata in `ComponentTypes`: builder calls for label, tooltip, help, order, read-only, editor kind, range with step, scrub, display unit, arc spread, and enum names; unit tests.
- `PIXIE-TASK-059` — **Status:** open — C API version 2: `pixie_field_info` and `pixie_field_text`; `capi-test` reads the metadata back.
- `PIXIE-TASK-060` — **Status:** open — Metadata for every reflected field of the runtime and the three anchors: tooltips with units, ranges, scrub steps, degrees for angles, read-only for `Pointer` and `Bounds`.
- `PIXIE-TASK-061` — **Status:** open — Editor on PixieInspector 0.2.0: a field handle (session, entity, type), one inspector per component type built from the descriptors with `AddField` and the node options; remove the proxies, their catalog, and their attributes.
- `PIXIE-TASK-062` — **Status:** open — Verification: each anchor in the editor, a new game component appearing without rebuilding the editor, and the documents (capi and runtime references, editor README, `DEPENDENCIES.md`).

## `PIXIE-TODO-013` — Scenario values, scene rebuild, and time control

**Parent:** [PIXIE-TODO-013](TODO.md#pixie-todo-013--scenario-values-scene-rebuild-and-time-control)

- `PIXIE-TASK-063` — **Status:** open — Scenario structs owned by each anchor's Project, mirrored in `BoidsScenario`, `FlocksScenario`, and `ParticlesScenario` components, read back when the scene is rebuilt; the players' options fill them.
- `PIXIE-TASK-064` — **Status:** open — A scenario mark on component types, exported by the C API; the editor adds a "Rebuild the scene" button to a scenario's inspector.
- `PIXIE-TASK-065` — **Status:** open — Session time scale and `pixie_session_set_time_scale`; editor buttons to slow down, speed up, and pause, with the scale shown.
- `PIXIE-TASK-066` — **Status:** open — Verification: a rebuilt scene with edited counts, measured time at 0.25 and 2 times, the players' options.

## `PIXIE-TODO-014` — Attractor size and death on contact

**Parent:** [PIXIE-TODO-014](TODO.md#pixie-todo-014--attractor-size-and-death-on-contact)

- `PIXIE-TASK-067` — **Status:** open — `Attractor`: `size` (body radius, times the transform's scale), `reach` (renamed from `radius`), and the die-on-contact flag; the particle system removes the particles an affector marks dead, in the same step.
- `PIXIE-TASK-068` — **Status:** open — Tests: contact with the flag on and off, and the orbit test unchanged.
- `PIXIE-TASK-069` — **Status:** open — The particles sample: attractor shapes follow their body; verification in the player and the editor.

## `PIXIE-TODO-015` — Particle systems and spawners apart

**Parent:** [PIXIE-TODO-015](TODO.md#pixie-todo-015--particle-systems-and-spawners-apart)

- `PIXIE-TASK-070` — **Status:** open — `ParticleSystem` (pool, capacity, style, affectors) and its update system, replacing `ParticleEmitter`.
- `PIXIE-TASK-071` — **Status:** open — `ParticleSpawner`: rate, lifetime, speed range, emission shapes (point, circle, ring, rectangle, segment), direction modes (fixed, shape normal, toward a target), direction and spread as an arc.
- `PIXIE-TASK-072` — **Status:** open — The `Entity` field kind in descriptors and the C API; the editor offers the entities that have the expected component.
- `PIXIE-TASK-073` — **Status:** open — Tests for each shape and direction mode and two spawners feeding one pool; the particles sample on spawners; the performance targets measured again.

## `PIXIE-TODO-016` — Selection and editing in the viewport

**Parent:** [PIXIE-TODO-016](TODO.md#pixie-todo-016--selection-and-editing-in-the-viewport)

- `PIXIE-TASK-074` — **Status:** open — C API: picking at a point and in a box, entity destruction, particle removal at a point, `pixie_spawn_at`, and the view bounds of an entity; `Project::SpawnAt` in each anchor.
- `PIXIE-TASK-075` — **Status:** open — Viewport modes, Game and Select, with the click, Shift, Ctrl, and box gestures, and outlines of the selection.
- `PIXIE-TASK-076` — **Status:** open — Multi-selection in the entity list, and inspectors over the components common to the selection, with `AddBind`.
- `PIXIE-TASK-077` — **Status:** open — Verification with real input in each anchor; `capi-test` for the new functions; the documents.
