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

Choose a route by the question you need to answer. The
[documentation policy](governance/documentation-policy.md#authority) names the
owner of each kind of fact; evidence records describe specific executed baselines.

For workspace work, use the [filesystem map](../FILESYSTEM.md) and
[workspace entry](../../../.governance/workplaces/void-workplace/README.md).
The [flattened catalog](../../../.governance/workplaces/void-workplace/docs/catalog/README.md)
routes across scopes without transferring technical ownership or active work.

## Build and use

| Guide | Contents |
|---|---|
| [Consumer handbook](handbook/README.md) | First game with an installed SDK and independent Editor: setup, public API, examples, UI handoff and export |
| [Development](development.md) | Build the native engine, consume an installed SDK, prepare releases and work in the shared solution |
| [Game export](game-export.md) | Manifest export, shipping/build separation and dependency closure |
| [Independent Editor](../editor/README.md) | Migration, checkout and build entry point; Editor-owned guides live with the Editor |
| [Dependencies](../DEPENDENCIES.md) | Toolchains, packages, versions, ownership and candidates |
| [Player controls](player-debug.md) | F3 metrics and F3+C VSync/frame-cap commands |
| [Telemetry](telemetry.md) | Per-World CPU metrics, scoped timings, counters, CSV and native display |
| [Deterministic capture](deterministic-capture.md) | Fixed-step capture, seed precedence and snapshot approval boundaries |
| [Entity input and C++ components](entity-input-and-code-components.md) | Generic input, component creation, source association and preserving rebuilds |
| [Scene movement authoring](editor-movement.md) | Components, Play/Stop, diagnostics and laboratory reuse |
| [Inspector field interactions](editor-field-interactions.md) | Route to the independent Editor's chain/lock/history guide |

## Native modules and API

Each module's manifest routes its public headers, reference contract and evidence.
Headers own the public API; the reference explains its contract.

| Module | Responsibility |
|---|---|
| [Core](modules/core/README.md) | CPU timing, logging, telemetry and images |
| [Platform](modules/platform/README.md) | Window/context ownership, input and frame limiting |
| [Render](modules/render/README.md) | GL resources, dimension-neutral View/readback and 2D renderers |
| [App](modules/app/README.md) | Native player loop and debug/input composition |
| [Runtime](modules/runtime/README.md) | World, entities, Behaviours, update phases, particles, reflection and projects |
| [C API](modules/capi/README.md) | Shared-engine C interface and development game modules |

## Features and samples

| Subject | Guide | Sample |
|---|---|---|
| Collision and movement | [Ownership, units and limits](collision-movement.md) | [Native movement room](../samples/movement/README.md), [no-code movement lab](../samples/movement-lab/README.md) |
| Body dynamics | [Gravity, lifecycle and limits](body-dynamics.md) | [Physics lab](../samples/physics-lab/README.md), [compound lab](../samples/compound-lab/README.md) |
| Body events | [Contact/sensor lifetimes and scene identities](body-events.md) | [Physics lab](../samples/physics-lab/README.md) |
| Character/body coupling | [Impulses, support carry and placement](character-body-coupling.md) | [Coupling lab](../samples/coupling-lab/README.md) |
| Dynamic particles | [Gravity and bounded circle-sweep bounce](particle-dynamics.md) | [Sample tree](../samples/) |
| Particle storage | [Layout, measurements and open design](particle-memory-layout.md) | [Sample tree](../samples/) |
| Physics telemetry | [Stages, buffers and qualification limits](physics-telemetry.md) | [Telemetry sample](../samples/telemetry/README.md) |

## Direction and design

| Document | Role |
|---|---|
| [Roadmap](../ROADMAP.md) | Overall picture, phase rationale, outcomes, acceptance criteria and freezes |
| [TODO](../TODO.md) | Accepted work, dependencies and gates |
| [TASKLIST](../TASKLIST.md) | Decomposition of large TODO entries |
| [Proposals](proposals/README.md) | Design origins and promotion records; the active planners own execution |
| [Editor authoring consolidation](proposals/editor-authoring-consolidation.md) | Origin of the C++/UX consolidation scope and World/Scene coordination |
| [Rigid-body requirements](rigid-body-physics-requirements.md) | Dynamics, coupling, migration and staged acceptance design |
| [Lighting requirements](lighting-requirements.md) | Lights, shadows, emissive surfaces, composition and acceptance design |
| [Lighting L0 design](lighting-l0-design.md) | Recorded data-model, composition and shadow-technique decisions |
| [Circuit consumer requirements](circuit-puzzle-requirements.md) | Atmosphere, lighting, movement and circuit panels as design input |
| [Runtime findings](modules/runtime/FINDINGS.md) | Unconfirmed observations and deferred investigations |

## Evidence and history

| Record | Contents |
|---|---|
| [Evidence index](evidence/README.md) | 021/022 decisions and execution, authoring deliveries and links to existing qualification |
| [Shared-engine/export qualification](shared-engine-export-validation.md) | TODO-051 results, local MCP, lighting snapshot and exact qualification limits |
| [Physics qualification](physics-qualification.md) | PHY-001–021 coverage, results and acceptance limits |
| [Lighting L5 consolidation](lighting-l5-consolidation.md) | Scenario matrix, stress report and recorded reference/visual limits |
| [History](history/README.md) | Completed roadmap outcomes with evidence links and planning baselines |
| [Legacy archive](archive/README.md) | Preserved earlier chronology, completed work and dated rationale |

## Documentation maintenance

| Document | Contents |
|---|---|
| [Policy](governance/documentation-policy.md) | Fact ownership, planning, evidence, closure and commit rules |
| [Schemas](schemas/README.md) | Metadata vocabulary and historical derivation |
| [Templates](templates/) | Module, planning, proposal and closure shapes |
| [Documentation checker](../checkdocs.ps1) | Standalone PowerShell structural check; [usage](development.md#checks-before-a-commit) |
