# PixieEngine Task List

**Document ID:** GLOBAL-TASKLIST

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** task breakdowns of promoted TODO.md entries

**Surface:** task-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

## Purpose

This file owns Engine task states and routes inherited workspace parents through
[`TODO.md`](TODO.md). Sections may contribute to a workspace TODO rather than a
new local parent. Compatibility sections contain links, not duplicate task states.

- Current task sections name their canonical parent in `Parent`; the workspace
  breakdown links to each project's current task owner.
- Tasks decompose the parent's accepted scope and never extend it.
- When the parent completes, remove its section through the
  [closure flow](docs/history/README.md). Retain only important details or links
  in the closure; Git preserves the full breakdown.

The full rules are in the
[documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd),
and the section shape is in the
[template](docs/templates/TASKLIST-SECTION.md).

## Open breakdowns

Engine task states and compatibility routes live here. Shared block constraints
are owned by the linked workspace. The 055/061 sections retain completed-task provenance. Historical
execution records are discoverable through the [evidence index](docs/evidence/README.md).

## `PIXIE-TODO-055` — Entity input and C++ component authoring

**Parent:** [PIXIE-TODO-055](TODO.md#pixie-todo-055--entity-input-controls-and-c-component-authoring)

The implementation tasks 219–224 are complete; their
[recorded delivery](docs/evidence/authoring-2026-11-06.md#entity-input-and-component-authoring)
retains the original limits. The only pending disposition is the parent's
independent review. No new implementation or test task is introduced here.

## `PIXIE-TODO-056` — Editor and C++ authoring consolidation

**Parent:** [PIXIE-TODO-056](../../.governance/workplaces/void-workplace/TODO.md#pixie-todo-056--editor-and-c-authoring-consolidation).

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-todo-056--editor-and-c-authoring-consolidation). Current shared context is required before execution.

### Block A — C++ code, input events and Behaviour authoring rework (P0.1; proposal 4.1/4.2)

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#block-a--c-code-input-events-and-behaviour-authoring-rework-p01-proposal-4142). Current shared context is required before execution.

#### A.1 — C++ code creation, navigation and contextual templates

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#a1--c-code-creation-navigation-and-contextual-templates). Current shared context is required before execution.

<a id="pixie-task-225"></a>
- [PIXIE-TASK-225](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-225) — transferred; this location has no mutable status.

#### A.2 — Input events and callbacks without controller prerequisites

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#a2--input-events-and-callbacks-without-controller-prerequisites). Current shared context is required before execution.

<a id="pixie-task-226"></a>
- `PIXIE-TASK-226` — **Status:** open — Remove KeyboardControl/MouseControl as event-unlocking prerequisites; define enabled/filtering/unused-handler delivery and explicit compatibility for saved scenes without hardcoded gameplay actions or unbounded event fan-out.

#### A.3 — End-to-end authoring verification and first example

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#a3--end-to-end-authoring-verification-and-first-example). Current shared context is required before execution.

<a id="pixie-task-227"></a>
- [PIXIE-TASK-227](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-227) — transferred; this location has no mutable status.

### Block B — Predictable viewport gestures (P0.2; proposal 5)

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#block-b--predictable-viewport-gestures-p02-proposal-5). Current shared context is required before execution.

<a id="pixie-task-228"></a>
- [PIXIE-TASK-228](../PixieEditor/TASKLIST.md#pixie-task-228) — transferred; this location has no mutable status.

### Block C — World/Scene integration handoff (P0.3; proposal 7)

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#block-c--worldscene-integration-handoff-p03-proposal-7). Current shared context is required before execution.

### Block D — Field inventory and meaningful Inspector (P1.1; proposal 6.1)

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#block-d--field-inventory-and-meaningful-inspector-p11-proposal-61). Current shared context is required before execution.

<a id="pixie-task-230"></a>
- [PIXIE-TASK-230](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-230) — transferred; this location has no mutable status.

### Block E — Presets, Session and viewport controls (P1.2; proposal 6.2)

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#block-e--presets-session-and-viewport-controls-p12-proposal-62). Current shared context is required before execution.

<a id="pixie-task-231"></a>
- [PIXIE-TASK-231](../PixieEditor/TASKLIST.md#pixie-task-231) — transferred; this location has no mutable status.

### Block F — Later diagnostics and resource workflows (P2; proposal 8)

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#block-f--later-diagnostics-and-resource-workflows-p2-proposal-8). Current shared context is required before execution.

<a id="pixie-task-232"></a>
- [PIXIE-TASK-232](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-232) — transferred; this location has no mutable status.

### Block G — Integrated acceptance and documentation (proposal 9)

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#block-g--integrated-acceptance-and-documentation-proposal-9). Current shared context is required before execution.

<a id="pixie-task-233"></a>
- [PIXIE-TASK-233](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-233) — transferred; this location has no mutable status.

## `PIXIE-TODO-060` — Engine SDK, game runtime and Editor separation

**Parent:** [PIXIE-TODO-060](../../.governance/workplaces/void-workplace/TODO.md#pixie-todo-060--separate-engine-sdk-game-runtime-and-editor-delivery).

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-todo-060--engine-sdk-game-runtime-and-editor-separation). Current shared context is required before execution.

### B0 — Contract readiness and coordinated extraction

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#b0--contract-readiness-and-coordinated-extraction). Current shared context is required before execution.

<a id="pixie-task-242"></a>
- [PIXIE-TASK-242](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-242) — transferred; this location has no mutable status.

### K01 — Native SDK and consumer support, one agent

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#k01--native-sdk-and-consumer-support-one-agent). Current shared context is required before execution.

<a id="pixie-task-243"></a>
- `PIXIE-TASK-243` — **Status:** in-progress — B4: define and implement selected compatibility identity and low-level interop delivery, keeping Editor session/history policy in B5. Preserve ABI28/contract1 candidate checks, token/receipt layouts, module leases and teardown. G2 rejects incompatible ABI/config/profile and unsupported authored types without treating C ABI equality as C++ compatibility or promising rollback of external effects.

<a id="pixie-task-244"></a>
- `PIXIE-TASK-244` — **Status:** in-progress — B1: produce the selected relocatable SDK/profile manifest, public targets/headers, player/module inputs, compiled math, dependency closure, CPU/toolchain/CRT identity, notices and provenance. Pass G1 with B9 through an external consumer and unavailable producer checkout, including World/Scene/presentation headers and missing-dependency negatives; package mode neither rebuilds Engine nor silently falls back to its sources.

<a id="pixie-task-245"></a>
- `PIXIE-TASK-245` — **Status:** in-progress — B2: consume the qualified SDK and frozen B4 contract for game/player support, CMake helpers, templates, registry markers/schema and bootstrap. G2 covers new, recognized legacy, registered and unfamiliar custom projects, code/no-code inputs and mismatch/refusal cases; preserve edited user code/CMake, hashes/preimages, explicit contract1 admission and v1 scene-conversion safeguards.

### K02 — Export and Editor authoring, one agent

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#k02--export-and-editor-authoring-one-agent). Current shared context is required before execution.

<a id="pixie-task-246"></a>
- `PIXIE-TASK-246` — **Status:** in-progress — B3: deliver the native SDK-aware exporter, accepted CLI/product policy, selected-runtime/descriptor/manifest identity and nonadjacent dependency resolution. Pass G3 with code/no-code runtime-only launch, asset/native/CRT closure, complete v2 scopes/presentation/seed/opaque data and unchanged v1 policy. Detect missing DLLs, mixed identities and development payload contamination without producer-machine paths.

<a id="pixie-task-247"></a>
- [PIXIE-TASK-247](../PixieEditor/TASKLIST.md#pixie-task-247) — transferred; this location has no mutable status.

### B6/B7/B8 — Standalone adapters and conditional dependencies

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#b6b7b8--standalone-adapters-and-conditional-dependencies). Current shared context is required before execution.

<a id="pixie-task-248"></a>
- [PIXIE-TASK-248](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-248) — transferred; this location has no mutable status.

<a id="pixie-task-249"></a>
- [PIXIE-TASK-249](../PixieInspector/TASKLIST.md#pixie-task-249) — transferred; this location has no mutable status.

### K03 — External evidence and release preparation, one agent

[Inherited workspace block](../../.governance/workplaces/void-workplace/TASKLIST.md#k03--external-evidence-and-release-preparation-one-agent). Current shared context is required before execution.

<a id="pixie-task-251"></a>
- [PIXIE-TASK-251](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-251) — transferred; this location has no mutable status.

<a id="pixie-task-256"></a>
- [PIXIE-TASK-256](../../.governance/workplaces/void-workplace/TASKLIST.md#pixie-task-256) — transferred; this location has no mutable status.

#### D09 — Accepted GLFW distribution policy

The Windows x64 Editor uses the SDK's GLFW for the selected profile and
configuration. The public Editor candidate carries authoring/Release GLFW.
See the [D09 evidence](docs/evidence/d09-shared-glfw-2026-11-12.md) for
the package comparison, the observed loaded providers and the OpenGL 4.5
lifecycle. The combined G6 disposition stays with TASK-256.

## `PIXIE-TODO-061` — Native entity parenting

**Parent:** [PIXIE-TODO-061](TODO.md#pixie-todo-061--native-entity-parenting).

- `PIXIE-TASK-257` — **Status:** done — In-memory 2D relations, local/world poses and render interpolation passed the simple three-level World fixture in Release. [Runtime evidence](docs/evidence/native-parenting-runtime-2026-11-07.md) records rejection and lifetime checks, artifact hashes and the limits of this result.
- `PIXIE-TASK-258` — **Status:** done — Version 3 scene and fragment persistence passed the local and installed SDK fixture. The existing Editor saved and reopened the scene and its paired no-code player rendered the exported hierarchy. [Persistence evidence](docs/evidence/native-parenting-persistence-2026-11-07.md) records the scope and artifact identities.
- `PIXIE-TASK-259` — **Status:** done — The roots-only Entities list and editable Inspector tree passed the simple-scene history, unsaved code rebuild and paired shipping export checks. [Editor evidence](docs/evidence/native-parenting-editor-2026-11-07.md) records the UI and player results.
- `PIXIE-TASK-260` — **Status:** done — The separate Weapons-Lab r5 copy passed branch duplication, code rebuild, save, Play/Stop and paired shipping-player checks. [Consumer acceptance](docs/evidence/native-parenting-weapons-lab-2026-11-07.md) records the observed scope and comparison baseline.
