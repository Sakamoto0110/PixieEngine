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
- `PIXIE-TASK-183` — **Status:** in-progress — D3: presets, admission/minimum guards, full collider dimensions, compound authoring and named collision layers delivered. Native record lists, fixed-slot name configuration, root/compound/mixed-selection checkboxes and history/save/Play/Stop validated with 176 tests per configuration, 15 Release integration scenarios and editor captures. Gravity migration delivered: CharacterMotor2D version 2 scales world gravity, and version 1 saves convert through a descriptor upgrade (no settings, zero and differing legacy values, unknown fields, a frozen movement-lab v1 fixture); 181 tests per configuration and 15 integration scenarios in Debug and Release, with a disabled-upgrade sensitivity failure. Native read-only effective mass/solid area/local center/inertia inspection is delivered through C API 13; single/compound bodies, density/size/override edits, snapshot restoration, invalid-body recovery and Play/Stop have bounded regression coverage. Native collider resize/rotation tools are implemented through C API 14 for boxes/circles/polygons and individual compound items, with 186 runtime tests, 14 Debug and 15 Release CTest suites, and scripted editor history/lifecycle checks. Upright capsule radius/full-height handles are implemented through C API 15 using the shared world-scale-aware native capsule validator; 189 runtime tests, 14 Debug and 15 Release CTest suites, and scripted capsule history/lifecycle checks passed. Native physics debug views are implemented through C API 16: collider/sensor outlines, backend origins, dynamic centers, linear velocities, awake/asleep state and current manifold normals; 193 runtime tests, 14 Debug and 15 Release CTest suites, session isolation, native editor/player captures and sensor sensitivity passed. Scene sync/rebuild, motor, event/pose publication telemetry and retained sync buffers are delivered, with 195 runtime tests, 14 Debug and 15 Release CTest suites. Seeded 0/100/500-body awake/sleeping headless qualification retained 43,200 warm ticks across collection on/off and a disabled-probe reference, plus allocation sensitivity; this is sparse-world CPU evidence, not a universal budget. Physical-pointer acceptance, debug-view owner acceptance and broader dense/mixed/coupled workload qualification remain pending.
- `PIXIE-TASK-184` — **Status:** open — D4: explicit character/body coupling design and platform/pushing/blocked scenarios.
- `PIXIE-TASK-185` — **Status:** open — D5: regressions, migration, stress report, remaining limits and owner feel review.

## `PIXIE-TODO-047` — Each particle carries its look

**Parent:** PIXIE-TODO-047

- `PIXIE-TASK-192` — **Status:** open — Engine: the 40-byte `Particle` with color, half-float size and flags, its filling function and half-float conversion, integer vertex attributes, and the renderer reading the look per particle; tests.
- `PIXIE-TASK-193` — **Status:** open — Runtime: `Emit` with the system's look, the spawner's own look, descriptors; tests; the cost measured before and after; verification with real input in the editor; the runtime reference.
