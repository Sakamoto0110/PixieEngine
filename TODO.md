# PixieEngine Promoted Work

**Document ID:** GLOBAL-TODO

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** accepted work, status, dependencies and links to roadmap acceptance

**Surface:** work-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

**Last promotion decision:** 2026-11-05

## Purpose and admission rule

This file owns the active work queue. The roadmap owns scope and acceptance;
this file states what remains, with dependencies and links to decomposition. It does not hold ideas
([`docs/proposals/`](docs/proposals/README.md)), direction
([`ROADMAP.md`](ROADMAP.md)), or execution narratives
([evidence](docs/evidence/README.md) and [history](docs/history/README.md)).

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

**Status:** in-progress (remaining acceptance dispositions recorded; implementation and local cutover exist).
**Source:** [Proposal 021](docs/proposals/engine-sdk-editor-separation.md), with the [proposal 022 extraction amendment](docs/proposals/standalone-editor-extraction.md).
**Accepted scope:** [Roadmap outcome](ROADMAP.md#pixie-roadmap-060).
**Tracking:** the breakdown retains the previously recorded task states and scope; the [execution record](docs/evidence/sdk-editor-separation-2026-11-06.md#execution-results-and-gate-dispositions) owns exact results and limitations. This cleanup does not reopen accepted deliveries or schedule additional qualification.
**Dependencies and gates:** preserve installed-SDK identity and source boundaries, completed World/Scene behavior, project data/history and independent Inspector/Lib ownership. D13 is deferred; B8 stays inactive. The [D09 freeze](ROADMAP.md#d09-glfw-distribution) remains in force. VAW adoption does not block separation; publication requires separate authorization. This documentation change authorizes no new qualification run.
**Completion criteria:** [Roadmap acceptance criteria](ROADMAP.md#pixie-roadmap-060).
**Breakdown:** [Remaining separation dispositions](TASKLIST.md#pixie-todo-060--engine-sdk-game-runtime-and-editor-separation).
**Recorded delivery:** M4 and the local G6 rehearsal are preserved in the [milestone record](docs/evidence/sdk-editor-separation-2026-11-06.md#milestones-and-closure); accepted decisions and scope amendments are retained there with their original limits.

### `PIXIE-TODO-061` — Native entity parenting

**Status:** in-progress.
**Source:** owner promotion of [Proposal 023](docs/proposals/native-entity-parenting.md) on 2026-11-07.
**Accepted scope:** [Roadmap outcome](ROADMAP.md#pixie-roadmap-061).
**Dependencies and gates:** start with the simple runtime fixture. Resolve the authored format and compatibility before wiring persistence and the independent Editor. Weapons-Lab is reserved for acceptance after the complete simple fixture passes.
**Breakdown:** [Parenting implementation and qualification](TASKLIST.md#pixie-todo-061--native-entity-parenting).
**Recorded delivery:** [Runtime fixture in Release](docs/evidence/native-parenting-runtime-2026-11-07.md) and [persisted simple scene through the SDK, Editor and no-code player](docs/evidence/native-parenting-persistence-2026-11-07.md). [The Editor hierarchy, structural history and code-bearing fixture](docs/evidence/native-parenting-editor-2026-11-07.md) passed the local qualification. [Weapons-Lab r5 consumer acceptance](docs/evidence/native-parenting-weapons-lab-2026-11-07.md) passed on the separate copy and paired player.

## Completion records

Use the [history index](docs/history/README.md) for completed outcomes and the
[legacy archive](docs/archive/README.md) for earlier records. The
[evidence index](docs/evidence/README.md) also covers delivered slices of still-open work.
