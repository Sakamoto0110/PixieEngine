# Authoring deliveries — recorded outcomes

**Document ID:** GLOBAL-AUTHORING-EVIDENCE-20261106

**Schema version:** 1

**Kind:** audit

**Lifecycle:** historical

**Subject:** delivered 055 implementation, 056 integration and accepted viewport prototype

**Surface:** audit

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Reference date:** 2026-11-06

**Planning baseline:** `1f44ae74008f1c2df4ff9b86ee8bb44f8ce8e072`, `TASKLIST.md`.

This record preserves important completed slices removed from active planning.
It records existing outcomes; no test, review or product acceptance ran during
this documentation change. Earlier complete task wording remains in Git.

## Entity input and component authoring

PIXIE-TASK-219 through PIXIE-TASK-224 were recorded as done: ordered platform
input, enabled-aware controls and Behaviour callbacks, native/managed metadata,
preserving code generation, Editor adoption and bounded qualification.
[The technical guide](../entity-input-and-code-components.md) owns behavior and
qualification boundaries. The parent [PIXIE-TODO-055](../../TODO.md#pixie-todo-055--entity-input-controls-and-c-component-authoring)
retains its recorded independent-review disposition; completed child tasks do
not silently close it.

## World and Scene handoff

**Work ID:** PIXIE-TASK-229. **Recorded status:** done.

Qualified the minimal handoff against native/editor ABI28, scene envelope v2, presentation schema1 and cooperative module contract1. The real C++ parent/child editor rebuild passed14 checks in Debug and Release, preserving dirty custom data, empty scopes/membership, World/view/Renderer and Scene Environment/gravity/camera values while retaining the saved file. Runtime/CAPI references and editor README own the contract; ProcessReopenTests.cs is the reproducible demonstration. This completes interface integration only; it does not accept the separate historical backend experiment or close the other TODO056 blocks.

## Viewport, Session and Entities prototype

**Work ID:** PIXIE-TASK-231. **Recorded status:** in-progress for the wider scope.

**Owner UX decision (2026-11-02):** accepted the delivered Viewport, Session and
Entities visual prototype: "a parte UX ficou boa, aprovo". The owner reviewed the
normal editor candidate after its technical qualification and independent read-only
review. This acceptance does not close the C++ authoring, Inspector-field or
World/Scene gates, or authorize commit/push/publication.

The accepted visual slice is preserved. Its remaining task stays in
[TASKLIST](../../TASKLIST.md#block-e--presets-session-and-viewport-controls-p12-proposal-62).
