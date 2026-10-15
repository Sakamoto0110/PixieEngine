# PIXIE-TODO-024 Completed: Entity Snapshots, Scene-ID Lookup, and Names in the Engine

**Document ID:** ARCHIVE-TODO-024-SNAPSHOTS-20261014

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-024: entity snapshots in the scene's form, lookup by scene ID, and names, in the runtime and the C API (version 6)

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-043

**Type:** completion

**Origin:** PIXIE-TODO-024 (PIXIE-TASK-107 to PIXIE-TASK-110)

**Author:** claude

**Reference date:** 2026-10-14

**Commits:** `6f3cb703b7dd2ed735a1e5ae2475fde16c87f325`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the `runtime` module's [reference](../modules/runtime/REFERENCE.md#entity-snapshots) and the `capi` module's [reference](../modules/capi/REFERENCE.md#entities-and-fields)

## Entry as completed

**Status:** completed on 2026-10-14

**Source:** owner decision on 2026-10-14; [PIXIE-PROPOSAL-006](../proposals/authoring-in-the-viewport.md), part 1

**Accepted scope:** in the `runtime`, `WriteEntities` writes a set of scene entities as a scene fragment, in the scene format, and `ReadEntities` reads one into a World, with the same scene IDs or with new ones; read with new IDs, `Entity` fields that pointed inside the set point to the new copies and the others keep their targets; unknown components and fields travel with the fragment. In the C API, version 6: an entity's scene ID, the live entity with a scene ID, a snapshot of a set of entities as text, restoring a snapshot (same or new IDs, returning the entities made), and writing an entity's `Name`. Out: the editor's use of them (PIXIE-TODO-025).

**Dependencies and gates:** none

**Completion criteria:** (1) runtime tests: a fragment read back with the same IDs gives the same components and values; read with new IDs, internal references follow the copies and external ones stay; unknown data survives; entities without a scene ID are refused or left out, as the reference states; (2) the C API test snapshots an entity, destroys it, restores it with its ID, and finds it by that ID; duplicates a pair whose one references the other; and renames an entity; (3) the `runtime` and `capi` references describe them; (4) zero warnings, and `ctest` passes.

**Commits:** `6f3cb703b7dd2ed735a1e5ae2475fde16c87f325` (snapshots, scene-ID lookup, names, C API version 6, and tests); the `runtime` and `capi` references are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-107` — **Status:** done — `WriteEntities` and `ReadEntities` in the `runtime`: a fragment in the scene format, read with the same or new scene IDs, internal `Entity` references following new copies, unknown data kept.
- `PIXIE-TASK-108` — **Status:** done — Runtime tests for snapshots: same IDs, new IDs with internal and external references, unknown data, entities without a scene ID.
- `PIXIE-TASK-109` — **Status:** done — The C API, version 6: an entity's scene ID, the entity with a scene ID, snapshot, restore, and writing a name; the C API test covers them.
- `PIXIE-TASK-110` — **Status:** done — The `runtime` and `capi` references.

## Choices made while building it

- A snapshot is a document of the scene's form with its own format name, `pixie-entities`, and the
  scene's version, so it reads and writes through the same code, keeps unknown data the same way,
  and cannot be mistaken for a scene (each reader refuses the other's format).
- `WriteScene` and `ReadScene` now share their per-entity code with the snapshots (writing an
  entity, checking a list, adding entities); a scene reads as before, and its references may also
  name scene entities already in the World.
- Entities without a scene ID are left out of a snapshot with a warning, rather than refused, so a
  selection that mixes scene and generated entities still snapshots the scene's.
- With new IDs, the list's own IDs map to the copies and every other ID to the World's entity, so
  references inside a copied set follow the copies and others keep their targets.
- In the C API, a snapshot is asked for with a null buffer and capacity 0 for its length, then
  written; restoring that fails returns `PIXIE_ERROR_ARGUMENT` with the reason, and adds nothing.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, Visual Studio 2026 (MSVC 14.51), .NET SDK 10.0.401;
commit `6f3cb70`.

- Debug and Release with zero warnings; `ctest` passes 10 of 10 in both. `runtime-tests` has 60
  tests, 5 of them new: destroyed entities brought back with their IDs and values (written again,
  the same text), copies with new IDs following internal references and keeping external ones, a
  copied Behaviour that runs, IDs already in the World refused with nothing added, entities outside
  the scene left out with a warning, and unknown entity keys, components, and fields carried by a
  copy.
- `capi-test`, in every game library: scene ID and lookup, a rename and its removal, a snapshot too
  large for a small buffer refused, a restore refused while the entity exists, then destroyed and
  restored under the same ID with its name and component. In the project run (`pixie-scene-capi`
  and `project-code`): a spawner copied with its system feeds the copy, and copied alone feeds the
  original.
- The editor builds on C API version 6 with zero warnings; it uses none of the new functions yet
  (PIXIE-TODO-025).
- Not run: Linux.
