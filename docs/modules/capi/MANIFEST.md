# C API Module Manifest

**Document ID:** CAPI-MANIFEST

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** identity, build project, dependencies, and documentation routes of the capi module

**Surface:** manifest

**Boundary:** capi

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

**Projects:** capi/CMakeLists.txt

**Language:** cpp

**Artifact:** shared-library

**Platforms:** windows, linux

**Module dependencies:** runtime, platform, render

**Third-party dependencies:** none

**Distribution:** shipped-runtime

**Stability:** experimental

**Public API:** capi/include/pixie_capi.h

**Profiles:** none

**Technical reference:** docs/modules/capi/REFERENCE.md

**Related boundaries:** runtime, platform, render

**Source:** capi/src

**Tests:** tests/capi

**Runtime scenarios:** none

## Routes

- Introduction: [`README.md`](README.md).
- Contract: [`REFERENCE.md`](REFERENCE.md); the header `capi/include/pixie_capi.h` owns the
  exact declarations.
- Build: `pixie_capi_objects` in `capi/CMakeLists.txt`, embedded in the root
  `pixie_shared` target (output `pixie.dll`). `pixie_game` in
  `cmake/PixieGame.cmake` generates development-only game modules beside direct
  game players; the C interface is not copied into each module.
- Tests: `capi-test` (`tests/capi/CapiTest.c`), registered in CTest once per game library, and
  `project-code` (`tests/project-code/ProjectCodeTest.cmake`), a project with code built against
  the engine.
- Distribution: this interface is part of the shared engine used by players,
  tools and hosts. Module loading and the planned development channel are
  excluded from shipping builds; export/dependency-closure qualification remains
  tracked separately in PIXIE-TODO-051.
- Origin: [PIXIE-PROPOSAL-002](../../proposals/editor-native-bridge.md) and the `capi` boundary of
  [PIXIE-PROPOSAL-001](../../proposals/engine-module-boundaries.md); built by PIXIE-TODO-010 (see
  the [archive](../../archive/README.md)).
- Platforms: built and tested on Windows only so far; the header and the test also cover Linux
  (`visibility`, `dlopen`), untested.
