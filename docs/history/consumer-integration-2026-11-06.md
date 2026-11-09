# Consumer integration closure: sprite exports and player exit

**Lifecycle:** historical
**Roadmap source:** Legacy direct-owner work; no separate roadmap block was recorded.
**Planning baseline:** `402634e5a45f805f95c6f3c2bd21e0b3310d24fc` (`TODO.md`, PIXIE-TODO-058 and PIXIE-TODO-059).
**Path-normalized planning baseline:** `006d5446675e016eb6b91bea2dc5c1439dfdca8f`.
**Work IDs:** PIXIE-TODO-058, PIXIE-TODO-059; proposals 019 and 020.
**Outcome:** Previously implemented and consumer-validated; removed from active planning on 2026-11-06.
**Evidence:** Prior recorded results below and the linked proposals; no validation rerun during this migration.
**Limitations:** Local Windows consumer evidence only. Historical raw logs are not bundled or newly verified. Performance and editor-host lifetime are outside this closure.

## Preserved roadmap

These legacy items originated directly in owner requests on 2026-11-02, without
a dedicated roadmap block. No retrospective roadmap wording is invented.
The planning baseline preserves their exact accepted scope and criteria.

The purpose was to unblock LogicSandbox's existing sized/tinted ScreenCanvas
sprite calls and provide native Play/Quit and contextual Resume/Quit behavior.
The accepted boundary excluded rendering/simulation changes, Project vtable or
layout changes, and closing embedded editor hosts.

## Acceptance disposition

| Work | Original acceptance | Recorded disposition |
|---|---|---|
| PIXIE-TODO-058 | All LogicSandbox Release targets link and its complete CTest suite passes | The prior TODO records all targets linked, 151 tests with zero failed checks, and five focused consumer browser checks. Performance qualification remains separate. |
| PIXIE-TODO-059 | Consumer menu request/isolation tests and complete Release build pass; native menu renders | The prior TODO records consumer validation; its exact additional observations are preserved below. Embedded hosts retain lifetime ownership. |

## Important nested evidence

**PIXIE-TODO-058 validation (2026-11-02):** all LogicSandbox Release targets linked; full CTest suite passed (151 tests, zero failed checks). Consumer browser follow-up passed five focused tests. Evidence: consumer docs/build-performance-2026-11-02.md and build/release-tests.log. This is local Windows evidence only.

**PIXIE-TODO-059 validation (2026-11-02):** all consumer Release targets built; 163 consumer tests passed, including request isolation by World, menu Quit state, pause restoration and input ownership. Native start screen rendered and inspected; normal timed player shutdown succeeded. Evidence: consumer docs/interaction-polish-2026-11-02.md and build/polish-tests-final.log. No editor-window close behavior is introduced.

## Source decisions

- [Sized ScreenCanvas export](../proposals/screen-canvas-sprite-export.md)
- [World-scoped native exit request](../proposals/project-quit-request.md)

The Git planning baseline owns the earlier scheduler wording. Existing consumer
logs and reports remain their original evidence owners, not new repository artifacts.
