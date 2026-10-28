# Platform Module Manifest

**Document ID:** PLATFORM-MANIFEST

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** platform identity, source/build ownership and documentation routes

**Surface:** manifest

**Boundary:** platform

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

**Projects:** engine/platform/CMakeLists.txt

**Language:** cpp

**Artifact:** shared-library

**Platforms:** windows, linux

**Module dependencies:** core

**Third-party dependencies:** PixieLib, GLFW

**Distribution:** shipped-runtime

**Stability:** experimental

**Public API:** engine/platform/include/pixie/platform

**Profiles:** none

**Technical reference:** docs/modules/platform/REFERENCE.md

**Related boundaries:** core, app, capi

**Source:** engine/platform/src, engine/platform/include

**Tests:** tests/engine

**Runtime scenarios:** none

## Routes

- [Introduction](README.md) and [technical reference](REFERENCE.md).
- CMake owns the internal pixie_platform object target and Pixie::Platform consumer facade. The artifact field describes distribution inside the shared engine, not a separate DLL per module.
- [Qualification matrix](../../shared-engine-export-validation.md) separates executed evidence from pending native configurations, exports and MCP work. Windows is the qualified execution environment so far; Linux remains unqualified.

