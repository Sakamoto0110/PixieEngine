# Render Module Manifest

**Document ID:** RENDER-MANIFEST

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** render identity, source/build ownership and documentation routes

**Surface:** manifest

**Boundary:** render

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

**Projects:** engine/render/CMakeLists.txt

**Language:** cpp

**Artifact:** shared-library

**Platforms:** windows, linux

**Module dependencies:** core

**Third-party dependencies:** PixieLib, GLEW, OpenGL, stb_truetype

**Distribution:** shipped-runtime

**Stability:** experimental

**Public API:** engine/render/include/pixie/render

**Profiles:** none

**Technical reference:** docs/modules/render/REFERENCE.md

**Related boundaries:** core, app, runtime, capi

**Source:** engine/render/src, engine/render/include

**Tests:** tests/engine

**Runtime scenarios:** none

## Routes

- [Introduction](README.md) and [technical reference](REFERENCE.md).
- CMake owns the internal pixie_render object target and Pixie::Render consumer facade. The artifact field describes distribution inside the shared engine, not a separate DLL per module.
- [Qualification matrix](../../shared-engine-export-validation.md) separates executed evidence from pending native configurations, exports and MCP work. Windows is the qualified execution environment so far; Linux remains unqualified.

