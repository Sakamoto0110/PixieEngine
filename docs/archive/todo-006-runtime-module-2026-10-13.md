# PIXIE-TODO-006: Runtime Module

**Document ID:** ARCHIVE-TODO-006-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completed PIXIE-TODO-006, its task breakdown, and its verification

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-018

**Type:** completion

**Origin:** PIXIE-TODO-006 (PIXIE-TASK-026 to PIXIE-TASK-032)

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** `6c9bd8bafd1bd552dc6ba46b912fd8b9e54a26d9`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** [`docs/modules/runtime/`](../modules/runtime/README.md) and the public headers under `runtime/include/pixie/runtime/`

## Entry as completed

**Status:** completed on 2026-10-13

**Source:** [PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md), part 1; the `runtime` boundary of [PIXIE-PROPOSAL-001](../proposals/engine-module-boundaries.md)

**Accepted scope:** the `runtime` module, with its `MANIFEST.md`, `README.md`, and `REFERENCE.md`: a World over an EnTT registry, with its own time and a seeded random number generator; an EC-style entity handle (create, destroy, and add, get, has, and remove components); `Behaviour` classes with `Update()`, stored as components; the update phases `PreUpdate`, `Update` (Behaviours), and `PostUpdate`, with engine systems registered per phase; destruction deferred to the end of a step; the built-in components `Transform2D`, `Velocity2D`, and `Shape`, with the previous transform kept for render interpolation; and a render system that draws shapes through the instanced renderer. Tests run through CTest.

**Dependencies and gates:** 1. PIXIE-TODO-003 (EnTT, PixieLib); 2. PIXIE-TODO-005 (renderer)

**Completion criteria:** (1) Debug and Release build with zero warnings; (2) tests pass for phase order, Behaviour lifecycle, deferred destruction, and identical random sequences from the same seed, and a deliberately broken copy of each makes its test fail; (3) the module's documents exist and match the public headers.

**Commits:** `6c9bd8bafd1bd552dc6ba46b912fd8b9e54a26d9` (the module and its tests); the module documents are added with this record.

## Breakdown as completed

- `PIXIE-TASK-026` — **Status:** done — Create the `runtime` module: the `runtime/` library target and `docs/modules/runtime/` with `MANIFEST.md`, `README.md`, and `REFERENCE.md`.
- `PIXIE-TASK-027` — **Status:** done — `World` over an EnTT registry and an `Entity` handle: create, destroy, and add, get, has, and remove components.
- `PIXIE-TASK-028` — **Status:** done — Update phases: engine systems registered for `PreUpdate` and `PostUpdate`, Behaviours run in `Update`, and destruction deferred to the end of the step.
- `PIXIE-TASK-029` — **Status:** done — `Behaviour`: a base class with `Start`, `Update`, and `Stop`, stored as a component, with access to its entity and World.
- `PIXIE-TASK-030` — **Status:** done — Time and randomness: the World counts its own steps and simulated time, and owns a random number generator created from a seed.
- `PIXIE-TASK-031` — **Status:** done — Built-in components `Transform2D`, `Velocity2D`, and `Shape`; the previous transform kept each step; integration in `PostUpdate`; a render system that draws interpolated shapes.
- `PIXIE-TASK-032` — **Status:** done — CTest tests for phase order, Behaviour lifecycle, deferred destruction, and identical sequences from one seed, each seen failing on a broken copy.

## Verification

Machine: Windows 11, Visual Studio 2026 (MSVC 14.51), CMake 4.3.1; commit
`6c9bd8b`.

- Debug and Release build with `/W4 /WX`: zero warnings in `pixie_runtime`,
  `runtime-tests`, and the rest of the project's own targets.
- `ctest` in Debug and Release: `engine-tests` and `runtime-tests` pass; the
  runtime has 14 tests.
- Each behaviour named in the completion criteria was broken in a copy and its
  test failed, then the copy was restored:
  - PreUpdate systems moved after PostUpdate: the phase-order test and the
    integration test fail;
  - `Start` called before every update: the lifecycle test fails;
  - `Stop` called without `Start`: the never-started test fails;
  - destruction made immediate during a step: the deferred-destruction test
    fails;
  - the generator salted per instance: the same-seed, reseed, and pinned-value
    tests fail.
- The first value of `Random(0)` is pinned at `0x99EC5F36CB75F2B4`, the known
  first output of xoshiro256** seeded through splitmix64 from 0.
- The module's `MANIFEST.md`, `README.md`, and `REFERENCE.md` were checked
  against the public headers under `runtime/include/pixie/runtime/`;
  `scripts/check_docs.py` reports no problem.
- Not run here: `SubmitShapes` draws nothing in the tests, which have no GL
  context; the boids sample of PIXIE-TODO-007 is its first use. Linux was not
  built.
