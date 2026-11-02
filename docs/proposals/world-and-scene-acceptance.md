# World and Scene: independent acceptance protocol

**Document ID:** PIXIE-REVIEW-WORLD-SCENE-EXPERIMENT

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** independent review of Luna's World/Scene experiment by GPT-6.1-sol Medium

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Assignment and authority

Parent: [PIXIE-PROPOSAL-016](world-and-scene-properties.md).
Implementation contract: [execution blueprint](world-and-scene-implementation.md).
Recorded 2026-11-01 at the owner's explicit request.

The owner requires independent acceptance for this experiment: an implementer
cannot accept its own work. Luna implements and performs developer verification.
GPT-6.1-sol with Medium reasoning reviews the result in a separate reviewer role
and issues the acceptance decision. This is a task-specific owner requirement,
not a claim that VAW already defines or enforces this policy globally.

The reviewer may be the author of the blueprint, as requested by the owner,
but must not have authored the implementation under review. Treat Luna's
comments, receipts, screenshots, and claims as inputs to examine, not authority
to skip gates. Model choice alone is not independence: preserve separate roles,
evidence, and decisions. A model/context change never converts the implementer's
self-verification into independent acceptance.

Do not start implementation through this document. Review starts only after
implementation is authorized and a candidate is handed over. Acceptance does
not authorize commit, merge, push, publication, release, or unrelated work.

## 1. Required handoff and freeze

Require Luna to deliver:

- Baseline commit, dirty-file inventory and hashes before implementation; final
  candidate commit or exact patch plus hashes of tracked AND untracked inputs.
- Hashes of the proposal and implementation blueprint followed, plus any owner
  decisions that changed them. No retroactive contract edits to excuse behavior.
- Changed-file list, summary, build configuration, dependencies/toolchain, native
  ABI version, executable paths, fixture commands, final exit codes and logs.
- Requirement-to-evidence map for all twelve acceptance groups in the blueprint,
  known limits, and an explicit list of deviations or unfinished requirements.
- Confirmation that implementation/build/test sessions have stopped and the
  candidate will remain unchanged during review.

Independently inspect status, HEAD, actual files, processes and hashes. Include
pending changes from the input/code and Inspector integration work in the baseline
inventory so they cannot be mistaken for Luna's scope. Missing baseline/provenance
means INCONCLUSIVE until reconstructed from independent evidence; do not guess.

Create a separate immutable review directory:
`.local/validation/world-scene-review/<timestamp>/`.
Record reviewer model/reasoning, role, candidate identifier, contract hashes,
toolchain and inputs in `REVIEW-STATE.json`. Never overwrite Luna's receipts.
Do not run against whichever binary happens to have the latest timestamp.

Prefer a private checkout/copy of the exact candidate with isolated build outputs
and temporary fixtures. If rebuilding in the shared checkout, first establish
that no other task is editing/building the relevant sources or normal outputs.
Unresolved concurrency is INCONCLUSIVE, not evidence of a passing candidate.

## 2. Review the implementation before trusting tests

Read the complete feature diff and relevant unchanged callers. Trace these paths:

1. Definition CRUD -> membership -> resolver -> effective gravity.
2. Resolver -> rigid-body compensation, motor tuning, particle-system stepping.
3. Scope or World edit -> native synchronization -> sleep/wake/contact lifecycle.
4. Read/write -> validation -> staging/commit -> unknown-data preservation.
5. Entity fragments -> duplicate/undo/redo -> stable entity and scope IDs.
6. C API -> managed bindings -> Scenes panel/Inspector -> history -> normal binary.

Write `requirements.md`: for each numbered blueprint decision and acceptance
group, list code location, reviewer observation, producer evidence, independent
evidence, and PASS/FAIL/UNVERIFIED. Test names alone are not coverage.

Specifically reject sentinel inheritance, multiple World registries/backends,
backend-global gravity switching per scene, scalar-ratio compensation, duplicate
managed gravity resolution, new generic arbitrary-property frameworks, silent
unsupported-character fallback, and delivery only in an isolated executable.
Check deferred mutation timing, stale body handles, static/kinematic/proxy
exclusion, current mass and units, and applying compensation exactly once.

Review blueprint feasibility as well as code. If following the blueprint exactly
exposes a design flaw, record it as a contract issue and request a bounded revised
decision. Do not accept broken behavior merely because the plan prescribed it.

## 3. Independent behavioral checks

Inspect Luna's test changes for deleted cases, weakened assertions, fake fixtures,
test-only production bypasses and circular expected values calculated by the same
resolver being tested. Rebuild the candidate yourself from the captured inputs.
Re-run the mandatory blueprint matrix in Debug and Release; use serial runs for
fixtures sharing workspaces. Capture commands, configs, outputs and exit codes.
These reviewer runs are separate evidence even when reusing an existing suite.

In addition, create reviewer-owned scenarios without copying Luna's expected-value
logic. Use the public native API or existing real runtime harness, not source-text
searches. Minimum independent checks and numerical oracles:

| Case | Independently calculated expectation |
|---|---|
| Inherit, multiply, override | World `(0,120)`; inherit `(0,120)`; factor 0.25 `(0,30)`; override `(40,0)` unchanged |
| Change World | World `(0,240)` changes the first two to `(0,240)` and `(0,60)`; override stays `(40,0)` |
| Zero base | World `(0,0)` with override `(0,80)` and entity scale 0.5 gives acceleration `(0,40)` |
| Accumulation | At rest, no damping/contact, known fixed dt: compare velocity change to `a * dt * ticks` with backend-appropriate documented tolerance |
| Mass independence | Different densities/mass overrides with identical settings give equal acceleration, not equal compensation force |
| Mixed consumers | Two rigid bodies, two particle systems and supported motors use their own scopes in one World; unsupported sideways motor does not poison the others |
| Changed membership | Keep pose/velocity/IDs/handle/contact identity; next complete tick uses new gravity; deferred assignment waits until following tick |
| Sleep | Reapplying unchanged settings preserves sleep; changing effective acceleration wakes the affected dynamic body |
| Bad input | Unknown scope, NaN, duplicate ID or fragment conflict rejects before changing World data, RNG or files |
| Round trip | Independently inject unknown JSON fields, then save/load/duplicate/undo; compare semantic preservation and stable IDs |

Do not use a broad tolerance that would allow World gravity or a double-applied
multiplier to pass. Separate numerical integration error from the resolver's exact
vector expectations. Keep tests temporary or clearly reviewer-owned and provide
their sources and hashes with the review artifacts.

Demonstrate sensitivity on a PRIVATE candidate copy: deliberately bypass the
scene multiplier or compensation and show the corresponding reviewer behavioral
check fails for the intended reason. Then restore and verify the copy's hashes
and passing check. Never mutate the shared source or user scenes for this step.
An unrelated crash or compile failure is not successful sensitivity evidence.

## 4. Real editor and adoption checks

Build the normal editor configurations with the recorded local dependency setup.
Inspect native library, managed library, ABI and asset provenance separately.
Confirm the normal `editor/PixieEditor/bin/{Debug,Release}/net10.0/` outputs really
contain the feature, not just a special validation output or stale native DLL.
Launch the exact captured binary for each configuration and preserve its path.

Exercise actual UI input through the existing supported automation/fixture path:
create two scopes, rename, change mode/value/factor, assign entities, undo/redo,
save/reopen, Play/Stop, and reject assigning the World settings entity. Verify
native readback, not only displayed text or a handler's true return value.
If live UI automation is unavailable, record the missing evidence and leave
that required gate UNVERIFIED; programmatic handler calls cannot replace it.

Record screenshots with candidate/configuration provenance. Check labels,
source/effective-value display and accessibility of the feature. The reviewer's
functional/visual inspection does not substitute for the owner's visual acceptance.
Report that owner gate separately as PENDING/ACCEPTED/REJECTED with actual evidence.

Run existing relevant regressions listed in the blueprint; do not expand to
unrelated platforms or claim unexecuted CI matrices. Validate docs/whitespace.
Recompute candidate hashes after verification: a changed source invalidates
affected evidence and requires an explicitly identified new candidate.

## 5. Decision rules

Write `DECISION.md`, with a short outcome first and evidence links near claims.
The independent technical outcome is exactly one of:

- **ACCEPTED:** every required technical gate has direct, candidate-matching
  evidence; no unresolved correctness/data-preservation/required-scope issue.
- **REJECTED:** observed behavior or implementation violates a required contract.
  A test suite passing does not override a demonstrated defect.
- **INCONCLUSIVE:** evidence, provenance, execution capability or stable candidate
  is missing. This must never be presented as acceptance.

No weighted score or "mostly passes" acceptance. Distinguish candidate defects,
pre-existing baseline failures and unavailable environment capabilities. A
baseline failure is not automatically exempt if it prevents qualifying this feature.
Lower-risk findings may be listed separately only if they do not violate a required
contract; give the reasoning. Required missing functionality is not an advisory.

Include a per-gate table, reviewed candidate/contract hashes, independently run
commands and results, findings with reproduction and file/line locations,
limitations, and owner visual status. Say exactly "gravity/scope experiment"
rather than claiming independent asset streaming or the whole World/Scene vision.

## 6. Corrections and re-review

Send findings to the implementer with requirement, reproduction, expected and
observed result, and evidence. Luna fixes them and returns a new frozen candidate.
The reviewer does not silently repair production code and accept that repair.
If the reviewer authors a production correction, that correction needs another
independent reviewer before acceptance.

For the next candidate, review the complete delta from the last reviewed state,
rerun each failed case and affected neighboring regressions, and recheck hashes,
dependencies and normal output adoption. Retain older decisions as immutable
history. A documentation-only correction does not justify rerunning every native
matrix; a resolver/physics change does invalidate relevant behavioral evidence.

Return the final decision to the assigning chat. Do not mark scheduler work
complete, publish, or ask the owner for final visual acceptance while required
technical checks remain unresolved. Independent acceptance is pending until this
protocol has actually been executed; writing this file is not an acceptance run.
