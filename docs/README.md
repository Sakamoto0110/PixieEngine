# Documentation Index

**Document ID:** GLOBAL-DOCUMENTATION-INDEX

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** entry point to PixieEngine documentation

**Surface:** index

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Start with the [documentation policy](governance/documentation-policy.md). It
owns the authority table: which document owns which kind of fact.

| Location | Contents |
|---|---|
| [`../ROADMAP.md`](../ROADMAP.md) | What the engine is and is not, direction, guardrails, freezes |
| [`../TODO.md`](../TODO.md) | Formally promoted work |
| [`../TASKLIST.md`](../TASKLIST.md) | Task breakdowns of promoted work |
| [`../DEPENDENCIES.md`](../DEPENDENCIES.md) | Dependencies, versions, status, and candidates |
| [`development.md`](development.md) | How to build, check, and commit a change |
| [`shared-engine-export-validation.md`](shared-engine-export-validation.md) | Qualification plan for PIXIE-TODO-051: shared engine, export barriers and separately isolated local MCP development |
| [`game-export.md`](game-export.md) | Native manifest export command, shipping/build separation and current qualification limits |
| [`player-debug.md`](player-debug.md) | Native-player F3 rates and F3+C runtime VSync/frame cap commands |
| [`telemetry.md`](telemetry.md) | Per-World CPU metrics, scoped timings, counters, CSV and native display |
| [`physics-telemetry.md`](physics-telemetry.md) | Physics stage boundaries, retained sync buffers and bounded headless qualification |
| [`character-body-coupling.md`](character-body-coupling.md) | D4 motor preparation, bounded body impulses, support carry and unsafe-placement policy |
| [`collision-movement.md`](collision-movement.md) | Static collision queries, capsule movement, controller ownership and neutral room |
| [`editor-movement.md`](editor-movement.md) | Scene components, editor authoring/Play-Stop, diagnostics and in-game laboratory reuse |
| [`rigid-body-physics-requirements.md`](rigid-body-physics-requirements.md) | Proposed dynamics architecture, character coupling, authoring, migration and staged acceptance before lighting |
| [`body-dynamics.md`](body-dynamics.md) | Initial native/scene body dynamics, world gravity, units, lifecycle and remaining delivery gates |
| [`body-events.md`](body-events.md) | Contact/sensor lifetimes, queries, scene identities and bounded stack/material/fast-body qualification |
| [`particle-dynamics.md`](particle-dynamics.md) | Native dynamic particles, world gravity, bounded circle-sweep bounce and the playable sparks room |
| [`particle-memory-layout.md`](particle-memory-layout.md) | The particle's 40-byte layout, how pools are stepped and drawn, and the open directions for dynamic particles and SIMD; a handoff |
| [`circuit-puzzle-requirements.md`](circuit-puzzle-requirements.md) | Existing consumer requirements analysis: atmosphere, lighting, movement and circuit panels; design input |
| [`governance/`](governance/documentation-policy.md) | Documentation and commit rules |
| [`schemas/`](schemas/README.md) | Documentation structure and vocabulary, and its derivation |
| [`templates/`](templates/) | Templates for modules, schedulers, proposals, and archive records |
| [`proposals/`](proposals/README.md) | Unpromoted ideas |
| [`archive/`](archive/README.md) | Chronology, completed work, dated rationale |
| [`../editor/`](../editor/README.md) | The C# ImGui editor |
| `../scripts/check_docs.py` | The documentation verifier; see the [development guide](development.md#checks-before-a-commit) |
| [`modules/runtime/`](modules/runtime/README.md) | The `runtime` module: World, entities, Behaviours, update phases, particles, reflection, and projects |
| [`modules/runtime/FINDINGS.md`](modules/runtime/FINDINGS.md) | Unconfirmed runtime observations and deferred investigations |
| [`modules/core/`](modules/core/README.md) | CPU timing, logging, telemetry and images |
| [`modules/platform/`](modules/platform/README.md) | Window/context ownership, input and frame limiting |
| [`modules/render/`](modules/render/README.md) | GL resources, dimension-neutral View/readback and 2D renderers |
| [`modules/app/`](modules/app/README.md) | Native player loop and debug/input composition |
| [`modules/capi/`](modules/capi/README.md) | The C interface in the shared engine and development game modules |
| `modules/` | One directory per decided engine boundary |
