# App Module Manifest

**Document ID:** APP-MANIFEST

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** app identity, source/build ownership and documentation routes

**Surface:** manifest

**Boundary:** app

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

**Projects:** engine/app/CMakeLists.txt

**Language:** cpp

**Artifact:** shared-library

**Platforms:** windows, linux

**Module dependencies:** core, platform, render

**Third-party dependencies:** PixieLib, GLFW (input composition), stb_easy_font (private headers)

**Distribution:** shipped-runtime

**Stability:** experimental

**Public API:** engine/app/include/pixie/app

**Profiles:** none

**Technical reference:** docs/modules/app/REFERENCE.md

**Related boundaries:** core, platform, render, runtime

**Source:** engine/app/src, engine/app/include

**Tests:** tests/engine

**Runtime scenarios:** none

## Routes

- [Introduction](README.md) and [player loop, teardown and configuration](REFERENCE.md).
- CMake owns the internal pixie_app object target and Pixie::App consumer facade. The artifact field describes distribution inside the shared engine, not a separate DLL per module.
- SDK extension points live under `pixie/app/`. `DebugConsole` and `DebugOverlay` are internal App implementation types.
- Contributor routes include `samples/hello-gl`, `tests/engine/PlayerDebugTests.cpp`, `tests/engine/DebugConsoleTests.cpp`, `tests/engine/DebugOverlayTests.cpp` and `tests/engine/DeterministicCaptureTests.cpp`.
- [Qualification matrix](../../shared-engine-export-validation.md) separates executed evidence from pending native configurations, exports and MCP work. Windows is the qualified execution environment so far; Linux remains unqualified.
