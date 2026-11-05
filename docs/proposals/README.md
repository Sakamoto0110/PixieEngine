# Proposal Index

**Document ID:** GLOBAL-PROPOSAL-INDEX

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** concise unpromoted ideas that require an owner decision

**Surface:** index

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Proposal files are lightweight idea records, one per file. They are not
specifications, commitments, or scheduling authority. A proposal becomes work
only through formal promotion to [`TODO.md`](../../TODO.md); other sources may
also be promoted directly.

When a proposal is promoted, rejected, superseded, or split, keep the file,
update its state, and link the outcome.

| Proposal | ID | State | Promotion state |
|---|---|---|---|
| [`engine-module-boundaries.md`](engine-module-boundaries.md) | PIXIE-PROPOSAL-001 | idea | Dependency direction promoted to `ROADMAP.md` (2026-10-10); `runtime` and `capi` boundaries promoted to `TODO.md` (2026-10-13); the rest requires an owner decision |
| [`editor-native-bridge.md`](editor-native-bridge.md) | PIXIE-PROPOSAL-002 | promoted | Guardrail in `ROADMAP.md` and PIXIE-TODO-010, PIXIE-TODO-011 (2026-10-13) |
| [`vertical-slice.md`](vertical-slice.md) | PIXIE-PROPOSAL-003 | superseded | Superseded by PIXIE-PROPOSAL-004 (2026-10-13) |
| [`scene-model-and-anchors.md`](scene-model-and-anchors.md) | PIXIE-PROPOSAL-004 | promoted | Object model and anchors in `ROADMAP.md` and PIXIE-TODO-005 to PIXIE-TODO-011 (2026-10-13); serialization carried by PIXIE-PROPOSAL-005 (2026-10-14) |
| [`projects-and-scenes.md`](projects-and-scenes.md) | PIXIE-PROPOSAL-005 | promoted | Guardrails in `ROADMAP.md` and PIXIE-TODO-018 to PIXIE-TODO-022 (2026-10-14), all completed the same day |
| [`authoring-in-the-viewport.md`](authoring-in-the-viewport.md) | PIXIE-PROPOSAL-006 | promoted | PIXIE-TODO-024 and PIXIE-TODO-025 (2026-10-14), completed by 2026-10-15 |
| [`sprites.md`](sprites.md) | PIXIE-PROPOSAL-007 | promoted | PIXIE-TODO-027 and PIXIE-TODO-028 (2026-10-15), completed the same day |
| [`interactive-2d-games.md`](interactive-2d-games.md) | PIXIE-PROPOSAL-008 | promoted | PIXIE-TODO-029 and PIXIE-TODO-030 (2026-10-15), completed the same day |
| [`logic-sandbox.md`](logic-sandbox.md) | PIXIE-PROPOSAL-009 | promoted | PIXIE-TODO-032 and PIXIE-TODO-033 (2026-10-15), with PIXIE-TODO-034 and PIXIE-TODO-035 (2026-10-16); all completed by 2026-10-16 |
| [`shared-engine-and-export.md`](shared-engine-and-export.md) | PIXIE-PROPOSAL-010 | promoted | PIXIE-TODO-051 (2026-10-28); EnTT gate first, shared engine/export and separately isolated local MCP development channel |
| [`scene-text.md`](scene-text.md) | PIXIE-PROPOSAL-011 | promoted | PIXIE-TODO-031 (2026-10-15), completed the same day |
| [`player-builds-once.md`](player-builds-once.md) | PIXIE-PROPOSAL-012 | idea | Requires an owner decision |
| [`dev-channel.md`](dev-channel.md) | PIXIE-PROPOSAL-013 | promoted | PIXIE-TODO-051 (2026-10-28); local Engine/Inspector MCP channel; save remains disabled pending owner question 2 |
| [`deterministic-capture.md`](deterministic-capture.md) | PIXIE-PROPOSAL-014 | promoted | PIXIE-TODO-051 (2026-10-28); fixed-step captures and approved snapshot scenarios; sample/tolerance choices remain explicit |
| [`gpu-particle-simulation.md`](gpu-particle-simulation.md) | PIXIE-PROPOSAL-015 | idea | Requires an owner decision; measure the CPU simulation at high counts first |
| [`world-and-scene-properties.md`](world-and-scene-properties.md) | PIXIE-PROPOSAL-016 | idea | Distinct scopes and property inheritance; loading/simulation policy and implementation remain undecided |
| [`editor-authoring-consolidation.md`](editor-authoring-consolidation.md) | PIXIE-PROPOSAL-017 | promoted | PIXIE-TODO-056 (2026-11-02); blocks A-G preserve ordered priorities, scope boundaries and later design/UX gates |
| [`world-scene-reconciliation.md`](world-scene-reconciliation.md) | PIXIE-PROPOSAL-018 | promoted | PIXIE-TODO-057 completed (2026-11-04); [scope and qualification](../archive/todo-057-world-scene-reconciliation-2026-11-04.md). |
| [`screen-canvas-sprite-export.md`](screen-canvas-sprite-export.md) | PIXIE-PROPOSAL-019 | promoted | PIXIE-TODO-058 (2026-11-02); restore the existing sized sprite API export for consumer builds |
| [`project-quit-request.md`](project-quit-request.md) | PIXIE-PROPOSAL-020 | promoted | Native player graceful close query, requested with the consumer menus on 2026-11-02 |
| [`engine-sdk-editor-separation.md`](engine-sdk-editor-separation.md) | PIXIE-PROPOSAL-021 | promoted | [PIXIE-TODO-060](../../TODO.md#pixie-todo-060--separate-engine-sdk-game-runtime-and-editor-delivery) (2026-11-04); B0-B11, whole single-agent batches and G0-G6; dependent decisions remain open, B7/B8 conditional and future VAW adoption nonblocking |
| [`standalone-editor-extraction.md`](standalone-editor-extraction.md) | PIXIE-PROPOSAL-022 | proposed | Detailed standalone Editor extraction and relocation contract; owner review requested, no extraction or publication authorized by drafting |
