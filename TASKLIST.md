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

## `PIXIE-TODO-043` — Rigid body dynamics

**Parent:** PIXIE-TODO-043

- `PIXIE-TASK-180` — **Status:** done — D0: pin ownership, units, compatibility bridge and initial contracts in the body dynamics guide; full editor migration remains tracked in D3.
- `PIXIE-TASK-181` — **Status:** done — D1: native/scene body baseline, world settings, mass/rotation/commands and lifecycle; 137 runtime tests per configuration and 13 Release integration scenarios passed. Compound scene-list authoring remains D3.
- `PIXIE-TASK-182` — **Status:** done — D2: copied contact/sensor lifetimes, scene identities, sensor-inclusive queries and bounded material/stack/fast-body qualification; 157 runtime tests per configuration, omitted-sensor sensitivity and 13 Release integration scenarios passed. Transient moving-wall overlap is documented.
- `PIXIE-TASK-183` — **Status:** in-progress — D3: presets, admission/minimum guards, full collider dimensions, compound authoring and named collision layers delivered. Native record lists, fixed-slot name configuration, root/compound/mixed-selection checkboxes and history/save/Play/Stop validated with 176 tests per configuration, 15 Release integration scenarios and editor captures. Gravity migration delivered: CharacterMotor2D version 2 scales world gravity, and version 1 saves convert through a descriptor upgrade (no settings, zero and differing legacy values, unknown fields, a frozen movement-lab v1 fixture); 181 tests per configuration and 15 integration scenarios in Debug and Release, with a disabled-upgrade sensitivity failure. Manipulation tools, mass/debug views and full telemetry acceptance remain.
- `PIXIE-TASK-184` — **Status:** open — D4: explicit character/body coupling design and platform/pushing/blocked scenarios.
- `PIXIE-TASK-185` — **Status:** open — D5: regressions, migration, stress report, remaining limits and owner feel review.
