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

**Module dependencies:** runtime

**Third-party dependencies:** none

**Distribution:** editor-only

**Stability:** experimental

**Public API:** capi/include/pixie_capi.h

**Profiles:** none

**Technical reference:** docs/modules/capi/REFERENCE.md

**Related boundaries:** runtime

**Source:** capi/src

**Tests:** tests/capi

**Runtime scenarios:** none

## Routes

- Introduction: [`README.md`](README.md).
- Contract: [`REFERENCE.md`](REFERENCE.md); the header `capi/include/pixie_capi.h` owns the
  exact declarations.
- Build: the object library `pixie_capi_objects` and the CMake function `pixie_add_capi_library`
  in `capi/CMakeLists.txt`. Each game calls it once: `boids-capi`, `flocks-capi`, and
  `particles-capi` under `samples/`.
- Tests: `capi-test` (`tests/capi/CapiTest.c`), registered in CTest once per game library.
- Distribution: the libraries serve hosts such as the editor; the players do not use them. They
  are still general (tools and tests use them too), as the guardrail on the editor asks.
- Origin: [PIXIE-PROPOSAL-002](../../proposals/editor-native-bridge.md) and the `capi` boundary of
  [PIXIE-PROPOSAL-001](../../proposals/engine-module-boundaries.md); built by PIXIE-TODO-010 (see
  the [archive](../../archive/README.md)).
- Platforms: built and tested on Windows only so far; the header and the test also cover Linux
  (`visibility`, `dlopen`), untested.
