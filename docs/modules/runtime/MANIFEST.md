# Runtime Module Manifest

**Document ID:** RUNTIME-MANIFEST

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** identity, build project, dependencies, and documentation routes of the runtime module

**Surface:** manifest

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

**Projects:** runtime/CMakeLists.txt

**Language:** cpp

**Artifact:** static-library

**Platforms:** windows, linux

**Module dependencies:** none

**Third-party dependencies:** EnTT, PixieLib

**Distribution:** shipped-runtime

**Stability:** experimental

**Public API:** runtime/include/pixie/runtime

**Profiles:** none

**Technical reference:** docs/modules/runtime/REFERENCE.md

**Related boundaries:** capi

**Source:** runtime/src

**Tests:** tests/runtime

**Runtime scenarios:** none

## Routes

- Introduction: [`README.md`](README.md).
- Contract: [`REFERENCE.md`](REFERENCE.md).
- Build target: `pixie_runtime` (alias `Pixie::Runtime`) in `runtime/CMakeLists.txt`. It links
  the provisional engine library (`Pixie::Engine`: `FrameTime`, `Input`, `ShapeRenderer`), which
  belongs to no decided boundary yet, so it is not listed under module dependencies.
- Tests: `runtime-tests`, from `tests/runtime/`, run through CTest.
- Origin: [PIXIE-PROPOSAL-004](../../proposals/scene-model-and-anchors.md), part 1, and the
  `runtime` boundary of [PIXIE-PROPOSAL-001](../../proposals/engine-module-boundaries.md);
  built by PIXIE-TODO-006 (see the [archive](../../archive/README.md)).
- Platforms: built and tested on Windows only so far; nothing in it is Windows-specific.
