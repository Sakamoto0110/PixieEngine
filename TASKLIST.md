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
