# PixieEngine Promoted Work

**Document ID:** GLOBAL-TODO

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** formally promoted work, gates, dependencies, and completion criteria

**Surface:** work-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

**Last promotion decision:** 2026-11-05

## Purpose and admission rule

This file is the only scheduler. It holds work that the owner has formally
promoted, with enough scope to execute and close it. It does not hold ideas
([`docs/proposals/`](docs/proposals/README.md)), direction
([`ROADMAP.md`](ROADMAP.md)), or completion narratives
([`docs/archive/`](docs/archive/README.md)).

An entry whose scope is large, or that decomposes into many small tasks, keeps
one entry here and its breakdown in [`TASKLIST.md`](TASKLIST.md). The rules are
in the [documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd).

Unchecked or `promoted` entries are not automatically authorized to start; an
entry may keep an explicit gate after promotion.

## Current promoted work

### `PIXIE-TODO-053` — Inspector fields, options and interactions overhaul

**Status:** promoted (expansion required)
**Source:** owner promotion on 2026-10-31: review and overhaul all the Inspector fields and options, and how they interact. Promoted deliberately vague; the owner marked it expansion required.
**Accepted scope:** [Roadmap outcome](ROADMAP.md#pixie-roadmap-053).
**Dependencies and gates:** expansion required. No implementation starts until the inventory and the proposed rules have the owner's review and this entry has a concrete scope and completion criteria. Edits keep undo/redo, save/reopen and the development channel's Inspector paths, and existing scenes load unchanged. Native engine code never depends on the editor. Lighting fields added by PIXIE-TODO-052 join the review once they exist.
**Completion criteria:** [Roadmap acceptance criteria](ROADMAP.md#pixie-roadmap-053).
**Breakdown:** none

### `PIXIE-TODO-054` — Field chaining and manual edit locks

**Status:** promoted (local integration ready; owner visual review pending)
**Source:** owner authorization on 2026-11-01, including chaining Shape and Collider dimensions on the same selected objects.
**Accepted scope:** [Roadmap outcome](ROADMAP.md#pixie-roadmap-054).
**Dependencies and gates:** Inspector owns the generic implementation. This bounded authorization does not unfreeze PIXIE-TODO-053. No commit, push or publication is authorized.
**Completion criteria:** [Roadmap acceptance criteria](ROADMAP.md#pixie-roadmap-054).
**Breakdown:** none

### `PIXIE-TODO-055` — Entity input controls and C++ component authoring

**Status:** promoted (local implementation qualified; independent review pending)
**Source:** owner authorization on 2026-11-01: generic entity KeyboardControl/MouseControl and C++ components created through Add component, with Open code and normal-build adoption; design and explicit legacy adapter reviewed by the coordinating owner chat.
**Accepted scope:** [Roadmap outcome](ROADMAP.md#pixie-roadmap-055).
**Dependencies and gates:** preserve existing uncommitted field interaction/New folder work and machine-local Inspector adoption settings. No commit, push or publication is authorized. Native ABI changes require an explicit version bump and rebuild diagnostics.
**Completion criteria:** [Roadmap acceptance criteria](ROADMAP.md#pixie-roadmap-055).
**Breakdown:** [Entity input and C++ component authoring](TASKLIST.md#pixie-todo-055--entity-input-and-c-component-authoring)

### `PIXIE-TODO-056` — Editor and C++ authoring consolidation

**Status:** promoted (ordered blocks; later design and UX gates retained)
**Source:** owner promotion on 2026-11-02 of [PIXIE-PROPOSAL-017](docs/proposals/editor-authoring-consolidation.md), including the subsequent section 4.1 rework, selection-aware headers, and removal of extra controller prerequisites for Behaviour events.
**Accepted scope:** [Roadmap outcome](ROADMAP.md#pixie-roadmap-056).
**Dependencies and gates:** preserve PIXIE-TODO-055's delivered C++ capabilities, source/CMake ownership, IntelliSense and unsaved/selection handoff, and PIXIE-TODO-054's interaction contracts. PIXIE-TODO-053 retains its inventory/rules review gate; the corresponding block below does not authorize an unreviewed generic Inspector rewrite. Proposal 016 retains its own World/Scene design, Luna assignment and independent acceptance; this entry authorizes coordination, not implementing that backend incidentally. Blocks A/B can be assigned independently; later blocks require their stated inputs, not completion of a World rewrite. Promotion records the work and does not launch every block, authorize a permanent final toolbar layout, or authorize commit/push/publication.
**Completion criteria:** [Roadmap acceptance criteria](ROADMAP.md#pixie-roadmap-056).
**Breakdown:** [Editor and C++ authoring consolidation](TASKLIST.md#pixie-todo-056--editor-and-c-authoring-consolidation)

### `PIXIE-TODO-060` — Separate Engine SDK, game runtime and Editor delivery

**Status:** in-progress (initial SDK decisions accepted; scoped implementation and later gates retained)
**Source:** owner decision on 2026-11-04: "promote proposal 021" (translated from Portuguese); [PIXIE-PROPOSAL-021](docs/proposals/engine-sdk-editor-separation.md), consolidated on the same date.
**Dispatch:** on 2026-11-04 the owner assigned B0, K01 (B4/B1/B2), K02 (B3/B5), K03 (B9/B10) and B6 to five separate chats. On 2026-11-05 the coordinator assigned the whole B11 unit to B06 with a separate preparation root and assigned whole B7 to a sixth dedicated chat. The [dispatch register](TASKLIST.md#dispatch-register--owner-decision-2026-11-04) records their identities and scopes. B7's first D08 package milestone uses the committed Inspector source as read-only input. Inspector repository changes require separate exact review. Implementation requires B0's applicable G0 clearance. B8 is inactive under accepted D04. Reports return to the coordinator for independent validation and owner acceptance.
**Decisions:** the owner accepted D01-D05/D10 on 2026-11-04 and D06/D07/D11 in the [subsequent verified human answer](TASKLIST.md#accepted-selectiongenerationexport-decisions--verified-2026-11-05); TASKLIST records outcomes and timestamp limits. Public authoring/shipping SDK payloads exclude the separate private automation distribution. D04 keeps B8 inactive; its optional task is dropped with a reason. The [scoped clearances](TASKLIST.md#scoped-g0-clearance--b0-c1) cover only the listed identity/package, validator/consumer, Editor selection/launch, exporter, private automation and [generation](TASKLIST.md#scoped-g0-clearance--b0-c3) paths, with serial B0 root wiring. They are not whole-unit or product-gate acceptance.
**Accepted scope:** [Roadmap outcome](ROADMAP.md#pixie-roadmap-060).
**Scope amendment (2026-11-05):** the owner promoted [PIXIE-PROPOSAL-022](docs/proposals/standalone-editor-extraction.md) as the detailed plan for the existing Editor extraction phase. It establishes the revised D14 destination, transfer and path-resolution rules, preservation and relocation acceptance, and independent release preparation. The [task plan](TASKLIST.md#proposal-022--standalone-editor-extraction) maps the stages to existing owners.
**Dependencies and gates:** G0 reconciles the actual source and working-state baseline and freezes exact write paths, interfaces and immutable artifact inputs. D01-D12 are accepted at the recorded scope. D14 uses the destination recorded in the scope amendment. Extraction requires matching native GLFW versions and a graphics smoke check at OpenGL 4.5. Shared-binary implementation and detailed native-provider qualification remain under the [D09 freeze](ROADMAP.md#d09-glfw-distribution). Inspector peer writes and the exact extraction transaction require separate authorization. Actual artifact and interface prerequisites still apply. D13 is a deferred follow-up outside this scope. VAW delivery, installation, rebind, member registration and future validation block no separation gate. Preserve the single native runtime baseline, ABI28/contract1 admission and lifetimes, scene v1/v2 safeguards, full-document history/recovery, presentation/viewport semantics, local motor/pool gravity and global-body fallback. Dispatch only whole blocks or whole single-agent batches with one active owner per block. Physical extraction requires the reviewed B0 move ledger, a writer freeze and applicable stage gates. Publication requires separate authorization. Subsequent exact clearances belong to TASKLIST.
**Completion criteria:** [Roadmap acceptance criteria](ROADMAP.md#pixie-roadmap-060).
The local source and build cutover is recorded in the [task plan](TASKLIST.md#proposal-022--standalone-editor-extraction). The extraction phase also tracks the [proposal 022 completion criteria](docs/proposals/standalone-editor-extraction.md#11-completion-criteria-and-unresolved-selections), including staged relocation evidence and post-removal checks. On 2026-11-06, M4 closed from the existing consumer, cutover and guide evidence, and the authorized local rehearsal passed for the exact Engine SDK, Runtime and Editor candidates. The [milestone record](TASKLIST.md#milestones-and-closure) records the remaining G6 limits. The [OpenGL 4.3 VMware investigation](ROADMAP.md#deferred-investigation), remote releases and automatic SDK acquisition are later work.

**Closure work (2026-11-06):** the local Engine-to-Editor candidate handoff, locked Inspector restore and affected Editor package checks passed. Pipeline preparation, the applied local cutover and the C11 documentation work are complete. The [execution record](TASKLIST.md#execution-results-and-gate-dispositions) records the scope of the existing G0–G5 evidence and the remaining hosted workflow verification and Windows x64 native provenance under G6.

**Breakdown:** [Engine SDK, game runtime and Editor separation](TASKLIST.md#pixie-todo-060--engine-sdk-game-runtime-and-editor-separation)

## Completion records

Completed work is discoverable through the [history index](docs/history/README.md)
and the [legacy archive](docs/archive/README.md). This file contains active work only.
