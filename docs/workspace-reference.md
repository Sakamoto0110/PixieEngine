# Pinned pre-transfer coordination reference

**Document ID:** GLOBAL-WORKSPACE-REFERENCE
**Schema version:** 1
**Kind:** reference
**Lifecycle:** historical
**Subject:** immutable pre-transfer context for standalone project readers
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** snapshot
**Indexing:** include

This is historical reading context, not a current queue or execution authority.
Current coordination, status and acceptance belong to the linked workspace.
A missing parent or block context is unresolved; these excerpts cannot authorize
implementation, qualification, D09 resumption, publication or activation.
TASKLIST block constraints are not reproduced here and remain required context.
Links inside excerpts are rendered as pinned origin coordinates, never redirected
to current truth. Existing statuses are baseline statements, not new decisions.
Source: `pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97` (recorded local commit; no publication claim).

[Current workspace outcomes](../../../.governance/workplaces/void-workplace/ROADMAP.md), [accepted work](../../../.governance/workplaces/void-workplace/TODO.md) and [required block constraints](../../../.governance/workplaces/void-workplace/TASKLIST.md).

## ROADMAP excerpts

<!-- Source ROADMAP.md; block SHA256 5612d3ab694873123991562a2363e290cc6bb7835f1e37a6fd276eb4fec87765 -->
### PIXIE-ROADMAP-053

**Outcome:** Inspector fields, options and interactions overhaul

**Rationale and source:** owner promotion on 2026-10-31: review and overhaul all the Inspector fields and options, and how they interact. Promoted deliberately vague; the owner marked it expansion required.

**Accepted scope:** every field and option the editor's Inspector shows for engine components and settings, and the interactions between them, such as options that enable, disable, hide or override other fields, and fields that have no effect in a given mode. The review covers labels, units, ranges, defaults, tooltips and widget choice. The overhaul applies the decisions from that review. The expansion defines the inventory, the target rules and the order of work, and where changes land: engine metadata, the editor or PixieInspector.

**Acceptance criteria:** to be defined by the expansion. At minimum, every reviewed field and option has a recorded decision, and the agreed changes are covered by Inspector qualification over the development channel.

**Work:** PIXIE-TODO-053 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TODO.md#pixie-todo-053--inspector-fields-options-and-interactions-overhaul`). Execution status,
dependencies and task decomposition remain there. This phase retains its existing
acceptance limits; documentation migration does not close it.


<!-- Source ROADMAP.md; block SHA256 5d7f21d6fb87330d3a56e99b024829b48d0ad72772876ac8b9f8c0850d79dbef -->
### PIXIE-ROADMAP-054

**Outcome:** Field chaining and manual edit locks

**Rationale and source:** owner authorization on 2026-11-01, including chaining Shape and Collider dimensions on the same selected objects.

**Accepted scope:** selection-scoped numeric chains and direct-edit locks through PixieInspector's generic API; per-bind total-delta scrubbing and absolute view-value entry; cross-component displayed dimensions; grouped native history, cancellation and lifecycle invalidation. Local source adoption is opt-in; the default published dependency stays unchanged.

**Acceptance criteria:** isolated Debug/Release builds, native-backed regression evidence for propagation, locks, history and lifecycle, default-package compatibility, publish refusal in local mode, documented limits, and a delivery handoff to the originating chat. Owner visual acceptance is recorded separately.

**Work:** PIXIE-TODO-054 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TODO.md#pixie-todo-054--field-chaining-and-manual-edit-locks`). Execution status,
dependencies and task decomposition remain there. This phase retains its existing
acceptance limits; documentation migration does not close it.


<!-- Source ROADMAP.md; block SHA256 2cfe4da458176a9c4a77708f3214e9bb14bf7c33f1bdedb5661029053fc6a3d7 -->
### PIXIE-ROADMAP-055

**Outcome:** Entity input controls and C++ component authoring

**Rationale and source:** owner authorization on 2026-11-01: generic entity KeyboardControl/MouseControl and C++ components created through Add component, with Open code and normal-build adoption; design and explicit legacy adapter reviewed by the coordinating owner chat.

**Accepted scope:** ordered raw device events and enabled-aware entity queries/callbacks through Behaviour; explicit KeyboardControl2D compatibility adapter; preserving C++ generation, registration/source metadata, CMake integration, VS Code opening, rebuild/reopen and stable selection/unsaved state. No gameplay/action mapping in the generic controls.

**Acceptance criteria:** runtime lifecycle/order/fixed-step/capture regressions; generated multi-component C++ code compiles and persists through scene/editor/player; failure/preservation fixtures for code/CMake paths; normal editor Debug/Release builds expose the feature; proportional native/editor validation and independent review.

**Work:** PIXIE-TODO-055 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TODO.md#pixie-todo-055--entity-input-controls-and-c-component-authoring`). Execution status,
dependencies and task decomposition remain there. This phase retains its existing
acceptance limits; documentation migration does not close it.


<!-- Source ROADMAP.md; block SHA256 99655b1f3075ba4e43c9a8c48c0721776e8167136cf2867544008b2536eb4b4a -->
### PIXIE-ROADMAP-056

**Outcome:** Editor and C++ authoring consolidation

**Rationale and source:** owner promotion on 2026-11-02 of PIXIE-PROPOSAL-017 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/proposals/editor-authoring-consolidation.md`), including the subsequent section 4.1 rework, selection-aware headers, and removal of extra controller prerequisites for Behaviour events.

**Accepted scope:** consolidate the delivered C++ workflow into usable create/attach/edit/build operations, minimal contextual templates and discoverable grouped Behaviours; provide callbacks without attaching KeyboardControl/MouseControl merely to unlock events; reproduce/correct viewport gesture defects; inventory Inspector fields and clarify presets; evaluate Session/viewport presentation; coordinate the separate World/Scene experiment; retain later diagnostics and editor-asset/runtime-resource manager work as gated blocks. Documentation is delivered with each slice.

**Acceptance criteria:** all numbered child tasks are done or explicitly dropped with reasons; one real normal-editor exercise demonstrates create/attach/code/IntelliSense/build/play/save/reopen, contextual component access, visible build/attachment state and predictable camera gestures; reviewed field/preset decisions and UI prototype receive owner acceptance; each implemented slice has candidate hashes, relevant regressions, independent review, normal Debug/Release adoption and a concise guide. Deferred manager/telemetry blocks must either receive their bounded designs and qualification or an explicit disposition before parent closure; no blanket completion while they remain open.

**Work:** PIXIE-TODO-056 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TODO.md#pixie-todo-056--editor-and-c-authoring-consolidation`). Execution status,
dependencies and task decomposition remain there. This phase retains its existing
acceptance limits; documentation migration does not close it.


<!-- Source ROADMAP.md; block SHA256 f8491780f12527bb8fad1cb57892214b5e1439c8e3bbeced9993cabead456b68 -->
### PIXIE-ROADMAP-060

**Outcome:** Separate Engine SDK, game runtime and Editor delivery

**Rationale and source:** owner decision on 2026-11-04: "promote proposal 021" (translated from Portuguese); PIXIE-PROPOSAL-021 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/proposals/engine-sdk-editor-separation.md`), consolidated on the same date.

**Accepted scope:** independently consumable and relocatable Engine SDK; qualified authoring/shipping runtime profiles; installed-package game support and generation; compatible native/module/managed interop; SDK selection and full authoring-state continuity; native export with runtime-only closure; extraction of Editor into its own local project/repository; bounded automation, external-consumer qualification, coherent product documentation and independent release-candidate pipelines. Adopt proposal contracts C01-C12 as acceptance targets and blocks B0-B11 as bounded ownership units. B7 Inspector and B8 Lib remain conditional on D08/D04; preserve their independent ownership, versions and releases. Separately owned projects may live outside Engine, without a sibling-layout requirement. Preserve completed proposal 018 / PIXIE-TODO-057 and unrelated active work.

**Acceptance criteria:** every child task is done or dropped with a reason; G0-G6 pass for exact accepted SDK/runtime/Editor artifacts and their supported tuples. External consumers build/run with producer checkouts unavailable and no silent source fallback; code/no-code exports contain only qualified runtime dependencies and preserve authored data; the independent Editor preserves the selected authoring/Inspector baseline, unsaved rebuild/reopen and mismatch/recovery behavior across two SDKs; allowed automation and artifact exclusions are qualified; current documentation has one owner per contract and working references; independent release candidates pass provenance, payload and pipeline rehearsal without publication. Existing 018 evidence remains bounded regression input; newly separated artifacts require their own applicable runtime/visual evidence. Completion does not wait for VAW.

**Work:** PIXIE-TODO-060 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TODO.md#pixie-todo-060--separate-engine-sdk-game-runtime-and-editor-delivery`). Execution status,
dependencies and task decomposition remain there. This phase retains its existing
acceptance limits; documentation migration does not close it.


<!-- Source ROADMAP.md; block SHA256 6af7a6323c7e3fa5797c1af41d2119100356fb3ea0e767bc880c71fe7c12d60b -->
### PIXIE-ROADMAP-061

**Outcome:** Native 2D entity parenting and hierarchy authoring in the independent Editor.

**Rationale and source:** the owner promoted PIXIE-PROPOSAL-023 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/proposals/native-entity-parenting.md`) on 2026-11-07 after the Weapons-Lab composition exposed the need for persistent parent links, relative poses and separately editable entities.

**Accepted scope:** runtime relationships and pose evaluation, authored scenes and snapshots, a compatible SDK and Editor hierarchy UI. Preserve the meaning of flat world-space transforms and existing scene formats. Game-specific fault rules remain in the game. The D09 freeze (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#d09-glfw-distribution`) applies.

**Acceptance criteria:** the simple three-level fixture passes runtime, authored persistence, SDK, Editor history/rebuild and standalone export checks before Weapons-Lab is used for consumer acceptance. Each surface records its own evidence. Flat projects retain existing poses, and unsupported readers refuse scenes that require parenting.

**Work:** PIXIE-TODO-061 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TODO.md#pixie-todo-061--native-entity-parenting`).


## TODO excerpts

<!-- Source TODO.md; block SHA256 06d498115a88fb592acef4f1b0c55fc2691446f15cdf0626566a68407243905b -->
### `PIXIE-TODO-053` — Inspector fields, options and interactions overhaul

**Status:** promoted (expansion required)
**Source:** owner promotion on 2026-10-31: review and overhaul all the Inspector fields and options, and how they interact. Promoted deliberately vague; the owner marked it expansion required.
**Accepted scope:** Roadmap outcome (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-053`).
**Dependencies and gates:** expansion required. No implementation starts until the inventory and the proposed rules have the owner's review and this entry has a concrete scope and completion criteria. Edits keep undo/redo, save/reopen and the development channel's Inspector paths, and existing scenes load unchanged. Native engine code never depends on the editor. Lighting fields added by PIXIE-TODO-052 join the review once they exist.
**Completion criteria:** Roadmap acceptance criteria (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-053`).
**Breakdown:** none


<!-- Source TODO.md; block SHA256 26c3c29c20558724682e9d31ccb8be0685e2994157daee8d0d8fe2e8a1c19ae7 -->
### `PIXIE-TODO-054` — Field chaining and manual edit locks

**Status:** promoted (local integration ready; owner visual review pending)
**Source:** owner authorization on 2026-11-01, including chaining Shape and Collider dimensions on the same selected objects.
**Accepted scope:** Roadmap outcome (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-054`).
**Dependencies and gates:** Inspector owns the generic implementation. This bounded authorization does not unfreeze PIXIE-TODO-053. No commit, push or publication is authorized.
**Completion criteria:** Roadmap acceptance criteria (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-054`).
**Breakdown:** none


<!-- Source TODO.md; block SHA256 4f5c72ecc701e9cb93f2e306ca1300d79ff15b8a3825278d184f82a0c6d40bfa -->
### `PIXIE-TODO-055` — Entity input controls and C++ component authoring

**Status:** promoted (local implementation qualified; independent review pending)
**Source:** owner authorization on 2026-11-01: generic entity KeyboardControl/MouseControl and C++ components created through Add component, with Open code and normal-build adoption; design and explicit legacy adapter reviewed by the coordinating owner chat.
**Accepted scope:** Roadmap outcome (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-055`).
**Dependencies and gates:** preserve existing uncommitted field interaction/New folder work and machine-local Inspector adoption settings. No commit, push or publication is authorized. Native ABI changes require an explicit version bump and rebuild diagnostics.
**Completion criteria:** Roadmap acceptance criteria (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-055`).
**Breakdown:** Entity input and C++ component authoring (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TASKLIST.md#pixie-todo-055--entity-input-and-c-component-authoring`)


<!-- Source TODO.md; block SHA256 c758da3d63b6722e035be075614743a6bc8de1f9a57b52f9392b161e1cb11d9a -->
### `PIXIE-TODO-056` — Editor and C++ authoring consolidation

**Status:** promoted (ordered blocks; later design and UX gates retained)
**Source:** owner promotion on 2026-11-02 of PIXIE-PROPOSAL-017 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/proposals/editor-authoring-consolidation.md`), including the subsequent section 4.1 rework, selection-aware headers, and removal of extra controller prerequisites for Behaviour events.
**Accepted scope:** Roadmap outcome (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-056`).
**Dependencies and gates:** preserve PIXIE-TODO-055's delivered C++ capabilities, source/CMake ownership, IntelliSense and unsaved/selection handoff, and PIXIE-TODO-054's interaction contracts. PIXIE-TODO-053 retains its inventory/rules review gate; the corresponding block below does not authorize an unreviewed generic Inspector rewrite. Proposal 016 retains its own World/Scene design, Luna assignment and independent acceptance; this entry authorizes coordination, not implementing that backend incidentally. Blocks A/B can be assigned independently; later blocks require their stated inputs, not completion of a World rewrite. Promotion records the work and does not launch every block, authorize a permanent final toolbar layout, or authorize commit/push/publication.
**Completion criteria:** Roadmap acceptance criteria (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-056`).
**Breakdown:** Editor and C++ authoring consolidation (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TASKLIST.md#pixie-todo-056--editor-and-c-authoring-consolidation`)


<!-- Source TODO.md; block SHA256 6f1857defa0e06eae45dd6bc805768d0ecc73103ec0964c567dbf1c9d9c4b755 -->
### `PIXIE-TODO-060` — Separate Engine SDK, game runtime and Editor delivery

**Status:** in-progress (remaining acceptance dispositions recorded; implementation and local cutover exist).
**Source:** Proposal 021 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/proposals/engine-sdk-editor-separation.md`), with the proposal 022 extraction amendment (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/proposals/standalone-editor-extraction.md`).
**Accepted scope:** Roadmap outcome (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-060`).
**Tracking:** the breakdown retains the previously recorded task states and scope; the execution record (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/evidence/sdk-editor-separation-2026-11-06.md#execution-results-and-gate-dispositions`) owns exact results and limitations. This cleanup does not reopen accepted deliveries or schedule additional qualification.
**Dependencies and gates:** preserve installed-SDK identity and source boundaries, completed World/Scene behavior, project data/history and independent Inspector/Lib ownership. D13 is deferred; B8 stays inactive. The D09 freeze (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#d09-glfw-distribution`) remains in force. VAW adoption does not block separation; publication requires separate authorization. This documentation change authorizes no new qualification run.
**Completion criteria:** Roadmap acceptance criteria (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-060`).
**Breakdown:** Remaining separation dispositions (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TASKLIST.md#pixie-todo-060--engine-sdk-game-runtime-and-editor-separation`).
**Recorded delivery:** M4 and the local G6 rehearsal are preserved in the milestone record (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/evidence/sdk-editor-separation-2026-11-06.md#milestones-and-closure`); accepted decisions and scope amendments are retained there with their original limits.


<!-- Source TODO.md; block SHA256 ad994f7343e92687e7f21fe25387844499ed483d202e247aefc1118e64dc6c2b -->
### `PIXIE-TODO-061` — Native entity parenting

**Status:** in-progress.
**Source:** owner promotion of Proposal 023 (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/proposals/native-entity-parenting.md`) on 2026-11-07.
**Accepted scope:** Roadmap outcome (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:ROADMAP.md#pixie-roadmap-061`).
**Dependencies and gates:** start with the simple runtime fixture. Resolve the authored format and compatibility before wiring persistence and the independent Editor. Weapons-Lab is reserved for acceptance after the complete simple fixture passes.
**Breakdown:** Parenting implementation and qualification (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:TASKLIST.md#pixie-todo-061--native-entity-parenting`).
**Recorded delivery:** Runtime fixture in Release (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/evidence/native-parenting-runtime-2026-11-07.md`) and persisted simple scene through the SDK, Editor and no-code player (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/evidence/native-parenting-persistence-2026-11-07.md`). The Editor hierarchy, structural history and code-bearing fixture (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/evidence/native-parenting-editor-2026-11-07.md`) passed the local qualification. Weapons-Lab r5 consumer acceptance (`pixieengine@02ba6ab5818f6d252e615356c71aa15aa57fff97:docs/evidence/native-parenting-weapons-lab-2026-11-07.md`) passed on the separate copy and paired player.
