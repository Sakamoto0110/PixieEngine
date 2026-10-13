# PIXIE-TODO-012: Single Source of Field Metadata

**Document ID:** ARCHIVE-TODO-012-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-012, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-027

**Type:** completion

**Origin:** PIXIE-TODO-012 (PIXIE-TASK-058 to PIXIE-TASK-062)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `057f92d398336919eb7eec65782849eb9ecd3eba`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [runtime reference](../modules/runtime/REFERENCE.md#reflection), the [capi reference](../modules/capi/REFERENCE.md#entities-and-fields), and [`editor/README.md`](../../editor/README.md)

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** owner decision on 2026-10-13: the native registry is the single source of field metadata, "even if it costs more"

**Accepted scope:** `ComponentTypes` fields carry the metadata an inspector needs, in the vocabulary of PixieInspector's node options: label, tooltip, help, order, read-only, editor kind, range (minimum, maximum, step), scrub (multiplier, axis), display unit (suffix, factor, offset), the arc's spread sibling, and enum names; range and step are in the displayed unit. The C API goes to version 2 and exports them (`pixie_field_info` for the numbers, `pixie_field_text` for the strings). The editor moves to PixieInspector 0.2.0 and builds one inspector per component type from the descriptors, with `AddField` over a handle (session, entity, component type); the hand-written proxies, their catalog, and their attributes are removed. Every reflected field of the runtime and the anchors gets a tooltip with its unit, a range, and a scrub step; angles show in degrees; fields a project rewrites every step (`Pointer`, `Bounds`) are read-only.

**Dependencies and gates:** none

**Completion criteria:** (1) the editor inspects every reflected component of the three anchors with no C# class per component, and a component added to a game's library appears without rebuilding the editor; (2) tooltips, ranges, scrub, and degrees show as described, and editing still changes the running World; (3) `capi-test` reads the metadata back; (4) zero warnings in the engine and the editor.

**Commits:** `057f92d398336919eb7eec65782849eb9ecd3eba` (runtime, C API, anchors, editor, and tests); the references, the editor README, and `DEPENDENCIES.md` are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-058` — **Status:** done — Field metadata in `ComponentTypes`: builder calls for label, tooltip, help, order, read-only, editor kind, range with step, scrub, display unit, arc spread, and enum names; unit tests.
- `PIXIE-TASK-059` — **Status:** done — C API version 2: `pixie_field_info` and `pixie_field_text`; `capi-test` reads the metadata back.
- `PIXIE-TASK-060` — **Status:** done — Metadata for every reflected field of the runtime and the three anchors: tooltips with units, ranges, scrub steps, degrees for angles, read-only for `Pointer` and `Bounds`.
- `PIXIE-TASK-061` — **Status:** done — Editor on PixieInspector 0.2.0: a field handle (session, entity, type), one inspector per component type built from the descriptors with `AddField` and the node options; remove the proxies, their catalog, and their attributes.
- `PIXIE-TASK-062` — **Status:** done — Verification: each anchor in the editor, a new game component appearing without rebuilding the editor, and the documents (capi and runtime references, editor README, `DEPENDENCIES.md`).

## Verification

Machine: Windows 11, AMD Radeon RX 7600, .NET SDK 10.0.401, Visual Studio
2026 (MSVC 14.51); commit `057f92d`.

- Engine Debug and Release, and the editor Debug and Release (warnings as
  errors): zero warnings. `ctest`: 5 of 5 pass; `runtime-tests` has 29 tests,
  including the metadata builder and a tooltip on every runtime field;
  `capi-test` checks `sizeof(PixieFieldInfo) == 72`, the degrees, range, scrub,
  and suffix of `Transform2D.rotation`, the label fallback, an unknown text
  kind, and a tooltip on every field of every type, for each anchor.
- The editor, from saved frames of real runs:
  - boids, `controls` selected: labels from the metadata ("max speed",
    "cruise speed"); `wander` shown as 143.239 °/s (2.5 rad/s); `Pointer` and
    `Bounds` grayed out as read-only; points split into "position x" and
    "position y", the rectangle into four fields. `--set
    BoidControls.mode=Repel` and `--set BoidControls.wander=90` went through
    the inspector nodes and show.
  - Real mouse: dragging the "radius" label 50 pixels to the right took it
    from 220 to 270 (scrub 1 per pixel); hovering "max speed" showed its
    tooltip, "Fastest a boid flies, in world units per second."
  - particles and flocks: every component inspected the same way;
    `ParticleEmitter.spread` set to 90 shows 90 °; `FlockControls.cohesionWeight`
    set to 2.5 shows under the label "cohesion".
- A component that exists only in a rebuilt game library (a temporary
  `ProbeOnly` with a number and a flag, added to the boids game and removed
  afterwards) appeared in the editor, with its tooltip and an edit through
  `--set`, while the editor's own assembly was older than the game's library.
- `--set` values are in the displayed unit: PixieInspector's `SetValue`
  converts a shown number back with the unit (`ToHeld`).
- No C# class per component remains: `Inspection/` holds `ComponentHandle`
  and `InspectorFactory` only.
- Not run: the Arc editor (no field uses it until the spawners of
  PIXIE-TODO-015), vertical scrubbing, another platform.
