# Core Module Manifest

**Document ID:** CORE-MANIFEST

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** core identity, source/build ownership and documentation routes

**Surface:** manifest

**Boundary:** core

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

**Projects:** engine/core/CMakeLists.txt

**Language:** cpp

**Artifact:** shared-library

**Platforms:** windows, linux

**Module dependencies:** none

**Third-party dependencies:** PixieLib, stb_image (private CPU decoding headers)

**Distribution:** shipped-runtime

**Stability:** experimental

**Public API:** engine/core/include/pixie/core

**Profiles:** none

**Technical reference:** docs/modules/core/REFERENCE.md

**Related boundaries:** platform, render, app, runtime

**Source:** engine/core/src, engine/core/include

**Tests:** tests/engine

**Runtime scenarios:** none

## Routes

- [Introduction](README.md) and [technical reference](REFERENCE.md).
- CMake owns the internal pixie_core object target and Pixie::Core consumer facade. The artifact field describes distribution inside the shared engine, not a separate DLL per module.
- [Qualification matrix](../../shared-engine-export-validation.md) separates executed evidence from pending native configurations, exports and MCP work. Windows is the qualified execution environment so far; Linux remains unqualified.

