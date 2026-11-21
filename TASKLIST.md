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

- `PIXIE-TASK-229` — **Status:** open — Agree and qualify a minimal editor/backend handoff and demonstration scene against the separately accepted World/Scene contract, preserving the distinct backend acceptance decision.

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

## `PIXIE-TODO-057` — World and Scene reconciliation

**Parent:** [PIXIE-TODO-057](TODO.md#pixie-todo-057--world-and-scene-reconciliation)
**Priority:** highest (owner decision, 2026-11-02)
**Scope:** implement and qualify the incremental plan in [PIXIE-PROPOSAL-018](docs/proposals/world-scene-reconciliation.md). Each task retains the proposal's prerequisites, preservation rules, and owner decision gates.

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
references or external state. The authoring gate admits dedicated Scene controls only for qualified exact Engine
fallback projects; existing low-level snapshot/import and already-v2 save APIs are not
blanket-disabled. No local body-gravity activation or VM qualification was added.

- `PIXIE-TASK-234` — **Status:** done — Baseline records Engine `ae598b6`, product 0.2.0.1, native/editor ABI 20 and the Inspector package/local dependency modes. Copies of LightMachine, LightLab, PhysicsLab and CouplingLab initially matched their original v1 scene SHA-256 hashes. On the local AMD Radeon RX 7600 (OpenGL 4.5), Debug and Release players opened every copy and captured the same step-60 image hash per project; Release also captured step 0. CouplingLab emitted a safe-placement warning at step 60 in both configurations. The Release editor opened and captured all four copies. PhysicsLab retained selection across Play/Stop, saved as v1 and reopened with the entity selected; saving reformatted only that copy. An isolated Release editor build with `PixieUseLocalConfiguration=false` used published PixieInspector.ImGui 0.3.0, compiled without warnings and opened PhysicsLab with selection preserved across Play/Stop. Every original scene stayed untouched. Local captures are under `pixie-todo-057-qualification-20261102-173516` in the temporary directory. This baseline does not qualify v2 authoring or the complete Debug editor workflow; those remain later gates.
- `PIXIE-TASK-235` — **Status:** in-progress — Typed lighting access now has a LightMachine consumer and distinguishes absent, disabled and configured states while keeping `LightingSettings2D` as the v1 edit authority. Legacy files without `enabled` retain their appearance; disabled settings retain authored values. Debug and Release builds of runtime tests, lighting tests and LightMachine passed (272 runtime and 34 lighting tests per configuration, zero failures). The automated unlit-image comparison passed. Player/editor visual qualification, Environment/view/Renderer ownership migration and old/new conflict rules remain open.
- `PIXIE-TASK-236` — **Status:** done — Added stable logical Scene scopes, explicit World-only membership, deferred transfers, and occupied-scope removal rules in memory. Focused runtime tests cover identity, motion, and contact continuity; 255 runtime tests pass in Debug and Release. No new persisted writes were enabled.
- `PIXIE-TASK-237` — **Status:** in-progress — The v2 snapshot envelope, scope-aware fragments, v1 preservation and explicit v1-to-v2 file conversion are implemented with focused runtime coverage. Conflicting imported scope definitions fail before mutation; remapping remains explicit and unimplemented. Registered project components without a qualified declarative contract fail v2 scene/fragment reads and conversion before any callback or World/index/RNG/file mutation; diagnostics list all unsupported types, while unknown payload stays opaque and v1 reads retain their behavior. Direct v2 whole-scene writes now reject such live components before serialization getters; fragment writes reject them when present in the selected set. Complete authored-scene restoration builds and validates a candidate World before publication; failed reads or builds retain the published World, revision, camera and mode. Debug and Release runtime tests passed (285) and lighting tests passed (40), including empty scopes, World defaults, unknown payload, stable IDs and a rejected custom component. The earlier negative external-hook test still limits the guarantee: live-World fragment import rolls back only added entities, scopes, memberships, RNG and scene metadata; an existing-component change and external effect persist. Project build hooks may also have external effects before candidate publication. Editor history retains an Undo/Redo step when its application throws; Debug and Release field-interaction executables passed, including four retry checks. The bounded serial fallback Scene controls now use the qualified candidate coordinator; contracted code-project reconstruction has focused Debug evidence; full qualification and remaining import/remapping decisions stay open.
- `PIXIE-TASK-238` — **Status:** in-progress — C API ABI 21 adds complete authored snapshot and candidate restore operations in Edit mode; managed restore is restricted to startup before editor binding subscribers. The code-rebuild handoff version 2 captures the full scene document, dirty state, ordered selection and anchor by Scene ID, plus pending attachment targets; legacy version 1 handoffs remain readable. Five C API tests and the managed code-component flow (generated consumer build, live-restore refusal and invalid handoff recovery) passed in Debug and Release. An additional managed fixture passed in both configurations: an unsaved v2 scene with an empty scope and edited `PhysicsSettings2D.gravity` retained its value, dirty state, ordered selection and anchor by Scene ID through the handoff, while the saved v1 scene file remained unchanged. A development-only process harness then launched two real editor processes in Debug and Release; seven checks per configuration confirmed v2 restore, empty scope, edited global gravity, dirty state, Scene ID selection/anchor, child timer exit and unchanged saved file on the local Windows/Radeon RX 7600 host. History and broader normal player/editor acceptance remain open. Custom components lacking declarative v2 admission reject v2 snapshots or restores. ABI26 adds explicit conversion/save, membership/format reads and serial fallback Scene controls; Scope inspection now survives history, Play/Stop and handoff. ABI27 adds explicit contract1 module admission with focused Debug evidence; uncontracted factories stay excluded and the full matrix remains pending.
- `PIXIE-TASK-239` — **Status:** in-progress — Native World/Scene gravity resolution supports authored Inherit, Multiply and Override modes, explicit zero, retained inactive values and v2 round-trip with unknown fields. Invalid modes, values, limits and conflicting imports fail before mutation. Runtime tests passed in Debug and Release. An isolated Box2D comparison harness passed 13 deterministic cases in Debug and Release, measuring 1, 60 and 600 steps, including zero/lateral gravity, mass and gravityScale changes, CCD, contacts, sleep, cross-Scene contact with different collinear gravities, sleeping-body transfer and a transfer request made inside pre-solve then applied after the step. The largest measured position error was 0.000001309 m; contact/event and velocity mismatches were zero. The [2026-11-03 qualification report](tests/qualification/local-gravity/RESULTS-2026-11-03.md) and dated CSVs preserve exact evidence. Non-collinear different-gravity contact remains observational without an equivalent one-World global-gravity reference; the deferred fixture does not call runtime `World::SetScope`, and motors, particles and actual project scenes remain unqualified. The owner approved the comparisons, not force compensation or local body-gravity activation; dynamic bodies, motors and particle pools still use legacy paths.
- `PIXIE-TASK-240` — **Status:** in-progress — The current session viewport has an explicit Environment selection independent of camera and editor entity selection. A removed Scene ID resolves through World with a non-blocking diagnostic and retained selection intent for undo restoration. No absent lighting capability is installed; absent, disabled and configured v1 lighting remain distinct. ABI 22 exposes read-only logical scope listing and current-viewport Environment selection/resolution through C API and managed bindings. The editor viewport has an explicit World/Scene selector; its catalog is polled at most every 250 ms, while resolution and fallback are read each frame. Focused C API and managed tests passed in Debug and Release, including empty scope, selection independence, removed-ID retention and World fallback. The current catalog has definitive removal but no temporary-unload operation; authored Scene Environment overrides, multiple viewports and visual acceptance remain open.
- `PIXIE-TASK-241` — **Status:** open — Reconcile each slice's evidence, local benchmark results where applicable, module/editor ABI and dependency modes, deferred decisions, and the task 229 integration handoff before closing the parent. VMware/software-renderer qualification is deferred by owner decision (2026-11-03); do not schedule or treat it as a gate for current TODO-057 work until the owner reactivates it.
