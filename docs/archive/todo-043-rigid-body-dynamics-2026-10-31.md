# Rigid Body Dynamics and Authoring Completed: PIXIE-TODO-043

**Document ID:** ARCHIVE-TODO-043-RIGID-BODY-DYNAMICS-20261031

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** native rigid bodies, materials, events, editor authoring, character/body coupling and their qualification, accepted by the owner

**Surface:** archive-record

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-087

**Type:** completion

**Origin:** PIXIE-TODO-043 (PIXIE-TASK-180 through PIXIE-TASK-185)

**Author:** claude

**Reference date:** 2026-10-31

**Commits:** D0–D4 code through `89a7c90f1230c6d06f838c410f878f96f23614c0`; consolidation code `c82401886b1703170e3eaecc023ef3f94cac3183`, `22f35ca`, `ccc2234a2a9e24181dfff668932f1ab03673bd12`

**Reference commit:** `f80501a`

**Current state:** [qualification record](../physics-qualification.md), [body dynamics guide](../body-dynamics.md), [character/body coupling](../character-body-coupling.md), [coupling lab](../../samples/coupling-lab/README.md)

## Accepted scope and outcome

The owner authorized the detailed physics specification on 2026-10-22 and accepted PHY-001
through PHY-021 of the [physics requirements](../rigid-body-physics-requirements.md), delivered
through D0–D5. Lighting follows this consolidation; it was never scope here, and this record
does not promote it.

D0–D4 delivered one native body service per scene World with units, world gravity, computed and
overridden mass, commands, materials, layers, copied events, compound authoring, editor presets,
handles, debug views, telemetry and the prepare/solve/finish character/body coupling. D5 (this
closing increment, 2026-10-30/31) reconciled every requirement with its mechanism and evidence in
the [qualification record](../physics-qualification.md), added a mixed coupled workload and a
render-rate trace test, qualified the no-code editor workflow over MCP, and built the
[coupling lab](../../samples/coupling-lab/README.md) for the owner's review.

The owner's review in the coupling lab found defects that were fixed with regressions and
mutation evidence: pushers losing their ground against boxes free to rotate, characters creeping
down walkable slopes, and false carry obstruction on fast-tilting supports. On 2026-10-31 the owner
decided two algorithm changes, both delivered: character proxies (bodies and particle pools stop
against characters, filtered by character layers, and push them within the push speed limit) and
support velocity inherited once on leaving a support without a jump. Character surface materials
(friction-driven sliding) were deferred to a separate item, which this record does not promote.

## Owner acceptance

On 2026-10-31, after playing the coupling lab and seeing the fixes, the owner stated in the
working session that the physics scope is accepted and that work may proceed.
This is recorded testimony covering movement feel, pointer use, debug views and interpolated
rotation as reviewed in that room; no owner screenshot is attached. Agent-made captures under the
local evidence folder show the room's states and are not the owner's evidence.

## Task snapshot

**Parent:** PIXIE-TODO-043

- `PIXIE-TASK-180` — **Status:** done — D0: pin ownership, units, compatibility bridge and initial contracts in the body dynamics guide.
- `PIXIE-TASK-181` — **Status:** done — D1: native/scene body baseline, world settings, mass/rotation/commands and lifecycle; 137 runtime tests per configuration and 13 Release integration scenarios passed.
- `PIXIE-TASK-182` — **Status:** done — D2: copied contact/sensor lifetimes, scene identities, sensor-inclusive queries and bounded material/stack/fast-body qualification; 157 runtime tests per configuration, omitted-sensor sensitivity and 13 Release integration scenarios passed. Transient moving-wall overlap is documented.
- `PIXIE-TASK-183` — **Status:** done — D3: presets, admission/minimum guards, full collider dimensions, compound authoring, named collision layers, gravity migration, read-only effective mass (C API 13), collider and capsule handles (C API 14/15), native debug views (C API 16), scene telemetry and sparse/compound/stack/sensor workload qualification, as recorded in the D3 milestones. On 2026-10-30 the no-code editor physics workflow passed 42 programmatic MCP cases and the five-workload stress run included the mixed coupled workload. Pointer use and debug views accepted by the owner on 2026-10-31.
- `PIXIE-TASK-184` — **Status:** done — D4: prepare/solve/finish coupling, bounded pushing, platform carry, blocked/crush reporting; on 2026-10-30/31 fixes for grounding while pushing, slope creep and false carry obstruction, character proxies and inheritance on leaving (owner decisions of 2026-10-31) and the coupling lab. Movement feel accepted by the owner on 2026-10-31.
- `PIXIE-TASK-185` — **Status:** done — D5: [qualification record](../physics-qualification.md) with PHY-001..021 coverage, executed versus historical evidence, render-rate trace equality, three serial stress runs, three serial build/test/MCP matrices and residual limits; owner review accepted on 2026-10-31.

## Verification and evidence limits

The VAW source was `c9d81ba`; profile `pixieengine/1` resolved `bound` for all seven routes.
The [qualification record](../physics-qualification.md) owns the executed results. In summary,
on the owner's Windows machine (Ryzen 7 5700X, RX 7600, Visual Studio 2026, MSVC 19.51):

- the last serial matrix passed vs2026 and ci CTest 22/22 in Debug and Release, 245 runtime
  tests with no failed check, MCP 177 (vs2026) and 170 (ci) cases per configuration including
  the 42-case editor physics suite, with no compiler warning; documentation checks passed;
- every fix and decision has a regression whose test-only mutation failed and whose
  byte-for-byte restoration passed;
- the last stress run measured 30 character proxies at about 0.04 ms per tick on their own and
  about 0.09 ms among 500 bodies; figures describe that machine and those fixtures only.

Machine-local evidence is under `.local/validation/physics-completion-2026-10-30/`; it is not
part of a clean clone's proof. Clean-clone CI runs when the branch is published.

## Remaining limits

Joints, character/character collision, motor trigger events, character surface materials,
contact events for character proxies and arbitrary-speed continuous character/body collision are
outside this delivery. A fast body can enter a character proxy within its arrival tick. No
universal no-tunneling guarantee, zero-allocation claim or gameplay budget is made. Unsafe
placement reports failure; damage, mechanism reversal and respawn are game policy.
