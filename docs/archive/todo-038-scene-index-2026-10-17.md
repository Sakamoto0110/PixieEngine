# PIXIE-TODO-038: Reading Many Small Sets of Entities into a Large World

**Document ID:** ARCHIVE-TODO-038-20261017

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-038: the World keeps its scene entities by scene ID, so reading a set of entities costs what the set holds, not the World's size

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-073

**Type:** completion

**Origin:** PIXIE-TODO-038

**Author:** claude

**Reference date:** 2026-10-17

**Commits:** `51a731bd42cf5841fb5f88b70ea7817156e64ae1`, and this record with parent `51a731bd42cf5841fb5f88b70ea7817156e64ae1`; the test fix `c4d0571870eb758232a322ca0324279c73d94774`, and its note below with parent `c4d0571870eb758232a322ca0324279c73d94774`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [the runtime reference](../modules/runtime/REFERENCE.md#entity-snapshots)

## Entry as completed

### `PIXIE-TODO-038` — Reading many small sets of entities into a large World

**Status:** completed

**Source:** owner request on 2026-10-17, through the session working on the logic sandbox's calculator, whose load spends about 104 s in `ReadEntities` (2272 chips, about 30,000 pieces)

**Accepted scope:** the World keeps its scene entities by scene ID, kept current as `SceneId` components are added, replaced, and removed, and built the first time a read needs it; `ReadScene` and `ReadEntities` check and resolve IDs through it instead of building a map of every scene entity on each call, so a read costs what its text holds, not the World's size. `SceneIds::Keep` and `SceneIds::New`, their checks against the World, and a set's references to its own entities naming their copies, stay as they were. Out: the C API's `pixie_entity_find`, which still scans.

**Dependencies and gates:** none.

**Completion criteria:** (1) runtime tests: the index follows entities destroyed, scene IDs removed and replaced, and repeated IDs; reading many small sets into a large World takes about as long as into a small one; the snapshot tests pass unchanged; (2) zero warnings in Debug and Release, ctest passing locally; (3) the runtime reference states the cost.

**Breakdown:** below

## Breakdown as completed

- `PIXIE-TASK-165` — **Status:** done — The World's scene entities by scene ID, in the registry's context, kept by hooks on `SceneId` (added, replaced, removed) and built on first use.
- `PIXIE-TASK-166` — **Status:** done — `ReadScene` and `ReadEntities` check and resolve IDs through it, with no copy per call; a set's own IDs are looked up first, so its references name its new entities.
- `PIXIE-TASK-167` — **Status:** done — Tests, and the runtime reference.

## What was built

At `51a731b`, in `runtime/src/Scene.cpp`: `detail::SceneIndex`, a map from scene ID to entity in the
registry's context. The first `ReadScene` or `ReadEntities` on a World builds it from the `SceneId`
storage and connects three hooks: an added `SceneId` enters it, a removed one (alone, or with its
entity, including when the World is cleared) leaves it, and a replaced one marks it to be built again,
since the ID it replaced is not known. When two entities hold one ID, which only a game adding one by
hand can cause, a removal also marks it to be built again, so the other is still found. Reads use it
in place: `CheckEntities` and the new-ID draw check against it, and `AddEntities` resolves an Entity
field through a `SceneLookup`, the set's own entities under the IDs they were written with first,
then the World's, as the per-call map did. `Scene.hpp` states the cost, and that a `SceneId` is
changed through `Entity::Add`, never in place. Writing scenes and sets, and the C API, are unchanged.

## Verification

- `cmake --build --preset vs2026-debug` and `vs2026-release`: zero warnings; `ctest` 11 of 11 in both.
  runtime-tests passes 92 tests, among them the snapshot tests unchanged, and two new ones:
  `TheWorldsSceneIdsFollowItsEntities` (destroyed entities, removed, replaced, and repeated IDs) and
  `ReadingASetCostsWhatItHoldsNotTheWorldsSize` (200 reads of a 4-entity set into Worlds of 1,000 and
  100,000 scene entities: 0.015 s and 0.014 s in Release, 0.314 s and 0.328 s in Debug).
- The checks catch the error: with the map built again on every read, the second test took 29.4 s
  into 100,000 against 0.7 s into 1,000, and failed; without the repeated-ID rule, or without the
  replace hook, the first test failed.
- `python scripts/check_docs.py` reports no problem, and `git diff --check` is clean.
- Done by the procedure of the [development guide](../development.md#void-agent-workplace): VAW at
  `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`, the explicit registry, `pixieengine`/1, the routes
  `runtime` and `documentation` resolved `bound_scoped`.
- A fault found afterwards, fixed at `c4d0571`: run against the build without the replace hook,
  `TheWorldsSceneIdsFollowItsEntities` read a read's first entity after the check on their count had
  failed, and the Debug runtime's "vector subscript out of range" assertion opened a modal dialog on
  the owner's screen. The test now stops when a read added none, and `tests/TestMain.cpp` sends the
  debug runtime's assertions and errors to stderr with no abort dialog, as the logic sandbox's tests
  do. Run again against that broken build, the test failed with exit code 1 and no dialog; on the
  fixed code, `ctest` 11 of 11 in Debug and Release, zero warnings. The C API's `capi-test` and the
  libraries it loads, each with its own runtime under `/MT`, keep their default reporting.
- Not run here: the logic sandbox's calculator load against this engine, which its session measures;
  continuous integration, since nothing was pushed; and the editor's build, which the change does not
  touch.
