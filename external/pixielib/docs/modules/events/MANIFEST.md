# Events Module Manifest

**Document ID:** EVENTS-MANIFEST

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** identity and routing of the events module

**Surface:** manifest

**Boundary:** events

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

```text
Projects: cpp/CMakeLists.txt
Language: cpp
Artifact: header-only
Platforms: windows, linux
Target frameworks: none
Module dependencies: none
Third-party dependencies: none
Distribution: source
Stability: preview
Public API: cpp/events/include/pixie/events
Profiles: none
Technical reference: docs/modules/events/REFERENCE.md
Related boundaries: none
Source: cpp/events/include/pixie/events
Tests: cpp/tests/pxCallback.test.cpp, cpp/tests/pxEventHandler.test.cpp, cpp/tests/pxSubscription.test.cpp, cpp/tests/pxEvent.test.cpp
Runtime scenarios: none
```

## Routes

- [Introduction](README.md) and [technical reference](REFERENCE.md).
- The C++ side only: the `pixie::events` CMake target, headers only (1.10).
  C# has `event` of its own.
- The decisions behind the module, and the 23 deliberately broken copies its
  tests catch, are in
  [PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md), section 4.6;
  the 2023 code it was ported from, and its three bugs, are in
  [PXL-ARCHIVE-005](../../archive/diagnosis-2023-code-2026-10-02.md).
- No on-demand surface exists yet.
