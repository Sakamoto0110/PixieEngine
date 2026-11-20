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

**Artifact:** shared-library

**Platforms:** windows, linux

**Module dependencies:** core, platform, render, app

**Third-party dependencies:** EnTT, PixieLib, nlohmann/json (private: scenes only), Box2D (private: collision backend)

**Distribution:** shipped-runtime

**Stability:** experimental

**Public API:** runtime/include/pixie/runtime

**Profiles:** none

**Technical reference:** docs/modules/runtime/REFERENCE.md

**Related boundaries:** core, platform, render, app, capi

**Source:** runtime/src

**Tests:** tests/runtime

**Runtime scenarios:** none

## Routes

- Introduction: [`README.md`](README.md).
- Contract: [`REFERENCE.md`](REFERENCE.md).
- Unconfirmed observations: [`FINDINGS.md`](FINDINGS.md).
- Collision and movement usage: [`collision-movement.md`](../../collision-movement.md).
- Authored scene movement and laboratory boundaries: [`editor-movement.md`](../../editor-movement.md).
- Body dynamics and world settings: [`body-dynamics.md`](../../body-dynamics.md).
- Contact/sensor events and qualification: [`body-events.md`](../../body-events.md).
- Build through the internal `pixie_runtime` object target in
  [`runtime/CMakeLists.txt`](../../../runtime/CMakeLists.txt). It uses the lower
  App, Render, Platform and Core boundaries. Link game code through `Pixie::Runtime`,
  which links the root shared engine containing Runtime.
- SDK declarations are installed from `runtime/include/pixie/runtime` under
  `include/pixie/runtime`. Runtime’s C++ surface serves directly linked game code
  and admitted development modules. The Runtime distribution payload and SDK
  profile rules belong to the [installed SDK guide](../../development.md#installed-sdk-and-local-rehearsal).
- Implementation lives in `runtime/src`. `Parenting2DInternal.hpp`,
  `PresentationJson.hpp` and `SceneSave.hpp` are private implementation headers.
  Box2D and the JSON codec stay behind Runtime’s API.
- Tests are registered as `runtime-tests` in [`tests/CMakeLists.txt`](../../../tests/CMakeLists.txt)
  with sources in `tests/runtime/`. The [validation section](REFERENCE.md#validation) also routes
  the C API scene checks and renderer coverage.
- Origin: [PIXIE-PROPOSAL-004](../../proposals/scene-model-and-anchors.md), part 1, and the
  `runtime` boundary of [PIXIE-PROPOSAL-001](../../proposals/engine-module-boundaries.md);
  built by PIXIE-TODO-006 (see the [archive](../../archive/README.md)).
- Platforms: built and tested on Windows only so far; only `UserDataFolder` is Windows-specific,
  with its other platforms' roots written and not yet run.
