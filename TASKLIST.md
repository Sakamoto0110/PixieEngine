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
- When the parent completes, its section moves to
  [`docs/archive/`](docs/archive/README.md).

The full rules are in the
[documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd),
and the section shape is in the
[template](docs/templates/TASKLIST-SECTION.md).

## Open breakdowns

PIXIE-TODO-051 and its completed task snapshot are in the
[completion record](docs/archive/todo-051-shared-engine-export-mcp-2026-11-01.md#task-snapshot).

## `PIXIE-TODO-055` — Entity input and C++ component authoring

**Parent:** [PIXIE-TODO-055](TODO.md#pixie-todo-055--entity-input-controls-and-c-component-authoring)

- `PIXIE-TASK-219` — **Status:** done — Preserve dirty baseline and implement ordered platform input with repeat/modifiers.
- `PIXIE-TASK-220` — **Status:** done — Add enabled-aware generic entity controls, Behaviour callbacks and legacy compatibility.
- `PIXIE-TASK-221` — **Status:** done — Carry input/source metadata through the C API and native/editor hosts; bump incompatible ABI.
- `PIXIE-TASK-222` — **Status:** done — Generate reusable C++ components, registry and preserving CMake wiring through existing project-code tools.
- `PIXIE-TASK-223` — **Status:** done — Adopt Add component/Open code/rebuild/reopen in the normal editor with selection/unsaved preservation.
- `PIXIE-TASK-224` — **Status:** done — Qualify runtime, generation, persistence, standalone player and normal Debug/Release adoption; document limits and deliver for independent review.

## `PIXIE-TODO-056` — Editor and C++ authoring consolidation

**Parent:** [PIXIE-TODO-056](TODO.md#pixie-todo-056--editor-and-c-authoring-consolidation)

**Promotion:** owner decision, 2026-11-02. Proposal 017 preserves the originating
reasoning; this section owns execution ordering, task state and block boundaries.
All tasks start open. Do not infer an implementation result from promotion.

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

- `PIXIE-TASK-229` — **Status:** done — Qualified the minimal handoff against native/editor ABI28, scene envelope v2, presentation schema1 and cooperative module contract1. The real C++ parent/child editor rebuild passed14 checks in Debug and Release, preserving dirty custom data, empty scopes/membership, World/view/Renderer and Scene Environment/gravity/camera values while retaining the saved file. Runtime/CAPI references and editor README own the contract; ProcessReopenTests.cs is the reproducible demonstration. This completes interface integration only; it does not accept the separate historical backend experiment or close the other TODO056 blocks.

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

**Owner UX decision (2026-11-02):** accepted the delivered Viewport, Session and
Entities visual prototype: "a parte UX ficou boa, aprovo". The owner reviewed the
normal editor candidate after its technical qualification and independent read-only
review. This acceptance does not close the C++ authoring, Inspector-field or
World/Scene gates, or authorize commit/push/publication.

- `PIXIE-TASK-231` — **Status:** in-progress — Owner selected Viewport, Session and Entities for the first visual delivery. The bounded prototype is technically qualified and owner UX accepted on 2026-11-02; retain that accepted slice while reconciling the task's remaining scope. This does not close the authoring or Inspector field gates.

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

**Promotion:** owner decision, 2026-11-04. This section owns execution state,
decision prerequisites, dispatch and integration ordering. Promotion started all
tasks open. The subsequent owner dispatch below assigns preparation; it does not
claim an implementation result or accept an unresolved decision.
[Proposal 021](docs/proposals/engine-sdk-editor-separation.md) retains the source
assessment, C01-C12 contract input, alternatives and atomic path-allocation rationale.

### Preservation and decision prerequisites

Use the completed
[018 / TODO-057 baseline](docs/archive/todo-057-world-scene-reconciliation-2026-11-04.md):
product 0.3.0.0, ABI28, scene envelope v2, presentation schema1 and cooperative
module contract1. Preserve candidate/session/storage/authored-token admission,
pinned module/descriptors and teardown order, complete authored scopes and opaque
data, explicit backed-up v1 conversion, full-document history and committed recovery,
unsaved process rebuild, stable selection and canonical camera/Environment intent.
Keep qualified local motor/pool gravity and the approved global-body fallback.
This is a preservation obligation, not installed-SDK evidence or a reopening of 018.
Retain user code/CMake safeguards, selected Inspector behavior and unrelated work.

The existing [ROADMAP project guardrail](ROADMAP.md) also applies to D06:
create/open must work through Editor UI without a command-line tool, PATH change
or environment variable, and no-code projects need no compiler. SDK UI selection
and optional automation overrides must resolve through one shared contract before
native runtime/session creation; machine-local locations are not portable project
requirements. This preservation rule selects no remaining D06 alternative.

D01-D12 are **accepted** at the recorded scope. D09 implementation and
qualification are deferred. D14's physical planning baseline is accepted. Exact
extraction execution remains **open**. The original alternatives and deadlines are in
[proposal section 5](docs/proposals/engine-sdk-editor-separation.md#5-open-decisions-and-their-deadlines).
B0 records accepted outcomes and contract revisions here before the affected
implementation or gate; the owner accepts product/scope choices and block owners
prepare technical evidence. No alternative is selected by promotion. Only dependent
work waits: D01/D03/D04/D10 precede the relevant SDK/profile closure; D02/D05 precede
the ABI/interop interface freeze; D06/D07 precede consuming build/generation changes;
D08 controls the extra Inspector baseline and B7 activation; D09's implementation
and qualification are outside the current separation scope; D11 precedes exporter changes;
D12 precedes executable release policy/G6; D14 precedes new namespace reservation
and extraction. Early evidence preparation need not wait for unrelated decisions.

B7's package target is selected under D08. The dispatch register records its
whole-block assignment. Exact Inspector write authority remains separate.
Accepted D04 keeps B8 inactive. An inactive optional block gives no other
owner permission to edit that repository; record an explicit reason if its task is
eventually dropped. Separately owned local projects outside Engine are already
permitted; D14 selects their actual paths, repositories and history strategy.
No installed consumer may require sibling folders. D13 is deferred outside this
scope: VAW delivery, installation, rebind, member registration and future validation
are prerequisites of no block or gate. C11 documentation completes independently.

#### Accepted initial decisions — 2026-11-04

The owner answered "Accept this initial baseline (Recommended)" in coordinator
chat `01a1a97a-f185-7412-a8e1-a23850a4c2e5`, human message
`01a1a995-ac10-7cf1-8c49-aacb9c339539`. B0 verified the actual question and
answer. These choices permit applicable contract/path clearance; they do not
pass a product gate or authorize publication, extraction or task completion.

| Decision | Accepted outcome |
|---|---|
| D01 | Initial Windows x64/MSVC C++20 Debug and Release; record AVX2/FMA/F16C and the actual compiler/toolchain/configuration/CRT tuple. |
| D02 | Initially match exact immutable SDK build/toolchain/C++ compatibility identity, independently of ABI28 and contract1 admission/lifetime checks. |
| D03 | One relocatable public SDK with authoring/shipping profiles. Local automation is a separate private distribution, absent from public SDK, Editor and runtime payloads. |
| D04 | Retain unchanged vendored native Lib primitives/math; no upgrade or events ABI commitment. B8 is inactive and its optional packaging task is dropped below. |
| D05 | Engine delivers low-level binding source/metadata; Editor retains resolver/loading, session, history and recovery policy. |
| D10 | Public Editor supports code authoring and native export. Shipping excludes modules/channels; automation remains local-only under the retained explicit save and publication boundaries. |

The later D06/D07/D11 acceptance and shared B2/B5 selection/requirement
representation are recorded next. The subsequent D09, D12 and D14 decisions
are recorded below. Accepted D11 still requires the actual paired tool
and a frozen exporter/product interface before its dependent implementation.
D13 remains outside scope and nonblocking. B7 requires its own assignment if
selected. The B11 assignment is recorded in the dispatch register below.

#### Accepted selection/generation/export decisions — verified 2026-11-05

B0 verified the actual human reply "Accept D06/D07/D11 (Recommended)" in
coordinator chat `01a1a97a-f185-7412-a8e1-a23850a4c2e5`, message
`01a1a9a5-df31-7123-aa3f-458072ffdc14`. The question's turn started at
2026-11-04 23:59:42 -03:00; the answer has no separately exposed timestamp.
The verification date is not a claim of an exact acceptance time.

| Decision | Accepted outcome |
|---|---|
| D06 | Installed SDK selection through Editor UI or optional launch overrides; normal create/open needs no CLI/PATH/environment setup and no-code needs no compiler. Preserve portable requirements, explicit adoption, SDK-specific caches and the exact selection through build/load/rebuild/reopen. Use one selector, without checkout, ancestor or same-name fallback; machine-local locations stay outside portable requirements. |
| D07 | Evolve existing versioned templates/registry, preserving user code, custom CMake safeguards, hashes/preimages/refusals and scene formats. No competing generator or implicit scene conversion. |
| D11 | Pair exporter/runtime/descriptors to the selected SDK with explicit dependency inventories, reject mixed identities, and keep tool versus selected-runtime provenance distinct. |

The C2 technical contract below fixes one metadata representation and portable
`.pixie-code/sdk.json` requirement, with optional `--sdk` then process-local
`PIXIE_SDK_ROOT` overrides ahead of the same validated UI selection. Neither
override is required; invalid explicit input fails without falling through.
D08, D12 and D14 were answered separately below. D13 remains nonblocking.

#### Accepted remaining product baseline — verified 2026-11-05

B0 verified the owner's reply "Use this baseline" in the coordinator chat
`01a1a97a-f185-7412-a8e1-a23850a4c2e5`, human message
`01a1aaa0-edc4-7142-b26e-3a6ce92088f0`. The question expressly reserved
separate review for Inspector peer-repository edits and the actual source move.

| Decision | Accepted outcome |
|---|---|
| D08 | Preserve current local Inspector interactions in a locked local core/ImGui package pair. B7 supplies the pair after whole-block assignment and applicable clearance. B5 owns Editor adoption. |
| D12 | Independently versioned, manually triggered local release rehearsals. B10 freezes exact inputs and scoped pipeline work before candidate qualification. |
| D14 | Plan a standalone Editor at `<development-root>/PixieEditor` with a documented import anchor and preserved Engine history. Reserve creation, exact removals and move execution for the reviewed transaction. |

The R1 decision package supplies proposals for detailed mechanics. The 30-day
retention floor, initial Editor version and exact relocation and rewrite
operations remain proposals. The owner clarified in message
`01a1aaa3-4791-75d2-ac58-611b5c2d0aec` that they can supply a folder, then
authorized `<workspace>/PixieEngine_ConsumerTests`
in message `01a1aaa4-25c3-7030-8414-6512983729a4`. K03 may use that folder
for focused standalone tests. Source isolation requires a separate proof.

#### D09 — Accepted GLFW distribution policy

The owner selected the same GLFW binary for the Editor and SDK packages on
Windows x64. Implementation and qualification of the shared binary are deferred
outside the current separation scope. The later qualification will verify the
loaded provider and OpenGL context creation, rendering and teardown. The policy
decision records no new test result.

### Assignment and ownership

A batch contains whole blocks assigned to one agent. Dispatch only an entire
standalone block or an entire batch, never parts of an active assignment to another
agent. Each block has one active accountable owner across all its milestones.
The default partition below is accepted for planning. On 2026-11-04 the owner
assigned the five units in the dispatch register below for preparation; applicable
G0 clearance remains required before implementation writes.
Regroup only before dispatch, replacing the previous partition and updating the
ledger so no block acquires overlapping owners. Read-only review transfers no writes.

| Unit | Whole blocks | Internal order and external inputs |
|---|---|---|
| Standalone B0 | Contract/integration | G0 readiness per dispatched unit; root registrations and final extraction remain serial B0 transactions |
| K01 | B4 ABI/interop, B1 SDK producer, B2 consumer build/generation | B4 identity contract before B1 completion; B1/G1 before installed B2 consumption; B0 frozen inputs and B8 artifact if selected |
| K02 | B3 exporter, B5 Editor | Freeze B3 CLI/product behavior before B5 export integration; other B5 preparation may proceed; consume K01 and selected B7 artifacts |
| K03 | B9 external qualification, B10 release pipelines | B9 fixture/acceptance inputs before B10 embeds them; same owner retains early, post-move and exact-candidate verification |
| Standalone B6 | Automation | Frozen K01 ABI and K02 host contracts; final host cases need B5 artifacts |
| Standalone B7, conditional | Inspector package | D08 selection; immutable local core/view pair for K02, without a remote-publication prerequisite |
| Standalone B8, conditional | Native Lib package | D04 selection; qualified native package/provenance before K01 dependency closure |
| Standalone B11 | Documentation | Ownership map/guides from accepted contracts, final references after the B0 move; current rules suffice |

#### Dispatch register — owner decision, 2026-11-04

The owner originally requested one separate chat per whole unit, using GPT-6.1
Sol with extra-high reasoning. `B00` means B0 and `B06` means B6. On
2026-11-05 the coordinator approved using the existing B06 chat for B11 under
the owner's direction to use the existing chats. The chat owns B6 and B11 as
two complete units with separate scratch roots. Each row retains its entire
unit across milestones. All chats use host `local` and the same live checkout.

| Unit | Accountable chat | Assigned blocks | Preparation/output root | Implementation clearance |
|---|---|---|---|---|
| B0 | `01a1a98a-6968-7ef3-a654-0b2e64192c07` | B0 | `.local/separation-060/B0/` | Scheduling/contract preparation only; G0 and extraction prerequisites open |
| K01 | `01a1a98b-42e9-7822-9ad7-d1715e87f9da` | B4, B1, B2 | `.local/separation-060/K01/` | Pending applicable G0, owner decisions and exact allocation |
| K02 | `01a1a98b-4847-7d00-87f3-ede8e2c45ed9` | B3, B5 | `.local/separation-060/K02/` | Pending applicable G0, owner decisions and immutable upstream inputs |
| K03 | `01a1a98b-4db3-70a0-8355-39aa22c21e5f` | B9, B10 | `.local/separation-060/K03/` | Pending applicable G0; fixture planning may proceed without product artifacts |
| B6 | `01a1a98b-541e-7e13-a468-5512db3ed11e` | B6 | `.local/separation-060/B6/` | Pending applicable G0, frozen ABI/host contracts and capability decisions |
| B7 | `01a1aaa5-cf26-7d20-ac18-482fba6414b3` | B7 | `.local/separation-060/B7/` | P1 read-only Inspector input and scratch packaging after coordinator relay |
| B8 | Unassigned, inactive under accepted D04 | B8 | Unreserved | Contained vendor closure selected; no peer Lib writes |
| B11 | `01a1a98b-541e-7e13-a468-5512db3ed11e` (B06 chat) | B11 | `.local/separation-060/B11/` | DOC1 after coordinator relay. Further writes require exact B11 clearance. |

Coordinator chat `01a1a97a-f185-7412-a8e1-a23850a4c2e5` receives readiness,
decision/dependency requests and reviewable results under the owner's reporting
instruction. Its independent review and the owner's acceptance are distinct from
a unit's self-report. B0 alone records confirmed task-status changes. Assigned
tasks remain open; the conditional B8 disposition is explicit below. Dispatch
alone records no G0-G6 pass or physical extraction.

Preparation may read relevant sources and create private inventories, interface
proposals and evidence plans under the assigned output root. Source implementation
writes wait for B0's applicable clearance, required owner decisions and the exact
path/interface/artifact ledger. No peer-project namespace is reserved before D14.
Use unit-local `build/`, `bin/`, `obj/`, `nuget/`, `dotnet-home/`, `fixtures/`,
`artifacts/` and `evidence/` beneath those roots; mutable caches and SDK selectors
are never shared. Root/test registrations remain serial B0 transactions, Editor
project properties B5 transactions, and final extraction a later B0 transaction.
No branches/worktrees, live-source removal or publication are authorized here.

The [section 6 allocation](docs/proposals/engine-sdk-editor-separation.md#6-atomic-ownership-model)
defines each block's accepted write boundary and explicit file exceptions. Before
implementation clearance, B0 expands it into an exact repository/path/owner/operation ledger, including
new destinations, input artifact/contract identities and isolated outputs. Resolve
all intersections. Native World/Scene, physics/render algorithms, vendors, samples
and historical fixtures remain protected; any necessary amendment is bounded and
recorded before work. B7/B8 do not include an implicit generic library redesign.

Use the shared live checkout with each unit's isolated output/cache area and
immutable producer artifacts; share no mutable build/restore directories or
global SDK-selector settings. Root
registrations go through B0; Editor project properties through B5; peer documents
and workflows through their selected repository owner. Handoffs state changed paths,
contract revision, exact inputs/outputs, focused evidence, limits and affected gates.
Contract changes return through B0 and invalidate only dependent evidence. B9 owns
the final integrated run; every worker need not rerun the whole matrix.

#### Scoped G0 clearance — B0-C1

The versioned [B0-C1 contract](docs/proposals/engine-sdk-editor-separation-contract.json)
freezes K01's initial B4 identity/query/pre-factory-refusal slice and B1 generated
identity input against the sealed K01 r2 draft and actual preimages. It clears
only these nine K01 files: `capi/include/pixie_capi.h`, `capi/src/Capi.cpp`,
`capi/src/GameModuleEntry.cpp`, `capi/src/SdkIdentity.hpp`,
`cmake/sdk/PixieSdk.cmake`, `cmake/sdk/PixieSdkBuild.h.in`,
`tests/capi/SdkIdentity.c`, `tests/capi/IncompatibleModule.c` and
`tests/capi/NullModule.c`. B0 alone applies root/test wiring in `CMakeLists.txt`
and `tests/CMakeLists.txt`; all outputs stay under K01's isolated root.

This is scoped implementation readiness under accepted D01-D05/D10, not a
whole-unit G0/G1/G2 pass, installed-SDK evidence or task completion. ABI28 and
contract1 checks remain independent. Typed identity comparison excludes ordinary
documentation, post-build file hashes and per-module candidate opt-in. Preserve
the module lease and pre-factory refusal boundary, including the unavoidable
OS-loading/DllMain limit. Later B1/B2 paths and all other units need their own
applicable clearance; unrelated later choices do not block this slice.

#### Scoped G0 clearance — B0-Q1

The versioned [B0-Q1 contract](docs/proposals/engine-sdk-editor-separation-qualification-contract.json)
clears K03/B9 to add exactly `tests/separation/inputs.schema.json`,
`tests/separation/artifacts.py`, `tests/separation/qualify.py` and
`tests/separation/test_artifacts.py` from its sealed gate-scoped design. This
independent slice implements input/hash/path/receipt infrastructure and focused
validator negatives. It needs no root registration, SDK/Editor artifact, later
product decision, new peer project or runner creation.

Resolve only selected gate/case inputs, distinguish generated evidence from input
receipts, and reject/block draft or missing required inputs. Synthetic validator
success cannot pass G1-G6; unimplemented product cases remain blocked. No native
consumer, producer, release-policy or CI paths are cleared. K03 retains its whole
B9/B10 assignment and final integration responsibility; later paths and product
gate acceptance remain separate. B0 tasks242/254 remain open.

#### Scoped G0 clearances — B0-C2

The versioned [B0-C2 contract](docs/proposals/engine-sdk-editor-separation-package-selection-contract.json)
extends C1 without rewriting it. It freezes SDK root/profile/portable-requirement
metadata and one UI/automation selection contract. Exact operations/preimages
are embedded in that contract; source writes remain with their assigned owners.

K01 may add `cmake/sdk/PixieEngineConfig.cmake.in`,
`cmake/sdk/PixieSdkManifest.cmake.in`, `scripts/sdk/assemble_sdk.py` and
`scripts/sdk/verify_sdk.py`, and modify `capi/src/GameModules.hpp`,
`tests/capi/ModuleLifetime.c`, `editor/PixieEditor/Native/PixieCapi.cs`,
`tools/NativeApiVersion.cs`, `tools/NativeApiVersion.props` and
`cmake/PixieTargetDefaults.cmake`. These are ten disjoint remaining B1/B4 paths;
the original nine C1 paths remain cleared. B2 installed consumption still waits
for actual B1/G1 inputs; C3 below clears bounded generation preparation. B0 has
added install registration after K01's function-ready handoff. Initial C1 native
builds did not depend on it. B5 still owns any managed project/property transition.

K02 may add `editor/PixieEditor/Native/EngineSdk.cs`,
`editor/PixieEditor/Native/SdkRequirements.cs` and
`tests/editor/code-components/SdkSelectionTests.cs`, and modify
`editor/PixieEditor/EditorSettings.cs`, `editor/PixieEditor/Program.cs`,
`editor/PixieEditor/Ui/StartScreen.cs`, `editor/PixieEditor/Ui/EditorWindow.cs`,
`editor/PixieEditor/Native/GameLibrary.cs` and
`tests/editor/code-components/Program.cs`. This nine-file slice preserves
New/Open/Recent, pending project open and SDK UI selection before native/session
creation. Synthetic parsing/refusal/UI-state checks may run now; actual SDK and
runtime admission still need the selected artifact, and no G4 pass is claimed.

Public SDK support excludes local channel/development inputs while canonical
private/producer source stays preserved. C2 alone clears no B3, B6, CI, peer,
source-move or publication writes. D08/B7 does not block these SDK/UI slices;
all later choices and product gates retain their actual dependencies.

B0 has registered `pixie_sdk_install()` after native/player/other target setup.
It is opt-in through `PIXIE_SDK_INSTALL=ON` for dedicated top-level SDK producers;
ordinary source/game builds return without packaging dependencies. Registration
readiness is handed to K01; package production and installed qualification remain
its actual artifact work, not a pass inferred from this root call.

#### Scoped G0 clearance — B0-N1

The versioned [B0-N1 contract](docs/proposals/engine-sdk-editor-separation-native-automation-contract.json)
clears B6 to modify exactly `devchannel/src/Dispatcher.cpp` and
`tests/devchannel/DispatcherTests.cpp` for native hello identity. The authoritative
C1 identity is forwarded under `result.value.engine`; existing operation
`result.value.capabilities` remains separate. Preserve protocol1/API28, bounds,
first-hello refusal, owner-thread/authoring admission and forbidden/save behavior.
No identity is inferred from source paths, version labels or file names.

Source/test work may proceed without B5 host implementation or D08. Focused
private native tests need a coherent exact K01 input/source handoff and identity
regeneration, with isolated B6 outputs; they are not installed independence or
full G5 evidence. No CMake, Editor hook/channel, managed metadata/client, fixture,
peer or public payload path is cleared by N1. B6 retains the whole block.

#### Scoped G0 clearance — B0-F1

The versioned [B0-F1 contract](docs/proposals/engine-sdk-editor-separation-g1-fixture-contract.json)
clears K03/B9 to add exactly `tests/sdk-consumers/CMakeLists.txt` and
`tests/sdk-consumers/InstalledRuntime.cpp`. Author the headless public-target,
World/Scene/presentation/viewport and compiled-math consumer now. Missing/invalid
SDK/package/profile refusal checks do not require a complete SDK or runner.
Use explicit `PIXIE_SDK_ROOT`, `PIXIE_SDK_PROFILE` and consumer configuration,
restricted package discovery and imported generated-header propagation; do not
patch missing producer dependencies with source paths.

Actual compile/link/run G1 acceptance still needs immutable SDK/runtime closure,
exact tuple and qualifying isolation. Missing runtime input blocks runtime proof,
not source authoring. No Q1 product executor, G2 module fixture, CI/release,
source move or later product decision is cleared. K03 retains its whole unit.

Final producer clarification uses only `PIXIE_SDK_ROOT`/`PIXIE_SDK_PROFILE` and
standard CMake configuration selection: F1's `PIXIE_CONSUMER_CONFIGURATION`
maps to matching `CMAKE_BUILD_TYPE` or restricted `CMAKE_CONFIGURATION_TYPES`,
not another package selector. `Pixie::Runtime` propagates the selected generated
header. Installed config exposes `PixieEngine_RUNTIME_DIRECTORY` and
`PixieEngine_RUNTIME_DLLS`; optional `pixie_stage_runtime(target)` stages declared
selected files only. Debug developer-runtime prerequisites and Release app-local
closure are labeled from actual artifacts. This clarification changes no sealed
F1/C1/C2 bytes or acceptance gate.

#### Scoped G0 amendment — B0-M1

The versioned [B0-M1 contract](docs/proposals/engine-sdk-editor-separation-editor-integration-contract.json)
adds two exact K02/B5 modifications: `editor/PixieEditor/Native/ProjectBuild.cs`
for an actionable refusal at `EngineFolder.Find` when an installed selection is
active, and `editor/PixieEditor/PixieEditor.csproj` for K01's explicit source-mode
native metadata import. This prevents checkout fallback while B2 adapters are
pending and lets B5 confirm the metadata registration before B4's coordinated
version switch. No generated project/state write, publish guard change, broader
B2 implementation or peer-file write follows. Installed extraction still needs
selected installed metadata instead of mandatory source imports.

#### Scoped G0 amendment — B0-N2

N2 clears B6 to modify only `tools/pixie-mcp/PixieMcp.csproj` and
`tools/pixie-mcp.tests/PixieMcp.Tests.csproj` for the same explicit Source versus
Installed native metadata registration rule in M1. Before their existing
`../NativeApiVersion.props` imports, default to Source only when unspecified and
supply the project-relative C API header only in Source mode when unspecified.
Preserve publish refusals, locks, dependencies and Installed missing/invalid-input
failure; no provider, client protocol or Editor hook write is added. Isolated
source builds and metadata refusal checks need no native/Editor host. They do
not qualify installed independence, final metadata delivery or G5.

#### Scoped G0 clearance — B0-E1

The versioned [B0-E1 contract](docs/proposals/engine-sdk-editor-separation-export-contract.json)
clears K02/B3 to modify `tools/export/CMakeLists.txt`, `tools/export/main.cpp`,
`tools/export/Export.hpp`, `tools/export/Export.cpp`,
`cmake/PixieExportProduct.cmake` and `tests/export/qualify_export.py`, and add
`tools/export/Sdk.hpp`, `tools/export/Sdk.cpp`, `tools/export/Product.hpp` and
`tools/export/Product.cpp`. B0's root guard permits only the native SDK export
tool against shipping runtime; shipping-game payload still disables tools.

E1 freezes selected shipping/Release tool/runtime/descriptor pairing, C1 preflight
before C++ calls, explicit SDK CLI, product/export v2 schemas, actionable old
manifest refusal and unchanged authored scene v1 policy, declared native/CRT
closure and module-marker/runtime-query distinction. CLI/schema/validator work
may proceed now; actual tool/no-code runs need immutable shipping inputs, code
export needs B2, and real G3 still needs its artifact/isolation evidence. No B5
export UI, peer file, broad tool admission, release or source move is cleared.

B0 registers `export-producer-refusals` under the existing Windows exporter
target guard, with separate configuration work directories. This replaces the
obsolete root invocation that lacked the now-mandatory `--sdk` argument. The
producer refusal test needs no installed SDK. Paired/no-code and full export
tests retain explicit artifact inputs and their own acceptance boundaries.

#### Scoped G0 amendment — B0-E2

E2 clears K02/B5 to modify exactly `editor/PixieEditor/Native/EngineSdk.cs`,
`editor/PixieEditor/Development/ExportPanel.cs`,
`editor/PixieEditor/Development/ProjectExportJob.cs`,
`editor/PixieEditor/Ui/EditorWindow.cs`, `editor/PixieEditor/PixieEditor.csproj`
and `tests/editor/code-components/SdkSelectionTests.cs` at their current
preimages. Four paths extend their existing scope with the same writer; the
export panel and job are two newly cleared existing paths.

An otherwise valid SDK with no `tools.exporter` and absent or false
`features.pairedExport` remains usable without export. A true flag requires
one valid E1 entry; an entry with a false/absent flag, missing entry with a true
flag, malformed advertisement or private-root advertisement refuses. Admit only
the selected public authoring SDK's own shipping/Release pairing, with exact
common/build IDs, E1 schemas, and tool/runtime/dependency references bound to
that closed C2 inventory and exact shipping closure. No other SDK/tool lookup.

Preserve the saved Edit-mode guard, existing modal and process lifecycle.
Revalidate the admitted manifest and files before using `ArgumentList` to pass
`--sdk` with that same immutable root, absolute project and output. Public
non-development compilation adds only the existing export panel and job from
the Development folder. Keep channel, capture, scripted export and other
automation behind their existing guards. This receipt does not change Toolbar,
the publish validator, package choices, or any B6 path.

Source and synthetic checks may proceed with the sealed E1 handoff. Actual
Editor-to-tool launch needs a paired artifact; any bounded fixture result keeps
its own provenance and is rerun against the formal composition. Code export
still needs installed B2 support. No G3/G4, publication or move is cleared.

#### Scoped G0 amendment — B0-E2a

E2a clears two planned K02/B5 modifications at their current preimages.
`editor/PixieEditor/Ui/Toolbar.cs` exposes the existing File > Export game
action in public builds through E2's paired SDK callback and saved Edit-mode
guard. `tests/export/qualify_editor_publish.py` checks that the two E2 export
types and their File menu surface are present in the local public build.

The qualifier adjusts the export-only expectations for the two types, menu
label and associated accessor. It retains the broader Development namespace
check, private automation and dependency exclusions, exporter payload ban,
and development and NoBuild refusal cases. The two types may use a neutral
namespace within their existing E2 paths and call sites. The publish qualifier
produces local fixtures. Scripted export and channel and capture operations
retain their private guards.

#### Scoped G0 amendment — B0-M2

M2 clears K02/B5 task247 source and synthetic preparation for installed
`sdk-v1` Add Code and same-SDK build/reopen. The five existing paths are
`editor/PixieEditor/Native/EngineSdk.cs`,
`editor/PixieEditor/Native/ProjectBuild.cs`, `editor/PixieEditor/Ui/BuildScreen.cs`,
`editor/PixieEditor/Ui/EditorWindow.cs` and
`tests/editor/code-components/SdkSelectionTests.cs`. BuildScreen is newly
cleared and the other four extend the same writer's scope.

Use the admitted SDK's hashed C3 generator and support inventory when it
advertises project generation. Unadvertised SDKs remain usable for no-code
hosting and existing admitted prebuilt modules. Require a compatible portable
project requirement and preserve explicit adoption and candidate opt-in.
Recognize the validated `sdk-v1` family and registry project ID before writes,
then configure in package mode for the selected authoring role and build only
its exact module target in the selected native configuration.

Use a machine-local build cache with one SHA256 key over canonical project and
SDK roots, manifest hash, build ID, authoring role and native configuration.
Require a matching ownership receipt before reuse and carry the same key and
selection through the existing child handoff within these five paths. Preflight
before closing the active session and preserve full authored reopen data and
failure recovery. Keep the existing launch protocol, registry authority and
source build boundary. The coordinator accepted the exact individual C3 public
authoring Release B2 handoff for this bounded Editor replay. A follow-up check
confirmed the same 696 authoring Release payloads, profile and build identity,
and portable requirement in the 718-file C3 public catalog and the 729-file r4
paired catalog. Each catalog requires its own cache and runtime and build
replay. Other catalogs require a separate applicability check. G4, graphics
and Inspector qualification retain their own gates.

#### Scoped G0 amendment — B0-M2a

M2a extends two existing K02/B5 scopes at their verified preimages.
`editor/PixieEditor/Native/SdkRequirements.cs` compares portable CPU features
as exact duplicate-free sets. C3's emitted portable order differs from C1's
native order. Typed native and module identities keep their ordered comparison.
Use the already cleared selection tests to check reordered features and refuse
duplicate, missing or extra entries.

`editor/PixieEditor/Development/ProjectExportJob.cs` reuses the existing
ProjectBuild CMake discovery for admitted code projects. Pass the absolute
executable in the export child environment and use a short owned temporary
shipping work path keyed by the canonical project, SDK root and manifest, and
shipping-export role. The native exporter checks the actual SDK and toolchain.
No-code export keeps the same admitted SDK and E2 arguments and needs no
compiler discovery or explicit work path. Actual installed code export retains
its applicable paired artifact and toolchain checks.

#### Scoped G0 amendment — B0-M3

M3 clears K02/B5 task247 component generation in four existing files:
`editor/PixieEditor/Native/ProjectCode.cs`,
`editor/PixieEditor/Native/EngineSdk.cs`,
`editor/PixieEditor/Native/ProjectBuild.cs` and
`tests/editor/code-components/SdkSelectionTests.cs`. ProjectCode is newly
cleared and the other three extend K02's existing scope.

Use the admitted C3 SDK's hashed component templates and validate the portable
requirement, generated family, exact target and registry preimages through M2
before writes. Reuse the installed Add Code generator and existing managed
registry transaction. Preserve unfamiliar layouts, existing authored bytes,
collision and backup rules, candidate opt-in and attachment policy. Keep
explicit source fixture mode separate.

Focused proof uses the accepted individual C3 public authoring Release SDK.
Generate and register a component, build and load the exact module, and record
the existing type observation separately from descriptor access or Editor
authoring permission. Keep contract 0 where already selected. Report which
observations are available under that contract. Workspace presets, Inspector,
D09 and full G4 retain their own gates.

The coordinator independently accepted the 117-member M3 seal and all four
live source postimages. Fresh Debug and Release copies passed 111 synthetic
checks each. The exact installed authoring Release replay generated and built
`InstalledProbe`, loaded the selected runtime and module, and observed the
component source in its descriptor under contract 0. The local build reported
MSBuild `MSB8029`. Attachment, candidate authoring, provider qualification
and G4 retain their own gates.

#### Scoped G0 amendment — B0-M4

M4 clears `editor/PixieEditor/Native/CodeEditor.cs`,
`editor/PixieEditor/Native/CodeWorkspace.cs` and
`tests/editor/code-components/SdkSelectionTests.cs`. It connects the existing
code editor action to the admitted SDK's configured M2 authoring cache and
uses preserving edits for `CMakeUserPresets.json` and `.vscode/settings.json`.
The existing cache owner, selected SDK, source folder, generator and native
configuration must match before writes. Focused proof uses CMake preset
listing and configure on the exact installed authoring Release SDK. IDE UI
automation is outside this scope. The coordinator verified the exact receipt
and all three live preimages after M3 validation and relayed M4 to K02.

The coordinator accepted M4's installed workspace replay after checking all
120 seal members and three source postimages. Fresh Debug and Release runs
each passed 134 synthetic checks and 16 source-mode legacy checks. The exact
authoring Release project and module built, preparation was idempotent, and
CMake listed and configured `pixie-editor-local` in the same owned cache.
Project, requirement and cache owner bytes were preserved. The public
non-development closure was checked. The local MSBuild `MSB8029` warning
remains. VS Code interaction, provider qualification, isolation and G4 stay open.

#### Scoped G0 amendment — B0-M5

On the exact installed authoring Release SDK, the coordinator replayed the
original Environment, Presentation and Scene Authoring fixtures. Environment
exited successfully with 31 checks and 12 captures. Presentation and Scene
Authoring reached their private handshakes after 17 and 42 checks and exited
with errors. The loader trace identified the selected runtime in all three runs.

Separately, the coordinator replayed the production `--project` no-code v2
smoke on the same SDK. Rename, Undo, Redo, Play, Stop and final Undo completed
with a frame, exit 0 and unchanged authored and requirement hashes. The dirty
indicator remained after final Undo. The final post-Stop step count was zero.
The Environment fixture supplies separate fixed-step evidence. These local
runs leave human UI, visual, provider, isolation and full G4 qualification open.

M5 extends K02/B5's existing scope for `tests/editor/code-components/Program.cs`
and adds planned modifications to two existing K02/B5 files,
`tests/editor/code-components/PresentationTests.cs` and
`tests/editor/code-components/SceneAuthoringTests.cs`. Keep the existing command
forms and add an explicit installed route for `--visual-environment`,
`--presentation` and `--scene-authoring`. Admit the exact SDK through the
existing selection path and pass its selected runtime to the fixture.

Gate only the existing private channel portions on the admitted native
automation capability and report execution and omission separately. Preserve
every private assertion for capable SDKs and every other fixture assertion in
both modes. Run the three fixtures against the exact public authoring Release
SDK. Check the unchanged private channel cases separately with the exact C3
private Debug SDK where feasible. Production paths, registration, B6, provider
and the D08, D09 and G4 gates remain outside this fixture amendment.

The coordinator independently accepted the M5 test changes and replayed all
six installed fixture runs. Fresh Debug and Release each passed 134 managed
checks. On the exact public authoring Release SDK, Environment, Presentation
and Scene Authoring passed 31, 22 and 52 checks. The last two explicitly
reported the private channel cases as omitted. On the exact private Debug
SDK, the same fixtures passed 31, 27 and 63 checks, including all 16 private
channel assertions. Each SDK produced 12 Environment and 9 Scene captures.
The traces identified the selected runtimes with zero unresolved events.
This accepts the local test-only evidence. D08, D09, visual review, source
isolation and full G4 retain their own gates.

Separately, the coordinator accepted a paired local graphics smoke on the
exact C3 public authoring Release SDK. The published Inspector package closure
and the local Inspector project closure each completed two fresh processes
with a frame, exit 0 and zero unresolved loader events. Both mapped the same
Hexa GLFW image and the selected Pixie runtime. Inspector origin and the
development-tools switch differed together. A subsequent local IAT replay
verified all 17 sealed files and matched Hexa export addresses and image
ranges in six snapshots across the two closures. The adopted package
candidate and instrumented context lifecycle remain due for D09 and G4.

The coordinator independently accepted the D09 tracer's synthetic calibration.
All 17 seal members matched. Fresh normal, fatal, abort and watchdog cases
verified patch restoration, the expected exception behavior and owned-child
cleanup. Actual Editor instrumentation requires the matched B5 closure and
separate relay. D09 remains open.

#### Scoped G0 amendment — B0-M6

M6 extends K02/B5's `editor/PixieEditor/PixieEditor.csproj` scope and clears
the two planned field-interaction fixture files
`tests/editor/field-interactions/PixieEditor.FieldInteractionTests.csproj` and
`tests/editor/field-interactions/Program.cs`. After coordinator relay,
add one explicit package-pair route for the exact B7 P1-r4
`0.3.0-pixie021.3` core and ImGui packages. Preserve the ordinary package
and explicit project-reference routes and the existing publish refusals.
Derive the feature condition and existing icon staging from the same
selection. Keep the fixture's original invocation and add exact installed
SDK selection. Gate only the existing private channel block on admitted
native automation and report execution or omission.

Use the exact accepted B7 feed inputs and a separate locked full Editor
restore in scratch. Keep the production graphics comparison and fixture
builds distinct. Exact package adoption, installed field behavior, matched
graphics lifecycle, visual review and source isolation need their own evidence.

#### Scoped G0 amendment — B0-M6a

M6a extends K02/B5's existing `tests/editor/field-interactions/Program.cs`
scope only. Preserve the three-argument legacy form and M6's runtime-only
installed form. The explicit installed form may also take
`--module <absolute-path> --module-sha256 <sha256>`. Verify the digest
before work creation and admit the module through the existing selected SDK
and loader. Require the `WriteProbe` descriptor for this requested mode.
Use a disposable fixture entity as needed and record execution of both
existing throwing-peer assertions.

Use the sealed N7 module with the exact private Debug SDK for the package
and local fixture controls. Run a wrong-digest refusal and a separate public
SDK crossed-module refusal. The publish qualifier and M6 acceptance retain
their own review.

The coordinator independently accepted the bounded M6 and M6a deliveries.
All 1,011 seal members and scoped postimages matched. Sixteen fresh M6
fixtures passed across package and local builds. Six fresh M6a positive runs
retained the runtime-only counts and exercised both throwing-peer assertions
in each private probe mode. Wrong digest, crossed SDK and raw preimage
controls were verified. A fresh package publish rendered, rejected the
private export command with exit 3 and preserved the fixture without
producing export output. The corrected recovery restored the dirty v2
handoff and preserved authored bytes. The earlier AMD fault and retry
observation-path error remain separate. Static qualifier repair, D09 product
trace, source isolation, human visual review and full G4 remain open.

The coordinator verified the recorded startup of the exact M6 private
Debug save-on host with the selected Inspector pair and private runtime.
All 133 seal members matched. It is an input for B6's separate protocol
and recovery qualification. The coordinator also verified the distinct
54-member M6 package-publish A/B observation. Four no-code opens and two
pre-runtime refusals matched on the exact SDK catalogs. The checkout was
readable, and code support stays on SDK A.

#### Scoped G0 amendment — B0-M6b

M6b extends `tests/export/qualify_editor_publish.py` and adds only
`tests/export/InspectEditorPublishIl.cs` from the exact reviewed delta
after coordinator relay. Keep the two-argument qualifier and all existing
publish and payload checks. Replace the whole-image `export` string test
with the method-level check that permits the public cache use only in
`PixieEditor.Native.ProjectBuild.Cache` and rejects it elsewhere.
Verify the default and selected-package qualifier paths and refusal of
the development command reference. This is a test-only clearance.

The coordinator independently accepted M6b's qualifier repair. All 19
seal members and both live postimages matched. The default and selected
package publish reports each recorded exits 1, 0 and 1 for the development
guard, shipping publish and no-build guard. A fresh offline build of the
repository helper accepted the public cache use and rejected the private
command reference. Full G4 and release acceptance remain open.

#### Scoped G0 amendment — B0-L1

L1 adds two exact K02/B5 operations: add
`editor/PixieEditor/Native/EditorLaunch.cs` and modify
`editor/PixieEditor/Ui/SessionPanel.cs`. A pure apphost/dotnet launch builder
preserves the already admitted C2 selection and action/reopen arguments, with
explicit sibling module paths instead of names needing lookup. Existing cleared
EditorWindow/tests may consume it. Synthetic argument/state checks need no SDK
artifact or D08; actual process/dirty-reopen continuity remains G4 evidence.

#### Scoped G0 amendment — B0-X1

X1 explicitly adds two B9 paths to K03's ledger:
`tests/separation/g1.py` and `tests/separation/test_g1.py`. They implement focused
configure/build/fixture-CTest orchestration and standard-library mock controls
from explicit immutable C2/F1/tool/runtime inputs. Existing Q1 and F1 files stay
unchanged except independently requested fixes. Select expected nonzero fixture
tests and require their actual receipts; preserve incomplete results on drift or
launch failure. Arbitrary hashed provenance is not real producer proof.
Synthetic inputs stay synthetic, and `productGatePassed` remains false in this
initial execution-adapter scope. Actual G1 still requires its qualifying evidence.

#### Scoped G0 amendment — B0-N3

N3 clears four exact B6 modifications: `tools/pixie-mcp/Program.cs`,
`tools/pixie-mcp/EngineConnection.cs`, `tools/pixie-mcp.tests/Program.cs` and
`tools/pixie-mcp.tests/EditorQualification.cs`. Consume C2's explicit selection
and C1's typed engine identity/schema/capabilities before mutating MCP requests.
Native player receives only its supported project/channel/capture/hidden options;
Editor receives the same resolved SDK flags through B5's selector. An explicit
dotnet Editor host may use an absolute assembly input without discovery. Keep
engine identity separate from operation capabilities and preserve bounds,
uncertainty, deadlines and nonretry behavior. Source/synthetic refusal checks
can proceed now; real private/native/Editor/cold-code and independence stay their
own G5 cases. No EditorChannel/provider, driver, lock/dependency or other path
is implicitly cleared.

#### Scoped G0 amendment — B0-N4

N4 clears B6 to modify only `tests/editor/channel/NativeWriteProbe.cpp` at its
verified preimage. The existing custom entry adds C1's `pixie_game_sdk_identity`
export from the module's own generated tuple header and compile checks. It
retains the single factory, the existing API query, the read-only descriptor
and the setter refusal before storage mutation. Keep compatibility identity
separate from candidate-authoring and custom-type admission.

Use an exact matching private `local-development` SDK tuple for actual probe
admission and distinguish a pre-factory identity refusal from the setter
refusal. Isolated probe builds and native controls provide focused evidence.
Real Editor, cold code and G5 retain their own artifact and acceptance gates.

#### Scoped G0 amendment — B0-N5

N5 clears B6 to modify exactly `editor/PixieEditor/Development/EditorChannel.cs`
and `tests/editor/channel/Program.cs` at their verified preimages. Clear the
Editor client epoch's readiness at every recognized hello attempt, before a
validation failure can leave an earlier admission in place. A hello notification
uses the existing owner-thread request helper to get a native response and stays
silent on the external wire. Both forms admit only after native success with an
engine object. Preserve the native identity, Editor operation overlay, client
reset and edit, recovery and history guards.

The test keeps its explicit probe, scratch and result paths and takes the C2
SDK root, profile, native configuration and expected SDK, build and manifest
identities. Use the existing B5 `EngineSdk.Load` and `GameLibrary.SelectSdk`
before `GameLibrary.Use`, with a fresh process for each selection. Rebuild the
N4 probe for the exact C3 private Debug tuple before positive tests. Run focused
hello and Inspector controls in isolated outputs. This amendment covers the
existing test entry point. Cold code and full G5 retain their own gates.

#### Scoped G0 amendment — B0-N6

N6 clears B6 to modify exactly `tests/mcp/qualify_mcp.py`,
`tools/pixie-mcp.tests/EditorPhysicsQualification.cs` and
`tools/pixie-mcp.tests/EditorCodeQualification.cs` at their verified preimages.
The driver takes explicit immutable Editor save-on and save-off closures,
bridge and client outputs, selected private SDK and expected identities,
same-SDK probe and N5 runner, and fixture inputs. Resolve the player from its
selected private profile manifest. Use the same C2 selection for Editor,
bridge and test admission and preserve the native player's supported arguments.
Stage projects, captures and logs under the caller's owned work root.

Preflight and focused private physics and no-code replay may proceed with the
declared inputs. The physics client retains its existing second-session, v1
migration and authoring checks. The cold client may prepare exact launch
arguments and omission controls with synthetic inputs. Actual cold-code replay
requires the exact private authoring/build handoff and an explicit never-built
installed fixture. Record omission when they are missing. A deadline control
uses its own declared bounded-delay input. The caller may stage the existing
legacy scene with its receipt. Keep the existing root, CI and peer boundaries.

The coordinator verified and relayed N6. The staged input review checked all
707 SDK payloads, 202 files across six closures, both full seals and both
fixtures. This confirms the declared inputs for the scoped driver work.

#### Scoped G0 amendment — B0-N7

N7 extends B6's existing scope for `tests/editor/channel/Program.cs` only.
Keep the absolute-probe invocation and its 38 checks. Add a separate fresh
process invocation with `--committed-recovery` in the probe slot and the same
remaining arguments. Reuse the explicit SDK selection, hidden GL and named-pipe
fixture setup. The new mode loads the selected C3 private Debug runtime and
exercises the real Scene publication and rebind behavior from the accepted
scratch recovery proof through the existing test-owned router.

Keep the wrong-build refusal before project and capture creation. Verify both
invocations, committed state and retained Undo/Redo, and preserve saved bytes.
The existing driver and product entry points stay outside this clearance.
Standalone Editor, external modules, provider qualification and full G5
retain their own gates.

The coordinator verified the N7 receipt and exact live test preimage and
relayed the one-file clearance. The same-host preimage run reproduced all
38 existing N5 checks with unchanged host and SDK files.

The coordinator independently accepted the durable N7 test entry on the exact
C3 private Debug SDK. The fresh copied closure reproduced all 38 N5 checks
and passed all 18 committed-recovery checks. The wrong-build control exited 1
before creating project or capture output. All 49 host files and 707 SDK
payloads were preserved. This qualifies the direct in-process Debug fixture.
N6 driver integration, standalone Editor, external modules, provider
qualification, source isolation and full G5 remain open under B6.

#### Scoped G0 amendment — B0-N8

N8 extends B6's existing scope for `tests/mcp/qualify_mcp.py` only. The
coordinator relays N8 after validating DOC1 against the exact N6 driver.
Add an explicit `--include-committed-recovery` option for the admitted
private Debug tuple. Preserve the default N6 behavior and schema 1 inputs.
Run the N7 recovery entry as a separate suite with its own process, project
root, log and receipt. Opt-in preflight records omission. A Release opt-in
refuses before creating a run or launching suites, and an explicitly
requested legacy artifact failure remains visible in the suite results.

Validate the unchanged cold-deferred 202-check baseline and the opt-in
run with the separate 18-check recovery receipt. Preserve cold readiness
and omission controls. The prior 38-check probe and direct N7 wrong-build
proof retain their own scopes. Production Editor, provider qualification,
source isolation and full G5 remain separate.

The coordinator independently accepted the bounded N8 driver delivery. The
fresh exact closure replay passed the 202 default checks and 220 with recovery
opted in. The legacy artifact failed only the requested recovery suite.
Release refusal and preflight omission passed, and cold dispatch vectors
matched. Packaged Editor, external-module recovery, isolation and G5 remain
open.

The coordinator separately accepted local external-module committed recovery.
The exact 109-member seal replay preserved all 49 host files and 707 SDK
payloads. The contract1 module passed 21 checks across 19 wire calls. The
same-tuple contract0 control passed nine checks across three wire calls.
This run used a source-built host and fixture-owned rebind fault.

The coordinator then accepted the distinct packaged friend replay on the
exact private M6 save-on Editor and selected Inspector pair. All 106 seal
members matched. The fresh replay preserved the 47-file Editor closure and
707 SDK payloads. Contract1 passed 21 checks across 19 wire calls and
contract0 passed nine across three. Both exited 0 and were reaped. The
standalone app-process fault and normal module apphost publication, provider
qualification, source isolation and full G5 remain open.

#### Scoped G0 clearance — B0-C3

The versioned [B0-C3 contract](docs/proposals/engine-sdk-editor-separation-generation-contract.json)
clears K01/B2 to modify `cmake/PixieProject.cmake`, `cmake/PixieGame.cmake`,
`cmake/PixieAddCode.cmake`, `cmake/PixieComponentRegistry.cmake`,
`cmake/PixieInitComponents.cmake` and `tests/project-code/ProjectCodeTest.cmake`,
and add `templates/project-code/versions.json`,
`templates/project-code/sdk/CMakeLists.txt.in` and
`tests/project-code/PreservingGeneration.cmake`. Source and synthetic preserving
generation/role checks may proceed now; actual installed consumption still needs
the corresponding B1/G1 inputs, and full G2 needs its exact qualified candidate.

Authoring is the default project build role. Explicit `shipping-export` consumes
the matching public shipping/Release profile without rewriting an authoring
project requirement; private selection cannot fall back to public. Versioned
template inventories, registry markers, preimages and user edits remain guarded.
Candidate-authoring opt-in requires the explicit known generator contract; legacy
recognition alone grants neither contract1 nor unsupported authored types. B0
registered `project-code-preserving-generation` after verifying K01's script hash
and command. The Windows CMake harness resolved separate Debug and Release work
paths, and the coordinator independently accepted the wiring. Release passed
all 31 original source and synthetic checks. Changed generation
support produces new SDK identities and affected evidence rather than relabeling
an earlier F1 seal. The existing K01/B0 write boundaries still apply.

The coordinator reproduced an adoption defect after replaying the 31 cases.
`PIXIE_PROJECT_ADOPT_SDK=INVALID` returned success and wrote both generated
code and SDK requirements. K01 corrected adoption to require literal `ON` and
reject invalid values before writes. The coordinator independently accepted all
34 corrected source and synthetic checks, including invalid and disabled
adoption, shipping-role generation refusal, and explicit export role selection.
Installed B2 consumption and G2 await their exact candidate evidence.

#### Confirmed bounded evidence — C1, Q1 and incremental identity

On 2026-11-05 the coordinator independently validated the sealed C1-r1 native
identity/query/pre-factory-refusal submilestone: all 143 members matched manifest
`25a4ec820e5a7a03f5a438c53c6e74f96cfc8d28223b773465437d381f37330f`.
Its independent runs passed authoring Debug/Release, shipping Release and private
Debug/Release: 74/74/22/74/74 checks, 318 total, zero failures. Source review
covered typed identity/refusal, bounds, tuple generation and retained leases;
producer focused regression receipts retain their producer-evidence boundary.
This completes only C1's bounded submilestone, not full B4/task243, B1/G1,
B2/G2, installed independence, visual/runtime or release acceptance.

On 2026-11-05 the coordinator independently reviewed the four Q1 source hashes
and reran [the structural tests](tests/separation/test_artifacts.py) with the
absolute frozen Q1 contract and an isolated review root: exit0, 42 discovered,
41 passed and one skipped because real Windows symlink creation lacked
privilege. Other path/link controls remain covered; no actual junction/symlink
lifecycle qualification is claimed. Q1 is a completed structural slice only;
task251, the whole K03 unit and G1-G6 remain incomplete.

The coordinator also confirmed a configure-only SDK hashing defect: a normal
build after editing an existing native input could change compiled bytes while
retaining stale identity/inventory. K01 registered every resolved identity input
in `CMAKE_CONFIGURE_DEPENDS` in its own cleared helper. The corrected helper
snapshot was `b54519d8f79025139adcfd11b2115f365c11db0b2b5f1c0cb01b6848050b3145`.
In a disposable minimal producer, ordinary build after a content edit changed
the library, inventory and SDK identity. A separate guard-disabled copy changed
the library while identity/inventory stayed stale, confirming sensitivity.
These are independent helper-copy results, not Engine/SDK/G1/G2 qualification;
K01 still supplies affected-profile evidence from the corrected actual source.

The coordinator later confirmed bounded source managed/UI readiness: generic
managed metadata queried admitted C1 authoring/private hosts without recompiling,
and K02 source selection/guard/import builds passed Debug/Release with 53 checks
each. Source/picker frame inspection and metadata controls do not qualify an
installed Editor, full 018 rebuild continuity or G4. The coordinated source
metadata version switch does not select a runtime SDK build identity.

N1's linked-C1 native adapter passed independent Debug/Release dispatch runs
(21 tests each), identity fault controls (118 checks each) and the intended
renamed-engine mutation oracle. Its pipe supplement passed 14 checks per
configuration. These standalone transformed-wrapper/private-error-relay results
do not qualify actual DLL TLS/integrated C API dispatch, regenerated private SDK,
installed independence, Editor/MCP/cold code or full G5. N2 separately passed
bounded metadata registration review: 85 members, eight assemblies and 18
expected metadata/publish observations; no protocol/SDK acceptance follows.

The coordinator matched all 877 listed N3 r1 seal members. Independent replay
failed at startup because the runnable seal omitted Windows RID assets,
including `System.Diagnostics.EventLog`. The coordinator independently accepted
the corrected 807-member r2 seal for its bounded synthetic and private Debug
scope. Fresh replay passed 428 assertions in each managed configuration and
27 actual native cases, with three loaded module inventories. Full G5 remains
open.

The coordinator verified the 35-member N4 seal and independently passed all
67 actual private Debug C API checks for module identity and descriptor faults.
This qualifies the exact native probe and selected private tuple. Editor,
Inspector, Release, cold code and G5 remain separate.

The coordinator accepted N5's source-built in-process Editor seam on the exact
C3 private Debug SDK. The 236-member seal and both source postimages matched.
All 38 checks passed from a fresh 102-file copy, and the 707 SDK payloads were
rehashed. The wrong expected build, C2 probe on C3, and original-channel
failed-notification regression produced their expected refusals. This earlier
in-process N5 fixture used simulated recovery. The standalone Editor, cold
code and committed recovery replays have separate evidence below.

The coordinator independently accepted a separate source-built standalone
private Debug no-code Editor and official MCP replay. All 216 seal members
matched before and after. Both save modes passed 41 assertions from fresh
short scratch, rehashed 707 SDK payloads, matched loaded Pixie, GLFW and ImGui
hashes, captured one PNG each and exited both owned hosts. Save-on changed only
the scratch scene, and save-off preserved all three input files. This seal
covers source-built no-code operation. Packaged Editor, provider and G5 remain
open.

The coordinator independently accepted N6's local explicit-package driver.
All 386 seal members matched before and after, and all three source postimages
matched. The replay rehashed 707 SDK payloads and passed 202 actual checks:
27 player, 41 per Editor save mode, 38 pipe and fault, 54 physics and one
owned orphan-host case. It recorded the cold suite as omitted in this replay.
Packaged Editor, provider qualification, source isolation and full G5 remain
open.

B6's read-only assessment confirmed that the exact C3 private Debug package
can plan `local-development` authoring requirements for never-built projects.
The coordinator accepted K01's generated-only success and deadline fixtures.
Fresh script-only generation from two authored preimages each reproduced all
ten entries per fixture byte for byte, including the v1 Spinner scene with
speed 1.5. There were no build, cache or module outputs, and all 707 SDK
payloads matched before and after.

The coordinator separately accepted the combined M2, M2a and N5 source-built
private Debug host input. The 223-file seal and 159 source references matched.
A fresh no-code startup exited successfully with empty PATH and no ambient SDK
state. The loaded C3 private Debug runtime path and hash matched, all 707 SDK
payloads were checked and the inputs were preserved.

The coordinator separately accepted B6's scratch-only deadline launcher input.
All 76 seal members matched before and after. Normal version output matched
the pinned CMake bytes, and the exact ten-file deadline fixture entered the
delay and cancelled before configure was forwarded. No build directory was
created and all 707 SDK payloads stayed intact.

The coordinator independently accepted the focused private C3 Debug cold
Editor and official MCP replay from all three inputs. All 177 seal members
matched before and after, and all seven actual checks passed. The absent
project and exact three-second deadline refused before channel admission.
The deadline stopped before configure was forwarded. The success case built
and loaded `ColdSuccess-module` in Debug and reported the selected typed SDK
identity. Spinner was readable and the 90-degree write stored pi/2. All 707
SDK payloads and the bytes of both ten-file fixtures were preserved. The
owned processes exited and the only extra scratch project file was `imgui.ini`.
This covers the source-built host and existing Inspector mode. Save-off on
the combined host, packaged Editor, provider qualification, source isolation
and full G5 remain open.

The coordinator separately accepted focused committed recovery in N6's private
no-code Debug friend fixture. A fresh replay matched all 73 seal members, 49
host files and 707 SDK payloads and passed 17 checks over 19 actual named-pipe
calls. The fixture-owned router used production Scene publication, channel
and rebind behavior. The committed Scene and one Undo entry survived the
callback failure and both retries without replay. Mutations and capture were
blocked during recovery, stale generation refused, saved bytes matched and
Undo and Redo restored the exact snapshots. Standalone GUI, external modules,
provider qualification, source isolation and full G5 remain open.

F1 r1 was held and superseded. The coordinator accepted r2's bounded source/
configuration/refusal slice after seal/source review and eight fresh config/
wrapper cases, including actual configuration-form compatibility. No actual
consumer compile/link/run or G1 was accepted by that fixture-only result.

The coordinator subsequently accepted F1 r3's focused actual public authoring/
Release run: all 37 sealed fixture members matched, and 675 SDK payload files
were copied and hashed into a fresh review root. Identity preflight passed 11
checks; compiled math, World/Scene/presentation/viewport execution passed 42
checks with the expected SDK/build/runtime path and hash. A wrong expected
runtime hash refused after C identity and before C++ calls. This is focused
headless compile/link/run evidence, not full G1 source-unavailability, other
configurations, module, Editor or graphics acceptance.

The coordinator independently replayed the separate F1 authoring Debug seal.
All 18 seal members matched, 11 identity checks and 42 headless checks passed,
and the observed runtime matched the selected DLL. Actual loaded CRT paths
remain unmeasured. The coordinator also reviewed five native negative case logs
and checked 3,484 external inventory files. A fresh missing-transitive-math
build reproduced `LNK2019` for `InverseSimd`. X1 passed 29 mock executor tests,
and its historical provenance input was refused with an incomplete result and
`productGatePassed=false`. G1 remains open.
The coordinator accepted the actual local C3 authoring Release X1 tuple after
verifying the 30-member seal, 696 SDK payload files and phase logs. Replay passed
11 identity and 42 headless checks. K01 received the exact tuple for the local
B2 positive. K03's read-only discovery found no qualified runner with producer
sources unavailable. Full G1 and G2 remain open.

The coordinator accepted the first G1 artifact-only ZIP for staging. All 704
member paths, sizes and hashes and the ZIP CRC matched, including 696 SDK
payloads. Transfer to an isolated runner and isolated execution retain their
own scope.

The coordinator separately accepted focused standalone Release G1 evidence
in the authorized consumer folder. All 40 seal members and 704 staged ZIP
members matched. A fresh configure, build and CTest run each exited 0, with
11 identity checks and 42 headless checks. The CMake cache and Ninja build
files contained no producer root dependency, and the loaded local runtime
matched the exact digest. Both producer source probes succeeded. That run
left the producer access boundary to be qualified separately.

In a separate folder-only access probe, the coordinator ran the same Win32
binary under a normal token and a restricted process token. The normal
run opened both producer sentinels. The restricted run read the scratch
marker and system executable and received `ACCESS_DENIED` for both producer
sentinels. The access grants covered only two new scratch files. The pinned
compiler and consumer process tree still needs an end-to-end isolation
proof. Restricted console initialization had failed in an earlier attempt.

The coordinator separately verified the fresh ConPTY startup. The same
restricted identity denied both producer reads, permitted owned scratch
output and launched a default-flag CMake child. This enabled the bounded
Release X1 process-tree attempt.

The coordinator verified the restricted Release A attempt and all 49
seal members. Both producer opens were denied before and after the run,
while staged reads and owned writes succeeded. Unmodified X1 exited 2
during input validation when its ancestry check could not stat
`<development-root>`. Configure, build and test did not start. Owned
process cleanup completed. Desktop automatic review stopped the subsequent
restricted calibration as a possible cybersecurity risk. The coordinator
halted that method and retained the r1 and partial r2 records as calibration
evidence.

The owner then ran the sealed 704-member Release A handoff from a separate
standard Windows account on the same host and returned the copied runner
folder. The coordinator audited the completed run and accepted the
account-isolation method for this exact headless tuple. Both producer
sentinels returned Win32 `ACCESS_DENIED` before and after under the same
verified standard account. Staged reads and scratch writes succeeded, and
the producer account could still open both sentinels. The approved input
changed only `draft` to `false`. Exact toolchain, configure, build and CTest
receipts, all 16 fixture references, 11 identity checks, 42 headless checks,
23 typed fields and the app-local runtime matched. The cache and native
build metadata showed no producer path dependency. Independent review SHA
`cc1c4734b900e76a9975ac39df4856e4c67ddbbc6995d3fdfac88f276bf36f68`.

The independent account audit supplies the source-unavailable finding for
that run. The returned X1 result retains `focused-producer-pass`,
`productGatePassed=false` and `sourceUnavailable=not-run`. This acceptance
covers only the exact headless Release A run.

The coordinator also accepted the focused local C3 public authoring Debug X1
tuple. All 38 seal files and both 688-payload SDK trees matched. Git, inventory
and configure, build and CTest receipts were verified. A fresh artifact replay
passed 11 identity and 42 headless checks and verified all 23 typed identity
fields. Loader traces matched the staged Pixie and GLFW files and four
System32 developer CRTs. The exact Debug tuple was relayed to K01 for its
focused replay. Producer sources remained readable, so full G1 and G2 stay open.

The owner subsequently ran the sealed five-case batch under the same
reviewed standard Windows account. The coordinator accepted all five
focused results. The audit matched 1,772 immutable files and 704/696/30
staged archive members. Seven access receipts showed Win32 `ACCESS_DENIED`
for both producer sentinels, with successful staged reads and scratch
writes. Independent review SHA
`53dbd418f71abe5735fe204b623358a8cc55283535af2b619801ea78a1a300bc`.

- **G1 Debug D** — Configure, build and CTest each exited 0, with 11
  identity checks, 42 headless checks and 23 typed fields. The separate
  replay of the exact built consumer in identity-only and headless modes
  mapped the four pinned System32 Debug CRT files and app-local Pixie and
  GLFW, with zero unresolved events. X1's own focused status is unchanged.
- **G2 Release A** — A1 passed all seven installed preservation script
  cases. A2 passed binary-only replay of the existing headless admission
  probe with three preserved authored scenes and ten app-local DLLs.
- **G3 sole-v2 playback** — The existing no-code and code exports each
  started on OpenGL 4.5 and exited 0 with app-local dependencies. No-code
  rendered the pinned sprite and text. Code covers empty-scope startup.
  Both frames matched the corresponding local runs byte-for-byte. The AMD
  driver hash and reported context version differed. This result covers
  playback of the existing exports. Isolated export and graphics provider
  qualification retain separate evidence requirements.

Other applicable profiles, including SDK B's comparison profile, and
remaining graphics cases need separate evidence. Full G1 through G6 remain
open. D14 execution still requires separate authorization.

The coordinator also verified the separate 697-file public C2 composition with
authoring Debug/Release and shipping Release. Its root manifest hash is
`4923c2e718a6bf3628b83b458be65462c4e5573fb9a47aebdda76b1380792d43`.
Project-generation and paired-export features remain false in that candidate.
The earlier F1 run remains bound to its own SDK bytes and does not qualify this
different composition automatically. No whole task or product gate is complete.

The coordinator independently verified C3's 718-file public and 707-file
private SDK compositions, all archive members and links to five exact producer
tuple receipts. The new authoring Release tuple and prospective provenance
were routed to K03 for actual X1 execution. K02 received the new shipping
identity for its paired exporter build. Installed generation and code export
each use separate artifact evidence.

The coordinator independently replayed the 19-member C3 explicit-SOURCE
runtime handoff. The C API module and llvmpipe player runs passed and authored
files were preserved. A separate packaged authoring Release no-code player
replay passed with its project preserved. Two other producer tuple logs and
the 22-member C3 fixture handoff were verified for readiness.

The coordinator accepted the focused installed B2 replay on the individual C3
public authoring Release SDK. Ten seal members and 50 external references
matched before and after replay. Both modules reported C API 28 and the same
expected native identity. The default candidate contract was 0 and explicit
opt-in was 1. C API loaded Spinner and the relocated llvmpipe player exited
successfully with authored and payload bytes preserved. This exact tuple is
admitted for M2's bounded replay. Source-unavailable G1 and G2 remain open.

The coordinator independently confirmed the catalog applicability review with
2,143 member checks. The public merge adds 22 entries and the r4 pairing adds
11 more. Existing authoring Release payloads and the portable requirement match
across all three catalogs. The earlier installed replay remains bound to the
individual catalog.

K03's additional local B2 rebuild used that individual authoring Release SDK.
The first run hit MSBuild FileTracker `FTK1011` during CMake's compiler check
at a 260-character attempted log path. All seven commands passed when replayed
in a fresh, shorter owned directory. The coordinator verified the 23-member
seal and accepted this focused rebuild with producer sources readable.
Source-unavailable G1 and G2 remain open.

The coordinator accepted the separate local B2 replay on the exact C3 public
authoring Debug SDK and its declared developer CRT. Ten seal members and 40
external references matched before and after replay, and seven producer
command logs were checked. Both modules reported C API 28, contracts 0 and 1,
and all 23 expected typed identity fields. C API loaded Spinner, and the
relocated llvmpipe player exited successfully with staged and authored bytes
preserved. The host could still read producer sources. Full G1 and G2 stay open.

The coordinator accepted L1 r1 as a bounded launch milestone: all 915 sealed
members matched, source review passed, and independent fresh Debug/Release runs
passed 60 checks each. A separately copied source-built Editor, selected SDK and
no-code v1 project ran from Unicode/space paths with empty PATH, no process SDK
root and explicit SDK selection. Exit0, selected runtime/GLFW loaded hashes,
unchanged project/scene bytes, no implicit adoption and the captured rendered
Editor frame were independently checked. This remains local Inspector/source-
built evidence with producer sources readable; D09/provider policy, two SDKs,
dirty rebuild, history/recovery and full G4 remain open.

The coordinator independently accepted E1 r2's focused actual paired fixture:
52 tool-handoff and 896 validation members matched their seals, and 24 checks
passed in the shorter fresh replay directory. The first long-path compiler
probe failure is retained. This covers its exact private qualification fixture;
the coordinator later accepted the formal 708-file composition for tool identity,
no-code export and verification. The new manifest retains all 697 previous
payload files and profile bytes and adds the 11 paired tool files. The replay
preserved project bytes and ran with an empty PATH and the SDK environment
override unset. Source isolation, installed code export and G3 remain open.

The coordinator launched a relocated copy of the formal r2 no-code export from
an unrelated working directory with an empty PATH and the SDK override unset.
The player exited successfully and preserved the game files. The recorded
loaded module hashes match the app-local Pixie runtime, GLFW and CRT, with
OpenGL loaded from System32. This evidence covers that r2 export. G3 and the
D09 provider decision remain open.

E2 reached the selected exporter and exposed a failure in its default cache
path. The coordinator verified the formal r3 708-file composition and replayed
tool identity and default-work export successfully at a 122-character project
root. A 175-character project root still fails during cache directory creation.

The coordinator accepted the C3 r4 exporter for focused native no-code export.
Both seals with 42 and 816 members and the exact shipping closure matched.
All 29 paired checks passed, including project roots of 122 and 175 characters.
The formal 729-payload SDK passed independent identity, export and verify
at 175 characters with an empty PATH and the SDK override unset. This resolves
the tested r3 cache failure in r4. Code export and G3 remain open.

The coordinator verified K01's exact K02 proof input inventory, including
catalogs, profiles, runtime bytes and 28 authoring support files per tuple.
The r5 exporter seal's 42 members and 11-file shipping tool closure matched.
Only the exporter executable digest changes from r4. K01 received the inputs
for a new composition. The coordinator independently accepted the formal r5
729-payload SDK and the paired tool identity. Rehashing all three roots showed
that r5 preserves the 718 base payloads and carries the exact 11 tool payloads.
The tool identity replay ran from fresh scratch with a System32-only PATH.
Code export and the Editor replay remain under K02 qualification.

The coordinator accepted a separate local artifact replay of the provisional r5
code export. Eight existing case logs matched their expected outcomes, including
compiler refusal, lock refusal and a stale descriptor. The 15-entry game manifest
was replayed and verified in fresh scratch. The player ran without an inherited
PATH, captured a PNG and preserved output hashes. This result is bound to the
provisional catalog.

The coordinator verified the separate formal r5 seal with 857 files and 729
SDK payloads, plus producer receipts for eight code, 29 paired and 13 preflight
cases. The formal 15-file code game verified and launched after relocation with
an empty PATH, captured a PNG and preserved output hashes. A fresh no-code
export and verification also passed from a 175-character project root and a
290-character manifest path with an empty PATH. The accepted scope is formal
artifact replay and no-code generation.

K03's fresh formal r5 code build and replay passed focused local B3 review.
The coordinator verified all 65 seal members and the report, run and audit
references. The replay exercised the installed shipping target, failed-build
retry, lock and stale-product refusals, both 15-file inventories, v1 and
complete v2 authored data and asset preservation, shipping capability bits 4,
and refusal of all four development options. K03 used a local x64 developer
shell. K02's separate Editor replay exercised automatic CMake discovery.

The formal r5 sample artifact review matched 97 seal members and all 33
producer receipts and diagnostics. Independent relocated Boids and Sprites
verification and launches passed with empty PATH, PNGs captured and output
files preserved. This covers the exact local sample artifacts.

A separate malformed-closure control exposed an r5 dependency failure. With
the local GLFW removed, the exporter returned identity successfully when
shipping bin was on PATH. The same bytes and arguments with empty PATH failed
before identity with `0xc0000135`.

The coordinator verified the r6 tool and validation seals with 42 and 813
members and the exact 11-file closure. Only the exporter executable changed
from r5. In a fresh private qualification copy, the malformed r5 tool returned
0 and r6 refused with exit 1, identifying the undeclared `glfw3.dll` import in
`pixie.dll`. Healthy r6 identity passed with empty PATH. The result covers the
exporter's operation after the OS starts it. The E1 contract permits pre-main
imports. Healthy r5 evidence stays bound to r5.

The coordinator separately accepted formal r6 composition and healthy exporter
identity with empty PATH. The rehashed 729-payload SDK preserves all 718 base
payloads and ten unchanged tool files. The exporter executable is the sole
payload change from r5.

The coordinator checked the 19-member formal r6 loader seal before and after
replaying all five cases in fresh scratch. Healthy identity loaded staged
GLFW with both empty and ambient PATH. Both malformed ambient cases mapped
the fallback DLL and refused the exporter operation. The coherent malformed
case with empty PATH failed before main with `0xc0000135`. The focused local
operation refusal passed under E1's allowance for pre-main imports.

The coordinator separately accepted the healthy formal r6 code and no-code
export and verify replay. Four checks passed with the exact catalog and 15
code and 16 no-code output files. Input hashes matched after the run. Only the
code case used an explicit absolute CMake path and normal Windows PATH.

The same 753-member supplement also supplied focused two-SDK Editor evidence.
A fresh copy of the exact M3-matched Editor closure opened four valid local
SDK and project combinations, including a renamed Unicode SDK path. Both
crossed requirements refused with the specific `sdkId` diagnostic. Periodic
module polling observed no `pixie.dll` load. `OpenProject` validates portable
requirements before its native load path. Authored inputs were preserved.
SDK B is the existing C2/C3 comparison.

A separate startup-to-exit loader replay verified all 28 seal members and ran
four cases with the exact 21-file Windows x64 Editor closure and 708- and
696-payload SDK trees. Both crossed requirements exited 1 with the expected
`sdkId` diagnostic and zero `pixie.dll` load events. Each matched control
loaded its selected runtime. There were zero unresolved load events and the
authored bytes matched after each run. This covers the source-readable no-code
comparison.

The coordinator later accepted the freshly attested C2 authoring Release
SDK B as a producer input for selection, mismatch and no-code comparison.
All 5,083 source files, 675 SDK payloads and 676 ZIP members matched the
reviewed receipts. A fresh relocated no-code player run loaded the expected
distinct SDK and staged GLFW, exited 0 and preserved authored bytes. G1
on B's used profile and G4 remain open. Code support stays on SDK A.

The coordinator separately accepted direct C ABI module admission on the exact
local runtime. All 12 seal members matched before and after. The matched C3
`CodeTest` module loaded and reset successfully. The OS mapped the crossed C1
Boids module, and the C ABI returned argument error 1 with the specific `sdkId`
mismatch. The rejected module was no longer resident afterward. Paths, hashes,
23 typed runtime fields and zero unresolved loader events were verified.
This direct test covers the C ABI and OS mapping. Factory and initializer
instrumentation and session behavior retain separate evidence requirements.

The coordinator separately accepted C3-r3's instrumented native loader probe
on the exact C3 public authoring Release SDK. All 30 seal members matched.
A fresh scratch process recorded 17 ordered events with zero failures.
Matching selection reached the API, SDK and candidate-contract exports
before explicit session creation invoked the factory. The probe also covered
missing factory, a refusing initializer and detach on clear selection.
This is local source-readable native probe evidence. Full G2 remains open.

The coordinator also accepted the separate installed C3 successful session
lease probe. The fresh 25-member replay matched 19 ordered events with zero
failures. Clearing module selection left the live session able to step and
render. Destroying the session released the World callback capture and
active Project before DLL detach and static destruction. The producer
checkout was readable. Full G2 remains open.

The coordinator separately accepted C3 installed managed interop. A fresh
offline .NET 10.0.403 build in the standalone consumer folder imported the
shipped Release bindings and default header. The same assembly queried
public Release, public Debug and private Debug with ABI 28, identity schema 1,
26 typed profile fields and capability bits 5, 5 and 7. The actual runtime
and GLFW paths and hashes and all 696, 688 and 707 SDK payloads matched.
The producer checkout was readable. Full G2 and source isolation remain open.

The coordinator separately accepted the exact public Editor `--game` module
admission replay. All 42 seal files matched before and after. The matched
module rendered and exited 0. Both crossed cases mapped the exact module and
refused with the `sdkId` diagnostic and exit 1, including with the producer's
`pixie.dll` beside the module. The startup-to-exit trace saw only the selected
SDK runtime and zero unresolved load events. The crossed cases produced no
frames and all authored inputs were preserved. This source-readable replay
covers the explicit Editor module route. Factory and initializer behavior,
D09, source isolation, full G2 through G4 and visual acceptance remain open.

The coordinator replayed M2's interim managed test binaries from fresh copies
and isolated temporary paths. Debug and Release each passed 101 synthetic
checks. The receipt pins the test report and binary digests. A separate sealed
M2 and M2a source review then matched all 86 members and seven live source
postimages. Fresh Debug and Release copies each passed 101 checks, and the
public compile closure was checked.

The coordinator accepted M2's focused native positive replay from the 799-file
seal. Fresh copied inputs exercised installed Add Code, a changed module and
real parent/child dirty v2 reopen on the same SDK. The formal r5 Editor code
export used automatic Visual Studio CMake discovery with CMake absent from
PATH and the parent override unset. These were source-built local Editor runs
using development scripted entry points. The MSBuild `MSB8029` temporary cache
warning remains.

The separate independent M2 failure recovery replay rejected a real module
compile error, preserved saved authored bytes and the dirty v2 handoff, and
restored that handoff on same-SDK retry. The harness checked the failed child
image before closing its own BuildScreen process. This covers build/reopen
recovery. B6 committed Inspector recovery remains separate. Full G3, G4 and
D09 stay open.

E2 and E2a passed independent source and managed review. All 148 seal members
matched, and the Debug and Release suites passed 82 checks each. The actual
formal r3 replay passed through the source-built Editor's scripted trigger and
shared export job at a 101-character project root. It used an empty PATH with
the SDK override unset, preserved project inputs, and verified all 16 export
files. Public File menu interaction and broader G4 remain separate.

The coordinator independently validated the E1 root registration with the
extracted Windows CMake harness. Debug and Release resolve separate work paths,
the omitted exporter target leaves the test unregistered, and Release passed
all eight producer CLI refusal cases.

The coordinator accepted B10's focused local runtime staging and package
inventory controls. The 21-member seal and 14 evidence references matched.
Both positive controls and all six expected refusals passed, including an
inventoried private header and a wrong Debug runtime. The evidence index's
original TASKLIST digest is historical and its bytes were not retained. The
coordinator verified a two-member supplement that retains a later snapshot
and records four external references. Current ownership follows the live
tasklist and latest exact path ledger.
The D12 policy baseline is accepted. Executable policy, G6 and release
candidate acceptance remain open.

The coordinator separately accepted B10's first manual local rehearsal of
the formal r6 Engine SDK archive. An independent repeat produced identical
ZIP bytes and verified all 730 members. The packaged verifier and paired
exporter identity passed. Missing notice, mixed shipping runtime and
inventoried private header controls each refused. This qualifies the local
SDK archive rehearsal. The independent product pipelines and G6 remain open.

#### Scoped G0 amendment — B0-RC1

RC1 adds one exact K03/B10 path, `scripts/release/rehearse_sdk.py`, from
the reviewed R2 draft after coordinator relay. The path and its parent
were absent at clearance. The manual local command verifies a declared
SDK and paired exporter and writes the archive and receipt to a fresh
local output root. The R2 draft passed a Python 3.12 grammar check and
the local runtime replay used Python 3.15.1. Verify the installed source
hash and replay the exact r6 inputs. CI workflows, release policy files,
version changes, publication, retention deletion and G6 remain separate.

The coordinator accepted RC1's exact tracked-path replay. All ten seal
members and the sole live script matched. The command reproduced the
730-member r6 archive byte-for-byte and refused existing output and a
wrong catalog with the expected preservation. Runtime evidence is for
local Python 3.15.1. CI and full G6 remain open.

### B0 — Contract readiness and coordinated extraction

- `PIXIE-TASK-242` — **Status:** in-progress — Reconcile actual HEAD and dirty work with completed 018; prepare D01-D12/D14 outcomes for owner decisions at their deadlines, select optional blocks, freeze C01-C12 interfaces/product profiles and exact unit/path/artifact ledgers for dispatch. Pass G0 per affected unit; reject duplicate ownership or an unresolved prerequisite. Preserve unrelated player/export/proposal work. The exact clearances above are scoped readiness results; later decisions, interfaces and whole-unit G0 remain incomplete.
- `PIXIE-TASK-254` — **Status:** open — After affected pre-move gates pass, freeze B4/B5/B6/B11 writers and execute the single B0 source-removal/destination-addition transaction under D14. Transfer source, test, documentation, root/CI registration and binding ownership coherently; original block owners retain semantic responsibility. Close the move ledger without unrelated cleanup, history rewriting or writes to old paths during the move.

The coordinator accepted the provisional D14 plan and verified all 89
candidate source entries, Git references and proposed destination boundaries.
Refresh the exact move ledger after the writer freeze and obtain separate
execution approval.

### K01 — Native SDK and consumer support, one agent

- `PIXIE-TASK-243` — **Status:** in-progress — B4: define and implement selected compatibility identity and low-level interop delivery, keeping Editor session/history policy in B5. Preserve ABI28/contract1 candidate checks, token/receipt layouts, module leases and teardown. G2 rejects incompatible ABI/config/profile and unsupported authored types without treating C ABI equality as C++ compatibility or promising rollback of external effects. C1 native submilestone is independently validated; remaining managed delivery and applicable G2 scope are not complete.
- `PIXIE-TASK-244` — **Status:** in-progress — B1: produce the selected relocatable SDK/profile manifest, public targets/headers, player/module inputs, compiled math, dependency closure, CPU/toolchain/CRT identity, notices and provenance. Pass G1 with B9 through an external consumer and unavailable producer checkout, including World/Scene/presentation headers and missing-dependency negatives; package mode neither rebuilds Engine nor silently falls back to its sources. C2 package implementation is underway; C1 producer evidence is not installed-SDK qualification.
- `PIXIE-TASK-245` — **Status:** in-progress — B2: consume the qualified SDK and frozen B4 contract for game/player support, CMake helpers, templates, registry markers/schema and bootstrap. G2 covers new, recognized legacy, registered and unfamiliar custom projects, code/no-code inputs and mismatch/refusal cases; preserve edited user code/CMake, hashes/preimages, explicit contract1 admission and v1 scene-conversion safeguards. C3 source/synthetic preserving-generation and build-role work is active; actual installed consumption and full G2 retain their artifact prerequisites.

### K02 — Export and Editor authoring, one agent

- `PIXIE-TASK-246` — **Status:** in-progress — B3: deliver the native SDK-aware exporter, accepted CLI/product policy, selected-runtime/descriptor/manifest identity and nonadjacent dependency resolution. Pass G3 with code/no-code runtime-only launch, asset/native/CRT closure, complete v2 scopes/presentation/seed/opaque data and unchanged v1 policy. Detect missing DLLs, mixed identities and development payload contamination without producer-machine paths. E1 source implementation is active; schema/parser evidence and root tool registration are not G3 acceptance.
- `PIXIE-TASK-247` — **Status:** in-progress — B5: make Editor an independent SDK consumer with explicit selection/discovery, portable project requirements, cache/profile continuity, selected Inspector adoption, authoring/export capabilities and owned native graphics assets. G4 exercises two SDKs, mismatch/relocation, create/open/Add code, full dirty v2 rebuild/reopen, exact targets, camera/Environment, Play/Stop, history and committed recovery. Resolve D09 with actual loaded paths/hashes and context create/render/dispose evidence; preserve 018 in admitted package/local Inspector modes and record visual regression evidence for changed surfaces. C2/M1 selection/UI/guard/metadata slices are underway; actual G4 and the full application scope remain incomplete.

### B6/B7/B8 — Standalone adapters and conditional dependencies

- `PIXIE-TASK-248` — **Status:** in-progress — B6: adapt native/editor development protocols and local fixtures to frozen ABI/host/artifact inputs. Pass G5 with cold external-code consumers, shared GUI/script/channel admission, stale-edit rejection and committed recovery, positive allowed operations and negative capability/artifact contamination cases. Package fixtures must not recreate an Engine ancestor to hide a source dependency; B5 retains export UI and application hooks. N1 native hello is a bounded active slice; Editor/client/cold-code and whole G5 acceptance remain separate.
- `PIXIE-TASK-249` — **Status:** open — B7: qualify the D08-selected immutable local Inspector core/view package pair and extra feature baseline after whole-block assignment and exact clearance. Supply package-only G4 evidence without requiring sibling Inspector sources or remote publication. B5 owns Editor feature conditions, icons, semantic mapping and grouped-history adoption. Broader core/view behavior changes need a path or contract amendment.
- `PIXIE-TASK-250` — **Status:** dropped — **Reason:** B8 is inactive because the owner accepted D04 on 2026-11-04: retain the unchanged contained native Lib primitives/math closure without an independent native Lib package, upgrade or events ABI commitment. B1 still owns actual contained dependency closure/provenance and G1 evidence; no other unit acquires PixieLib write ownership. Reactivation requires an accepted scope/ownership amendment.

#### Scoped preparation — B0-P1

The sixth dedicated chat owns whole B7. After coordinator relay, it may
archive the exact committed Inspector source at
`c4dccc9c2bb7df7619f09e6b6da6cd7c5750d8b5` and build the locked local
core/ImGui pair in `.local/separation-060/B7/`. Package production,
restore and qualification outputs use that scratch root. Peer source
access is read-only. B5 receives the sealed pair and needs its own exact
scope for Editor adoption. Durable Inspector writes require separate review.

The coordinator accepted P1-r4's exact local core/ImGui package pair
`0.3.0-pixie021.3` as B5 input. All 14 handoff members and five package
archives and lock entries matched. A fresh relocated offline locked restore
and build passed 22 headless ImGui assertions with all four loaded identities
verified. Missing and substituted core controls refused with NU1101 and
NU1403. Use the P1-r4 handoff for M6. Actual Editor adoption, source
isolation, B7 completion and G4 remain open.

### K03 — External evidence and release preparation, one agent

- `PIXIE-TASK-251` — **Status:** in-progress — B9: prepare isolated external consumer fixtures and acceptance inputs for G1-G5, using exact SDK/profile/toolchain/package identities and preserving affected 018 regressions. Verify available upstream artifacts incrementally; source paths must be unavailable or covered by an explicitly accepted access-verification method. Read-only live checkouts do not prove independence; never rename/delete the owner's repositories to simulate absence. Q1 structural implementation is independently confirmed above; product consumers, real G1-G5 evidence and the whole unit remain incomplete.
- `PIXIE-TASK-252` — **Status:** open — B10: after D12, prepare independent Engine SDK/runtime and Editor CI/release pipelines over immutable inputs, B9 fixtures, dependency locks, provenance, capability exclusions and retention policy. Keep B7/B8 peer workflow ownership and independent product versions; prepare local candidate assembly and verification without creating remote repositories, pushing, tagging or publishing.
- `PIXIE-TASK-255` — **Status:** open — B9: after the B0 move, run the coordinated external-consumer integration against the moved artifacts, record exact identities and affected G1-G5 results, and verify no source fallback or accepted-feature loss. Reconcile focused evidence and rerun only checks affected by changed inputs; builds alone do not establish independence or runtime/visual acceptance.
- `PIXIE-TASK-256` — **Status:** open — B10 with B9 verification: assemble exact SDK/runtime/Editor release candidates and pass G6 using completed B11 documentation and the accepted D12 policy. Verify payloads, native assets, locks/provenance/notices, public/local exclusions and mismatch/contamination negatives; record rehearsal results and remaining limits. No upload, tag, push, release or future VAW validation is required.

### B11 — Product documentation without a VAW prerequisite

- `PIXIE-TASK-253` — **Status:** open — B11: map each fact/contract to one authoritative project, canonical location, stable identity, supported cross-project reference and validator; deliver minimum SDK/Editor guides and reconcile current references after the move, including the consumed API27 overview to implemented ABI28 baseline. Route peer-document edits through B7/B8 or their repository owner, preserve archive bodies and complete C11/G6 under current rules with bounded documented validator limits. No competing workplace/schema framework, broad API formalization or VAW installation/rebind work.

#### Scoped G0 clearance — B0-DOC1

DOC1 clears B11 to modify only `docs/development.md` and `editor/README.md`
through the sealed R2 operator correction after coordinator relay. It
documents N6's explicit `--inputs` and `--work` procedure, including
preflight, inventory keys and cold-suite omission. B6 completed the
technical review. The B11 owner verifies both preimages and the driver
before applying the patch, then submits both postimages and focused
documentation checks for independent coordinator review.

The whole B11 assignment spans pre-move documentation and post-move
reconciliation. DOC1 is the first two-file clearance. Future guides and
reference edits require exact receipts for their paths. Historical
qualification records retain the original commands. D08, D12, D14 and the
extraction transaction retain their respective authority and evidence boundaries.

The coordinator accepted DOC1 after rechecking all 15 seal members, replaying
the exact patch from both preimages and matching both live postimages.
The fresh pinned N6 CLI preflight verified 707 SDK payloads and reported
cold code omitted. Repository-wide diagnostics matched the prior report.
The coordinator then verified and relayed the separate N8 one-file scope.

#### Scoped G0 clearance — B0-DOC2

DOC2 clears B11 to change only the active `pixie_api_version` overview in
`docs/modules/capi/REFERENCE.md` from 27 to 28 after coordinator relay.
The Engine header and implementation establish the current ABI value.
Preserve the separate historical ABI27 and ABI28 sections, links, anchors
and document ID. B11's Editor guide and MCP reference relocation proposals
depend on their own exact path and artifact reviews.

The coordinator verified the receipt and relayed the single-line scope to B11.
Independent review matched all six seal members and the live one-byte
postimage. Reverse apply and scoped whitespace passed. The remaining guides
and post-move work are open under B11 and G6.

#### Scoped G0 amendment — B0-DOC3

DOC3 extends B11's `docs/development.md` scope only. Apply the exact
reviewed SDK guide patch after coordinator relay. The insertion covers
installed SDK preflight, focused X1 and the accepted local RC1 command.
It preserves the existing document bytes outside the insertion and keeps
source isolation, release and G6 evidence distinct. The field-interaction
guide and move-dependent references retain their own reviews.

The coordinator accepted the exact DOC3 postimage after matching all
15 seal members and the single 2,703-byte insertion. DOC1 bytes were
preserved. Reverse apply and scoped whitespace passed.

#### Scoped G0 clearance — B0-DOC4

DOC4 clears only `docs/editor-field-interactions.md` for the exact
reviewed B11 patch after coordinator relay. Document the locked selected
Inspector pair, the local project route and the accepted installed M6a
fixture invocation with its optional hashed module. Preserve prior
anchors, the document ID and unrelated behavior descriptions. Source
isolation, human visual review and other guides retain their own evidence
and scope.

The coordinator accepted DOC4 after verifying all 11 seal members, the
exact live postimage, reverse apply and scoped whitespace.

### Milestones and closure

M0 is B0/G0 readiness for the selected dispatch. M1 establishes K01/G1-G2
with the accepted contained native Lib closure; assigned B7/B11 may prepare
independently. M2 establishes K02/B6 and G3-G5 for accepted capabilities,
including public code authoring/native export under D10; actual invocation and
artifact evidence remain required. M3 is the coordinated B0 extraction. M4 is
K03 post-move consumer verification and B11 reference/guide reconciliation.
M5 is exact-candidate G6 rehearsal. These are readiness milestones, not agent
assignment batches; a unit retains one owner while spanning several milestones.

Close the parent only when every child task is done or explicitly dropped with
a reason and the accepted G0-G6 outcomes are evidenced for the same immutable
product tuples. Conditional omissions do not waive an accepted contract. Keep
actual publication separately authorized, retain other TODO owners' scopes and
state, and complete separation without waiting for the later VAW adoption.
