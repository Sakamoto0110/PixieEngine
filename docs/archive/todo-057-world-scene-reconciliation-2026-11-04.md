# World and Scene Reconciliation Completed: PIXIE-TODO-057

**Document ID:** ARCHIVE-TODO-057-WORLD-SCENE-RECONCILIATION-20261104

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** Complete World/Scene implementation and qualified global-body fallback

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-091

**Type:** completion

**Origin:** PIXIE-TODO-057 (PIXIE-TASK-234 through PIXIE-TASK-241)

**Author:** codex

**Reference date:** 2026-11-04

**Commits:** cf86a50fb27226e1218eef37c2d9d0cdd8568d21, f3504ca6292ec9d2843de40b15cbd75c10eaaa59, 15ab5f85762d85a8af50eb11ee60e6fa491e0fa5, cc8dc463b39fcfe6fc8bfbfeea5f129b9e7d6f40; documentation with this record (parent cc8dc463b39fcfe6fc8bfbfeea5f129b9e7d6f40)

**Delivery authorization:** On 2026-11-04 the owner authorized scoped commits, push, remote CI and VAW documentation. The code commits above deliver the implementation; this record closes the local scheduler. Remote CI is recorded separately after publication.

## Completed technical boundary

Product candidate 0.3.0.0, native/editor ABI 28, scene envelope v2, presentation
schema1 and cooperative module contract1. World remains the execution context;
logical Scenes share registry/clock/solver and preserve World-only membership.
The delivery includes transactional persisted scope authoring, complete candidate
reconstruction/history/recovery, separate presentation authorities, explicit safe
viewport camera/Environment contexts and qualified local motor/pool gravity.

The extended body-force experiment failed in Debug and Release:204 sleep mismatch
steps while the Scene override remained fixed, despite tiny position drift. The
owner-approved failure fallback retains global body gravity and logical Scenes.
No solver change, local-body activation or weakened tolerance is included.

## Verification

Windows11/AMD Ryzen7 5700X,32GiB RAM, AMD Radeon RX7600/OpenGL4.5, MSVC19.51.
Dirty baseline ae598b612583f299271ec83f1cf089ce1aa97232; exact local source/binary
hashes accompany the qualification evidence. VAW bb1c8b39665688f7f95f241ff472e5788b7e2ba5,
pixieengine/3, all authority routes checked through the explicit Pixie registry.

| Layer | Executed result |
|---|---|
|Native VS2026 and CI-local, Debug/Release|31 CTests each,124 total; runtime309 and lighting42 per configuration.|
|Managed published/local Inspector, Debug/Release|50 commands; candidate51,Scene63,module67,repair8,presentation27,input16 in all four modes.|
|Real module/editor rebuild|14 checks each in published Debug/Release, including dirty canonical/scoped data and unchanged file.|
|MCP/development channel|191 cases per configuration, including cold C++ code.|
|Actual LightMachine consumer|42 headless checks each Debug/Release, legacy/canonical Build and F/P behavior.|
|Authoring timing/memory|16 fresh processes,64 edits,0/100/1000/10000 entities with shape/physics corpora.|
|Production gravity step costs|72 fresh Worlds,8640 measured steps; matching paired state, raw distributions retained.|
|Documentation|182 exactly inherited owner-deferred diagnostics; no additions; whitespace passes.|

At10000 entities the first Release Create took753.44ms(shape) or1961.61ms(physics),
with process working-set peaks387.5/716.3MiB. Debug took11.43/29.66seconds,
peaking412.8/806.4MiB. Peaks include startup/graphics/fixtures, old/candidate Worlds,
GC and retained history; they are not isolated candidate/GPU allocation. No FPS,
responsiveness budget, percentile claim or controlled A/B improvement is inferred.

Serial World::Step measurements exclude setup/warmup/validation/output. At1000
motors, Release median trial mean was12031.4us World-only and12320.8us Scene-Inherit;
at100 pools of64particles it was75.2/81.9us. Three paired trials/120measured ticks
each retain timing noise and negative deltas; no approved numeric budget exists.

Initial native timestamp integration errors, strict color/schema regressions,
seed/malformed-JSON failures and their red/green evidence are retained. The first
final matrix was deliberately stopped for product defects before source changes;
the corrected native matrix passed. Two old managed fixtures then required existing
coordinator/full-document-history expectations; their exact failures and targeted
greens are retained. Those were test-only amendments; native/product bytes stayed
unchanged after the completed native matrix.278 final scoped files matched.

Guard mutations detected the specific failures; Windows CLR assertion exits did
not match one driver's exit1 oracle. That driver failure remains recorded, not
relabeled as a successful command. Exact byte restoration and post-restoration
greens establish the sensitivity evidence.

## Publication preparation

On 2026-11-04, the committed delivery tree passed the documentation checker:
189 documents, zero problems. The 182 inherited diagnostics remain in excluded
local owner work and nested worktrees; they are not shipped or silently repaired.
The four code commits separate runtime, ABI/editor integration, the LightMachine
consumer and regression evidence. Native and managed interop build together.

## Limits and independent acceptance

Candidate publication protects session-owned storage under the cooperative module
contract, not arbitrary external native effects or a sandbox. Unsupported custom
authored types reject before mutation; unknown unregistered payload stays opaque.
No conflicting-scope remap action is exposed. AO/normal values remain one read-only
compatibility authority. Existing v1 behavior and explicit backed-up conversion stay.

The owner's earlier question46 accepted the original Scene presentation. Expanded
controls and the recovery panel are assumed by the completion instruction and remain
unreviewed by the owner. They are not represented as approved reference images;
TODO057 has no additional blanket manual visual-approval completion gate.
The task229 handoff does not accept the separate historical backend experiment.

No loading threads, VM qualification, new independent solver/streaming architecture,
multiInspector, push, merge or remote CI is included. These retain their existing
boundaries. Publication is a separate action and has not occurred.

## TODO snapshot

### `PIXIE-TODO-057` — World and Scene reconciliation

**Status:** completed
**Source:** owner promotion and activation of [PIXIE-PROPOSAL-018](../proposals/world-scene-reconciliation.md) as the highest priority on 2026-11-02.
**Accepted scope:** execute the incremental World and Scene plan in proposal 018: typed World authority, logical Scene scopes and membership, complete persistence and editor reconstruction, qualified local gravity, and explicit camera and environment selection. Preserve World-only entities, existing v1 behavior, and the independent input work described there.
**Dependencies and gates:** follow the proposal's slice prerequisites and evidence requirements. Resolve its listed owner decisions before migrating settings groups, enabling a new persisted format, exposing local gravity, or extending viewport policy. Coordinate with PIXIE-TODO-056 at task 229; do not treat that coordination as backend acceptance.
**Completion criteria:** each applicable slice meets the evidence and regression gates in proposal 018, with player and editor qualification and documented disposition of deferred choices. Existing scenes and unsaved authored state survive the relevant transitions.
**Breakdown:** [Task snapshot](#task-snapshot)

## Task snapshot

## `PIXIE-TODO-057` — World and Scene reconciliation

**Parent:** [TODO snapshot](#todo-snapshot)
**Priority:** highest (owner decision, 2026-11-02)
**Scope:** implement and qualify the incremental plan in [PIXIE-PROPOSAL-018](../proposals/world-scene-reconciliation.md). Each task retains the proposal's prerequisites, preservation rules, and owner decision gates.

**Execution cadence (owner decision, 2026-11-03):** Assign one writer to each
related set of code and interfaces. Other participants may investigate and review
in parallel; transfer write ownership explicitly before editing that set. Implement
in bounded, related blocks. Group small reviews; a large review may form its own
block. Continue autonomously under existing authority. For the current run, after
approximately 30 minutes, finish the current unit, persist and validate a VAW
planned-pause checkpoint, report the result and wait for the owner to resume.
After the run 03 pause and context compaction, the owner authorized run 04 with
the same writer/reviewer separation and a turn limit of approximately two hours.

**2026-11-03 bounded follow-up:** Two read-only reviewers investigated persistence
and viewport continuity; root was the only writer. The private code-reopen
handoff now retains viewport Environment choice (including a missing Scene ID),
validates it before scene mutation, and accepts older handoffs without the field.
Opening a different project resets the viewport choice only after successful
publication; reopening the same directory or a rejected open preserves it.
Both regressions failed with their respective fixes removed and passed after
source restoration. The VS and CI presets each passed all 28 CTests in Debug
and Release; managed code-component checks passed 30 checks per configuration.
Specific MCP qualification and visual acceptance remain pending. This does not
close tasks 237, 238 or 240 or enable durable scope authoring. Current validation
records and the planned-pause checkpoint are under `.local/vaw/records/`.

**2026-11-03 run 02:** The run 01 checkpoint validated before resumption. Root
was the only writer; two reviewers remained read-only. ABI 23 adds shared native
structural import/replacement preflight, called before editor destruction and
binding notifications, including the legacy handoff. Replacement owners are
excluded logically; copies retain originals for singleton conflicts. A valid
existing index preserves normal fragment-import cost. New native and managed
regressions cover rejected replacements, singleton/default-unit handling and v1
compatibility; negative mutations detected removed guards. VS and CI each passed
28/28 CTests in Debug and Release. Published Inspector 0.3.0 editor builds and
managed fixtures passed 36 checks per configuration. Full MCP qualification,
including cold code projects, passed 189 cases per configuration in package mode.
It exposed stale API20 constants and an obsolete duplicate-settings expectation;
the MCP bridge/test version now derives from the CAPI header, and duplicate refusal
is propagated explicitly. Failed attempts and corrections are retained in local
validation records. Structural preflight does not make destruction, binding
subscribers or component callbacks transactional; durable scope authoring remains
closed. The next architectural alternatives are documented locally for owner
direction. This run does not close TODO-057 or its remaining visual/lifecycle gates.

**2026-11-03 run 04:** After checkpoint validation and context compaction, the
owner authorized a turn bounded to approximately two hours, with root as the
sole writer and read-only reviewers. A private shared save guard now refuses a
recovered v2 snapshot over an existing v1 file without explicit backed-up
conversion, preserving bytes and dirty state in Edit and Play. New settings
field admission refuses duplicate authorities and invalid prospective physics
values before writing, including post-step unit changes. Legacy v1 reads stay
compatible. Environment selection fits a320px viewport. Session now provides
World properties navigation into a distinct target in the existing Inspector,
binding unique v1 Physics/Environment owners without installing capabilities or
introducing a second editable authority. Missing/duplicate/generated states are
explicit; World selection survives history, Play/Stop and private code handoff.
Generation-checked handles refuse stale reads and writes. The owner accepted
the Session-to-Inspector organization; multiple Inspector panels remain outside
this slice. The owner also approved the Edit-only authored-candidate direction;
its lifecycle/admission/publication contract still needs implementation and
qualification before dedicated Scene-authoring controls are exposed.

**2026-11-03 run 06:** The owner authorized serial candidate implementation and
timing/peak-memory qualification across World sizes, with one writer and parallel
read-only reviewers; multithreading remains deferred. ABI 24 adds immutable
Engine-fallback factory admission, owner-thread and expected-World-revision checks,
candidate-only movement validation and preparation/publication timing receipts.
The internal managed coordinator reserves history before commit, retires old
bindings, restricts history replay to its admitted path and retains committed
history through explicit rebind recovery. Seven guard mutations were detected
and restored. Native VS2026/CI local Debug/Release passed 28/28 each; published
and local Inspector configurations passed 30 candidate checks each, and full MCP
passed 191 per configuration including cold code. Synthetic 0/100/1000/10000-entity
shape/physics workloads were measured in fresh Debug/Release processes. At 10000
entities, first Release edits measured 731 ms/1780 ms with observed resident working-set peaks
292 MiB/589 MiB; peaks include startup, fixtures, both Worlds and retained history,
not isolated candidate memory. No performance threshold or full-frame budget is
claimed. GUI/MCP-wide gesture/recovery admission and dedicated Scene controls
remain gated; arbitrary code-project lifetimes, local body gravity and VM work
remain unqualified. No commit, push or remote CI was performed.

**2026-11-03 run 07:** The owner authorized serial loading preparation and duplicate
serialization investigation, retaining one writer with parallel read-only reviewers.
ABI 25 adds session/storage/authored tokens; supported mutation attempts invalidate
prepared data before mutating callbacks. The internal managed boundary separates
immutable preparation from publication, rechecks history/content identity and uses
shared GUI/script/MCP admission and committed binding recovery. Fitting snapshots
now serialize once; bounded growth retries once, with retained buffer capacity
measured. v2 history avoids another managed JSON write round trip. Four guard
mutations were detected and restored. Native VS/CI local Debug/Release passed
28/28 each; candidate checks passed 49 each in published/local Inspector modes,
and MCP passed 191 each including cold code. Across 64 samples in 16 processes,
10000-entity first Release edits measured 362 ms without bodies and 1221 ms with
physics (run06:731/1780 ms). Resident process peaks were 300/530 MiB (run06:292/589);
memory did not uniformly decrease. Growth retries produced warm spikes; these
successive observations are not percentile/FPS or full-frame guarantees. No
loading worker or Scene action was enabled. GUI recovery click/close execution
remains unqualified, while real pipe recovery passed. Documentation retains the
182 inherited diagnostics. No commit, push or remote CI was performed.

**2026-11-03 run 08:** The owner authorized serial Scene authoring with one writer
and read-only reviewers. ABI26 adds explicit current-World v1 conversion with
recoverable original backup and synchronized cached text/epoch, plus cheap format
and membership queries. Exact Engine fallback projects expose Scene create/select/
rename/remove-empty and entity membership, using candidate reconstruction, queued
gesture freshness, stable selection/bindings and mixed Undo/Redo. Occupied removal
refuses. Eight targeted guard failures were detected and restored; actual ImGui
input and real hidden-editor injected recovery timer tests passed. The owner
accepted the Scene section, wide/320px Inspector and membership presentation in
question46; separate recovery-panel owner acceptance is not claimed. Native VS/CI
local Debug/Release passed28/28 each; managed authoring55 and candidate49 passed
in all four published/local configurations; full MCP passed191 each including
cold code. Across64 full-controller samples in16 fresh processes, first Release
edits with10000 entities took732 ms without bodies and2029 ms with physics; process
resident peaks were419/707 MiB. Debug first edits took9.74/26.45 seconds. Peaks
include startup, GL, fixtures, old/candidate Worlds, GC and retained history; this
boundary includes proposal construction and is not a direct A/B against run07.
224 frozen implementation files matched after qualification. Documentation retains
182 inherited diagnostics. Broader code-project admission and remaining018 gates
stay open. No threads, local body gravity, VM, commit, push or remote CI was added.
Evidence: .local/validation/todo057-run08/REPORT.md and VAW validation records.
**2026-11-04 run 09:** The owner authorized real C++ candidate admission, the complete
editor lifecycle and controlled failures. ABI27 introduces explicit contract1 module
opt-in, a pinned factory and fresh matching Project/types/World reconstruction with
schema and format checks before publication. V2 output rejects unsupported authored
components and settings conflicts. Restricted AddData supports trivial scalar data;
uncontracted authored types remain refused. V2 Edit component-add status and mutation
share admission before callbacks/construction/epoch. Latest focused Debug passed295
runtime tests,58 module checks with published Inspector0.3.0,49 candidate checks,55
Scene checks,8 repaired-load checks and11 real process-rebuild checks. Compiler
qualification passed one positive and12 rejected cases. Targeted red/green evidence,
diagnosed harness failures and exact restoration are retained.238 implementation
files matched after focused qualification. Full VS/CI local Debug/Release, managed
matrix, MCP and module World-size measurements remain pending at the planned pause.
No parent/task closure, threads, VM, local body gravity, commit or push was added.
In the task entries below, retaining a published World means retaining its
session-owned storage and revision. Arbitrary callbacks can still mutate retained
references or external state. Dedicated Scene controls admit qualified exact Engine fallback projects and explicit
cooperative contract1 modules. Editor v2 fragment edits use candidate publication;
legacy raw imports retain their documented limits. No production local-body gravity
activation or VM qualification was added.

**2026-11-04 run11 completion candidate:** The full proposal018 implementation and
local automatic qualification are complete at product candidate0.3.0.0 and ABI28.
Body-force qualification failed its sleep criterion; the already-approved global-body
fallback applies, with local motors/pools and logical Scenes retained. New visual
surfaces are assumed by the completion instruction and remain unreviewed by the
owner; this is not an additional manual approval gate for TODO057. The explicit
no-commit restriction prevents the commit-backed archive transition required by
the documentation policy. Task241 tracks only that formal delivery step, not more
implementation. No commit, push, merge, VM or loading worker was performed.

- `PIXIE-TASK-234` — **Status:** done — Baseline records Engine `ae598b6`, product 0.2.0.1, native/editor ABI 20 and the Inspector package/local dependency modes. Copies of LightMachine, LightLab, PhysicsLab and CouplingLab initially matched their original v1 scene SHA-256 hashes. On the local AMD Radeon RX 7600 (OpenGL 4.5), Debug and Release players opened every copy and captured the same step-60 image hash per project; Release also captured step 0. CouplingLab emitted a safe-placement warning at step 60 in both configurations. The Release editor opened and captured all four copies. PhysicsLab retained selection across Play/Stop, saved as v1 and reopened with the entity selected; saving reformatted only that copy. An isolated Release editor build with `PixieUseLocalConfiguration=false` used published PixieInspector.ImGui 0.3.0, compiled without warnings and opened PhysicsLab with selection preserved across Play/Stop. Every original scene stayed untouched. Local captures are under `pixie-todo-057-qualification-20261102-173516` in the temporary directory. This baseline does not qualify v2 authoring or the complete Debug editor workflow; those remain later gates.
- `PIXIE-TASK-235` — **Status:** done — Typed legacy lighting preserves absent, disabled and configured states; explicit canonical presentation separates Environment, view and Renderer with one writer and rejects mixed authorities. Scene overrides affect only explicitly selected viewport Environment; legacy AO/normal data stays read-only compatibility storage. The actual LightMachine consumer passed42 headless checks in both configurations, including Build without implicit legacy installation and legacy/canonical F/P gameplay. Lighting42, presentation27 and real-input16 checks passed; source/render evidence and unreviewed appearance remain distinct.
- `PIXIE-TASK-236` — **Status:** done — Added stable logical Scene scopes, explicit World-only membership, deferred transfers, and occupied-scope removal rules in memory. Focused runtime tests cover identity, motion, and contact continuity; 255 runtime tests pass in Debug and Release. No new persisted writes were enabled.
- `PIXIE-TASK-237` — **Status:** done — One v2 document preserves empty scopes, separate entity/scope identity domains, direct World membership, unknown data, seed and inactive values. v1 reading/writing remains compatible; conversion validates before replacement and preserves the original backup. Conflicting scope definitions reject atomically; no remap action is exposed. Supported entity copies remap internal references while preserving outside references and membership. ABI28 prepares full candidate proposals on unbuilt scratch storage, serializing only the changed entity set; raw live v2 restore refuses before destruction. Custom authored types require declarative admission; unknown unregistered data stays opaque. Candidate reconstruction protects published storage, not arbitrary external native effects. Native309 and CAPI/managed fragment/history regressions passed.
- `PIXIE-TASK-238` — **Status:** done — ABI28 aligns native exports, managed bindings and development tools. Exact Engine fallback and explicit cooperative contract1 C++ factories reconstruct fresh Project/descriptors/World bundles with pinned DLL lifetime, schema/format checks and safe teardown. Qualified declarative scalar data is supported; unrestricted callbacks/Behaviours are not silently admitted. Complete snapshots and stable IDs preserve dirty authored data, selection/World/Scene targets, defaults and camera/Environment intent across Play/Stop, save/reopen and real process rebuild. GUI/script/channel share admission, full-document v2 history and committed binding recovery. Candidate51, Scene63, module67 and repair8 checks passed in package/local Debug/Release, plus14 real rebuild checks per published configuration. No arbitrary external-effect rollback is promised.
- `PIXIE-TASK-239` — **Status:** done — Authored Inherit/Multiply/Override resolution, explicit zero and retained inactive values are implemented and persisted. Motors and dynamic particle pools consume the native per-owner result, apply scale once and preserve existing fallback/order/diagnostics; inherited particles use the effective backend base. Actual runtime comparisons cover1/60/600 steps, CCD, contacts, sleep and deferred membership transfer. The expanded changing-World-base force experiment failed in Debug and Release with204 sleep mismatch steps despite tiny position error; the owner-approved failure fallback therefore retains global body gravity and logical Scenes. No solver change or production body compensation was activated. See tests/qualification/local-gravity/RESULTS-Runtime-2026-11-04.md and expanded CSVs. Serial per-step motor/pool cost was measured separately; no numeric performance budget or equivalence for non-collinear interacting bodies is claimed.
- `PIXIE-TASK-240` — **Status:** done — Stable authored Camera2D references and Scene/World defaults resolve through explicit per-viewport context to the existing free camera. Rendering, coordinates, picking and gizmos share native resolution, with retained expired-target intent and diagnostics. Environment selection is independent of camera and Inspector selection; World fallback does not install/enable absent lighting. Canonical Scene Environment overrides and two actual same-World framebuffer contexts were qualified, including different dimensions, independent view settings and GL state restoration after success/failure. Code rebuild retains camera/free navigation and Environment choices. The current logical-scope model has definitive removal and no temporary unloading/streaming operation. Earlier q46 visuals are accepted; expanded presentation is assumed by the completion instruction, not owner-reviewed.
- `PIXIE-TASK-241` — **Status:** done — Implementation and automatic qualification are complete: native31 CTests in each VS2026/CI-local Debug/Release configuration;50 managed commands across published/local Inspector modes;191 full MCP cases per configuration including cold code; LightMachine42 checks per configuration. Sixteen fresh module processes measured64 edits across0/100/1000/10000 entities and shape/physics workloads, recording timing and process peak memory. Serial consumer-step measurements cover72 fresh Worlds/8640 measured steps. After the native matrix, product bytes were unchanged; two managed fixtures were updated for existing full-document v2 history and their prior failures retained. Documentation retains exactly182 owner-deferred inherited diagnostics, with no additions; whitespace passes. Task229 handoff is reconciled. The owner subsequently authorized commits and publication; the code hashes above and this archive/router/TODO transition complete formal local closure. No implementation or manual visual-approval gate remains. VM, loading workers, remote CI and publication are excluded/deferred under their existing boundaries, not new implementation requirements.
