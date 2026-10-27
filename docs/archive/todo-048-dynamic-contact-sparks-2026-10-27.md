# Dynamic Contact Sparks Completed: PIXIE-TODO-048

**Document ID:** ARCHIVE-TODO-048-DYNAMIC-CONTACT-SPARKS-20261027

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** bounded lightweight particle dynamics and native contact-driven sparks, with physics prerequisite qualification

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-085

**Type:** completion

**Origin:** PIXIE-TODO-048 (PIXIE-TASK-194 through PIXIE-TASK-198)

**Author:** codex

**Reference date:** 2026-10-27

**Commits:** `3eb87f02f9147b4153f8b847c89309ce8471d87a` (code)

**Reference commit:** `d03db62c775005fc4c614f07e205b7a4a36d9efa`

**Current state:** [dynamic particle guide](../particle-dynamics.md), [runtime reference](../modules/runtime/REFERENCE.md), [remaining physics scheduler](../../TASKLIST.md#pixie-todo-043--rigid-body-dynamics)

## Accepted scope and implementation

The owner requested gravity/bounce sparks, a player pushing a physical box,
a reset shortcut, then freely rotating rectangles, one real-mass textbox,
right-click emission, left-dragging, ramps and box-on-box impact/sliding sparks.
The owner approved publication through a pull request to main on 2026-10-27;
this record does not authorize or claim that merge.

The 40-byte particle layout and one shared sample pool remain. A birth flag
selects lightweight gravity/circle-query dynamics; no particle is a rigid body.
Native shape materials and point velocity drive bounce/friction. Four responses
bound each particle's collision work. Invalid pool configuration reports an
entity diagnostic and falls back ballistically, without blocking other pools.
The direct API still throws, while World stepping separately restores its state
on exceptions. Physics is an explicit phase before PostUpdate consumers; legacy
integration precedes native geometry/solving, and particle-only installation is
independent. Shared gravity defaults and spawner field order/comment were fixed.

The room reads actual native contact manifolds/impulses, not a box-floor proximity
heuristic. Impact count is mass weighted; relative tangential speed with friction
produces sliding sparks. Canonical shape/point keys prevent duplicate emission;
steady loads are suppressed. Per-point/tick emission is bounded at 32/256 into
a 10,000-capacity pool. Two ramps, a platform, rectangles and orange stack/drop
boxes exercise the shared backend. Dragging uses a bounded point spring force;
the query-based player uses safe capsule movement. The single textbox edits
native mass/inertia and preserves velocity. R restores poses/masses/motion,
particles, emitter history and random sequence.

## Physics prerequisites retained in the same code commit

D3 workload fixtures/telemetry and D4 prepare/solve/finish character/body coupling
were preserved and delivered with this increment. Their source coverage includes
pushing, platform carry and blocked/crush handling. This is not completion of
PIXIE-TODO-043: workload limits, independent feel acceptance and final physics
qualification remain explicit in its scheduler.

## Task snapshot

**Parent:** PIXIE-TODO-048

- `PIXIE-TASK-194` — **Status:** done — Native dynamic births, gravity/circle-sweep bounce and materials, descriptors and telemetry implemented. Nine new cases bring runtime coverage to 229; Debug/Release full builds and all 17 CTest suites passed, with a detected no-bounce fault and exact restoration.
- `PIXIE-TASK-195` — **Status:** done — Click sparks, player-pushed dynamic box floor sparks and R reset implemented. Persistent C API controls and native framebuffer checks passed Debug/Release, including held-key reset, deterministic replay and authored Stop restoration; disabled clearing failed and was restored.

- `PIXIE-TASK-196` — **Status:** done — Added three free-rotation rectangles, one selected-body mass textbox and corrected pointer controls. Native C API checks passed Debug/Release for tipping, inertia scaling, invalid mass, authored reset, physical lift/release and existing sparks/push controls; hidden GL captures verified decimal-comma mass input and unchanged scene. Extended under tasks 197/198.

- `PIXIE-TASK-197` — **Status:** done — Fixed per-pool diagnostic/ballistic fallback, exception-state recovery, independent Physics phase with legacy integration preserved, shared gravity default and spawner field clarity. Six new regressions bring runtime coverage to 235; Debug/Release full matrices and Editor build passed.
- `PIXIE-TASK-198` — **Status:** done — All boxes use native manifold/impulse impact and sliding sparks. Two ramps, platform and stack/drop boxes added. Isolated green/floor, ramp, box-on-box, mass scaling, sliding-only, quiet-stack and native render/reset checks passed Debug/Release; one 40-byte particle pool remains.

## Verification and evidence limits

The VAW source was `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`; profile `pixieengine/1` resolved `bound` for all seven routes (build, capi, documentation, editor, engine, runtime and samples). Binding is not build evidence.

- Full Debug/Release native builds completed without compiler warnings/errors.
- 235 runtime tests and all 17 CTest suites passed per configuration. Full local
  matrices took 176.80 s (Debug) and 174.17 s (Release), including project-code
  compilation/loading and the native C API/lifecycle cases. Editor Release built
  with zero warnings/errors. These are local Windows/OpenGL observations; CI is
  checked separately after publication.
- Isolated fixtures remove the blue box: green/floor emitted 8 sparks, ramp 14,
  box-on-box 11. Ramp/box tests assert emission before floor-contact height.
  The same 4 kg drop emitted 16 versus 8 at 1 kg in this bounded fixture.
- An isolated sliding-only fixture emitted zero at initial rest, 76 during motor
  pushing, then zero at rest; all five fixtures passed 60 quiet ticks and checked
  surviving per-tick counts within the cap. Both affected suites were rerun per
  configuration after adding that last fixture.
- Native controls cover mass/inertia edits, rejected zero, force lift/release,
  R with movement held, invalid radius diagnostics/recovery and scene lifecycle.
  Hidden GL captures in both configurations showed ramp/box contacts, 52 live
  sparks at tick 120, cleared R state and unchanged authored scene SHA.
- Publication preflight found one sample source matching the earlier reset
  sensitivity snapshot rather than the validated revision. Both copies/diff were
  preserved, the exact validated source was recovered, and sample builds plus
  both affected suites passed again in Debug/Release before committing.
- Local detailed evidence remains under .local/validation/contact-sparks-2026-10-26
  and .local/validation/publication-2026-10-27; it is machine-local and not part of
  a clean clone's proof. Persistent regressions are committed with the source.

No calibrated abrasion/heat model, 100k dynamic-particle budget, universal
allocation-free contact emitter or independent physical-pointer acceptance is
claimed. Particle queries use completed body poses; arbitrary-speed moving-body
crossings are not qualified. Query-only characters are not particle obstacles.
The C API remains version 16: its exports/layouts did not change. Inspector
capability/package integration is a separate consumer task, not proven by a
sibling checkout or these Engine tests.
