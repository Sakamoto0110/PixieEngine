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

This file is a child scheduler of [`TODO.md`](TODO.md). Each section breaks
one promoted `TODO.md` entry into tasks, so that large entries, or entries made
of many small tasks, do not pollute `TODO.md`.

- Every section names its parent in `Parent`, and the parent links back here
  in `Breakdown`.
- Tasks decompose the parent's accepted scope and never extend it.
- When the parent completes, remove its section through the
  [closure flow](docs/history/README.md). Retain only important details or links
  in the closure; Git preserves the full breakdown.

The full rules are in the
[documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd),
and the section shape is in the
[template](docs/templates/TASKLIST-SECTION.md).

## Open breakdowns

Pending decomposition and its necessary constraints live here. Historical
execution records are discoverable through the [evidence index](docs/evidence/README.md).

## `PIXIE-TODO-055` — Entity input and C++ component authoring

**Parent:** [PIXIE-TODO-055](TODO.md#pixie-todo-055--entity-input-controls-and-c-component-authoring)

The implementation tasks 219–224 are complete; their
[recorded delivery](docs/evidence/authoring-2026-11-06.md#entity-input-and-component-authoring)
retains the original limits. The only pending disposition is the parent's
independent review. No new implementation or test task is introduced here.

## `PIXIE-TODO-056` — Editor and C++ authoring consolidation

**Parent:** [PIXIE-TODO-056](TODO.md#pixie-todo-056--editor-and-c-authoring-consolidation)

**Promotion:** owner decision, 2026-11-02. Proposal 017 preserves the originating
reasoning; this section owns execution ordering, task state and block boundaries.
The task states below retain the recorded baseline. Documentation cleanup
does not authorize execution or change acceptance.

### Block A — C++ code, input events and Behaviour authoring rework (P0.1; proposal 4.1/4.2)

**Scope:** reusable native Behaviour types, event delivery, contextual templates,
source navigation and separate create/attach/build operations in the existing project.
**Boundaries:** no alternate scripting language, per-entity source copies, duplicate
registration/build pipeline, compulsory input-controller attachment for callbacks,
or incidental physics/World rewrite. Preserve edited C++/CMake, legacy scene behavior,
fixed-step order, filtering/opt-out, unsaved handoff, exact targets and IntelliSense.
Keep runtime code in Engine, editor composition in its C# host; shared generic
Inspector changes require a bounded contract, not gameplay logic in the library.
**Entry/exit:** start from 055's actual delivered candidate. Close with one normal
editor C++ loop and owner UX review. Creating code must not force immediate build;
new native types remain visibly pending until compiled. Hot reload without restart
is not implicitly required. Keep instance enable/disable useful independently of wiring.

#### A.1 — C++ code creation, navigation and contextual templates

- `PIXIE-TASK-225` — **Status:** open — Rework create/attach/edit/build presentation, coherent Behaviour grouping and source navigation; generate minimal selected callbacks and native public headers for the selection's common components, without modifying an existing Behaviour on attachment.

#### A.2 — Input events and callbacks without controller prerequisites

- `PIXIE-TASK-226` — **Status:** open — Remove KeyboardControl/MouseControl as event-unlocking prerequisites; define enabled/filtering/unused-handler delivery and explicit compatibility for saved scenes without hardcoded gameplay actions or unbounded event fan-out.

#### A.3 — End-to-end authoring verification and first example

- `PIXIE-TASK-227` — **Status:** open — Qualify contextual Get/TryGet, code preservation, pending build/attachment and normal Debug/Release use; provide the first working example and independently review the slice before owner UX acceptance.

### Block B — Predictable viewport gestures (P0.2; proposal 5)

**Scope:** middle-click/pan, focus/capture, coordinate conversion, resize/DPI.
**Boundaries:** first measure camera AND entity transforms. Do not assume World
is the cause, change gameplay input semantics incidentally, or redesign toolbars.
**Entry/exit:** independent of A and C; close with inert stationary middle-click,
camera-only deliberate pan, clean cancellation and a regression reproducing the symptom.

- `PIXIE-TASK-228` — **Status:** open — Reproduce the reported middle-click jump with native camera/transform readback, correct its demonstrated cause and qualify focus, drag threshold, resize and capture transitions.

### Block C — World/Scene integration handoff (P0.3; proposal 7)

**Scope:** interface agreement for identity, authored/effective properties, scene
context, Play/Stop/Reset, dirty state and rebuilding consumers.
**Boundaries:** proposal 016 and its execution/review blueprints own the separate
backend experiment. No streaming policy, second managed resolver, or backend work
inside this editor block. Its completion does not accept Luna's implementation.
**Entry/exit:** use a frozen backend contract/candidate once available; do not make
local authoring or camera corrections wait for it. Record independent backend acceptance.

The completed PIXIE-TASK-229 handoff is recorded in the
[authoring evidence](docs/evidence/authoring-2026-11-06.md#world-and-scene-handoff).
No active backend implementation task is scheduled by this block.

### Block D — Field inventory and meaningful Inspector (P1.1; proposal 6.1)

**Scope:** inventory, units, defaults, active/inactive conditions, geometry/layers,
displayed dimensions, chain/lock meaning and field ownership.
**Boundaries:** retain 053's owner review of inventory and target rules before
implementation. No changing physics from drawable edits, dropping inactive/unknown
data, or teaching PixieInspector domain rules through type-name checks. Preserve 054.
**Entry/exit:** produce one decision per inventoried field and an explicit mapping
to 053; agree the implementation owner once, with no competing mutable checklist.

- `PIXIE-TASK-230` — **Status:** open — Prepare/reconcile the field inventory under 053, obtain target-rule review, then qualify the agreed bounded changes with history, multi-selection, cancellation, persistence and native readback.

### Block E — Presets, Session and viewport controls (P1.2; proposal 6.2)

**Scope:** explain entity presets and their resulting bundles; make Session state
and editing/game input controls understandable at practical widths and DPI.
**Boundaries:** presets are not new ECS object kinds. A motor starter must not
silently promise playable movement without visible game code. Toolbar placement
is a prototype for owner review, not an already accepted permanent layout.
**Entry/exit:** use the clarified authoring and field contracts; close with the
owner-reviewed prototype and deliberate, documented action/state semantics.

Preserve the [accepted visual prototype](docs/evidence/authoring-2026-11-06.md#viewport-session-and-entities-prototype).

- `PIXIE-TASK-231` — **Status:** in-progress — Reconcile the remaining preset, Session and viewport-control scope while preserving the accepted Viewport, Session and Entities prototype. Authoring and Inspector-field gates remain separate.

### Block F — Later diagnostics and resource workflows (P2; proposal 8)

**Scope:** advanced telemetry, editor asset manager and runtime resource manager.
**Boundaries:** keep editor browsing/import/assignment separate from runtime
loading/cache/lifetime; reuse existing telemetry. No manager is a prerequisite
for A/B. This is an accepted later front, not permission to invent a broad framework.
**Entry/exit:** defer until the core loop is accepted; first formalize identity,
lifetimes, missing-resource behavior and reload boundaries, then expand numbered
tasks here under this parent before implementation. No externally published API
or cross-repository modification without its own necessary contract review.

- `PIXIE-TASK-232` — **Status:** open — After the earlier usability gates, formalize and decompose the later diagnostics/asset/resource scope, implement its accepted slices and record bounded acceptance, or obtain an explicit deferral/disposition before parent closure.

### Block G — Integrated acceptance and documentation (proposal 9)

**Scope:** one end-to-end owner exercise and evidence tying the slices together.
**Boundaries:** no blind repetition of every matrix for each UI/doc edit. Record
baseline/candidate hashes, actual commands/configurations and limits; producer
self-tests do not replace independent review or owner UX acceptance. Do not
archive unrelated entries, change project-wide documentation governance, or
claim streaming/assets/other platforms from the bounded World snapshot experiment.

- `PIXIE-TASK-233` — **Status:** open — Run the proposal's integrated exercise on normal editor outputs, reconcile each slice's independent evidence and owner UX decision, update guides and retain explicit dispositions for unfinished/deferred scope.

## `PIXIE-TODO-060` — Engine SDK, game runtime and Editor separation

**Parent:** [PIXIE-TODO-060](TODO.md#pixie-todo-060--separate-engine-sdk-game-runtime-and-editor-delivery)

The SDK and standalone Editor implementation and local cutover exist. The
[roadmap](ROADMAP.md#pixie-roadmap-060) owns the accepted scope. This list retains
only the baseline's pending dispositions, without changing acceptance or
scheduling new runs. Read the [recorded gate table](docs/evidence/sdk-editor-separation-2026-11-06.md#execution-results-and-gate-dispositions)
for the exact evidence limits, and the [decision record](docs/evidence/sdk-editor-separation-2026-11-06.md#preservation-and-decision-prerequisites)
for the accepted product boundaries.

### B0 — Contract readiness and coordinated extraction

- `PIXIE-TASK-242` — **Status:** in-progress — Reconcile actual HEAD and dirty work with completed 018; prepare D01-D12/D14 outcomes for owner decisions at their deadlines, select optional blocks, freeze C01-C12 interfaces/product profiles and exact unit/path/artifact ledgers for dispatch. Pass G0 per affected unit; reject duplicate ownership or an unresolved prerequisite. Preserve unrelated player/export/proposal work.

### K01 — Native SDK and consumer support, one agent

- `PIXIE-TASK-243` — **Status:** in-progress — B4: define and implement selected compatibility identity and low-level interop delivery, keeping Editor session/history policy in B5. Preserve ABI28/contract1 candidate checks, token/receipt layouts, module leases and teardown. G2 rejects incompatible ABI/config/profile and unsupported authored types without treating C ABI equality as C++ compatibility or promising rollback of external effects.

- `PIXIE-TASK-244` — **Status:** in-progress — B1: produce the selected relocatable SDK/profile manifest, public targets/headers, player/module inputs, compiled math, dependency closure, CPU/toolchain/CRT identity, notices and provenance. Pass G1 with B9 through an external consumer and unavailable producer checkout, including World/Scene/presentation headers and missing-dependency negatives; package mode neither rebuilds Engine nor silently falls back to its sources.

- `PIXIE-TASK-245` — **Status:** in-progress — B2: consume the qualified SDK and frozen B4 contract for game/player support, CMake helpers, templates, registry markers/schema and bootstrap. G2 covers new, recognized legacy, registered and unfamiliar custom projects, code/no-code inputs and mismatch/refusal cases; preserve edited user code/CMake, hashes/preimages, explicit contract1 admission and v1 scene-conversion safeguards.

### K02 — Export and Editor authoring, one agent

- `PIXIE-TASK-246` — **Status:** in-progress — B3: deliver the native SDK-aware exporter, accepted CLI/product policy, selected-runtime/descriptor/manifest identity and nonadjacent dependency resolution. Pass G3 with code/no-code runtime-only launch, asset/native/CRT closure, complete v2 scopes/presentation/seed/opaque data and unchanged v1 policy. Detect missing DLLs, mixed identities and development payload contamination without producer-machine paths.

- `PIXIE-TASK-247` — **Status:** in-progress — B5: make Editor an independent SDK consumer with explicit selection/discovery, portable project requirements, cache/profile continuity, selected Inspector adoption, authoring/export capabilities and owned native graphics assets. G4 exercises two SDKs, mismatch/relocation, create/open/Add code, full dirty v2 rebuild/reopen, exact targets, camera/Environment, Play/Stop, history and committed recovery. Verify the proposal 022 GLFW version and graphics smoke requirements. D09 shared-binary implementation and detailed provider qualification remain frozen. Preserve 018 in admitted package/local Inspector modes and record visual regression evidence for changed surfaces.

### B6/B7/B8 — Standalone adapters and conditional dependencies

- `PIXIE-TASK-248` — **Status:** in-progress — B6: adapt native/editor development protocols and local fixtures to frozen ABI/host/artifact inputs. Pass G5 with cold external-code consumers, shared GUI/script/channel admission, stale-edit rejection and committed recovery, positive allowed operations and negative capability/artifact contamination cases. Package fixtures must not recreate an Engine ancestor to hide a source dependency; B5 retains export UI and application hooks.

- `PIXIE-TASK-249` — **Status:** open — B7: qualify the D08-selected immutable local Inspector core/view package pair and extra feature baseline after whole-block assignment and exact clearance. Supply package-only G4 evidence without requiring sibling Inspector sources or remote publication. B5 owns Editor feature conditions, icons, semantic mapping and grouped-history adoption. Broader core/view behavior changes need a path or contract amendment.

### K03 — External evidence and release preparation, one agent

- `PIXIE-TASK-251` — **Status:** in-progress — B9: prepare isolated external consumer fixtures and acceptance inputs for G1-G5, using exact SDK/profile/toolchain/package identities and preserving affected 018 regressions. Verify available upstream artifacts incrementally; source paths must be unavailable or covered by an explicitly accepted access-verification method. Read-only live checkouts do not prove independence; never rename/delete the owner's repositories to simulate absence.

- `PIXIE-TASK-256` — **Status:** in-progress — B10 with B9 verification: the exact local candidates and Engine-to-Editor handoff passed. The recorded remaining G6 disposition concerns a verification-only hosted run and Windows x64 native provenance. This is retained planning state, not authorization to rerun qualification; D09 remains frozen.

The completed cutover, release preparation, reconciliation and guide work
(252–255), and the excluded B8 task 250, are indexed in the
[recorded dispositions](docs/evidence/sdk-editor-separation-2026-11-06.md#delivered-and-excluded-work).

#### D09 — Accepted GLFW distribution policy

The owner selected the same GLFW binary for the Editor and SDK packages on
Windows x64. Implementation and qualification of the shared binary are deferred
outside the current separation scope. The later qualification will verify the
loaded provider and OpenGL context creation, rendering and teardown. The policy
decision records no new test result.

## `PIXIE-TODO-061` — Native entity parenting

**Parent:** [PIXIE-TODO-061](TODO.md#pixie-todo-061--native-entity-parenting).

- `PIXIE-TASK-257` — **Status:** done — In-memory 2D relations, local/world poses and render interpolation passed the simple three-level World fixture in Release. [Runtime evidence](docs/evidence/native-parenting-runtime-2026-11-07.md) records rejection and lifetime checks, artifact hashes and the limits of this result.
- `PIXIE-TASK-258` — **Status:** done — Version 3 scene and fragment persistence passed the local and installed SDK fixture. The existing Editor saved and reopened the scene and its paired no-code player rendered the exported hierarchy. [Persistence evidence](docs/evidence/native-parenting-persistence-2026-11-07.md) records the scope and artifact identities.
- `PIXIE-TASK-259` — **Status:** done — The roots-only Entities list and editable Inspector tree passed the simple-scene history, unsaved code rebuild and paired shipping export checks. [Editor evidence](docs/evidence/native-parenting-editor-2026-11-07.md) records the UI and player results.
- `PIXIE-TASK-260` — **Status:** done — The separate Weapons-Lab r5 copy passed branch duplication, code rebuild, save, Play/Stop and paired shipping-player checks. [Consumer acceptance](docs/evidence/native-parenting-weapons-lab-2026-11-07.md) records the observed scope and comparison baseline.
