# Character and Body Coupling

**Document ID:** RUNTIME-CHARACTER-BODY-COUPLING-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** pinned D4 motor/platform/body coupling algorithm and qualification boundaries

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Accepted algorithm before implementation

Owner decision, 2026-10-26: retain the query-based upright motor, add explicit
platform following, bounded impulses and blocked-placement reporting. Physics
and gizmos remain native Engine responsibilities. No second simulated character
body or character/character collision is introduced.

The scene tick synchronizes authored data, collects gameplay commands, then:

1. Prepare every motor once, in stable entity order. Discard the old support
   anchor and query current enabled nonsensor geometry with the current filter.
   This handles teleports, replacement, removal, disabled bodies and filter changes
   without carrying any pose delta from the previous completed tick.
   Integrate input, jump timers and velocity exactly once. Predict the capsule's
   requested movement using queries without committing a second integration.
2. Apply horizontal push impulses to contacted dynamic bodies, skipping floors,
   ceilings and sensors. Bound their total per motor/tick by push force times dt,
   effective character mass times closing speed, and contact-point speed limit.
   Account for body mass and rotational inertia. Compound contacts do not multiply
   the motor's force budget. Apply bounded downward effective load to dynamic
   support; static and kinematic supports receive no impulses/load.
3. Advance the single native physics world once when simulation is needed.
   Static-only scenes keep the no-step optimization. The motor is not a solver body.
4. Finish each motor with query-only geometry corrections. For grounded riders,
   transform the stored local support point by the solved body pose and apply
   that displacement once, through collision constraints. Rotate the support
   point, not the upright capsule. Then resolve the motor's own prepared motion
   once and reacquire walkable support at the resulting pose.
5. Publish body and motor poses, diagnostics and copied events. Game policy
   consumes the completed state on the following update; no reentrant solve.

## Momentum, limits and failure

Jump takeoff adds the clamped support-point velocity (linear plus angular)
exactly once and detaches before carry correction, avoiding double carry.
Subsequent air input/braking retains the existing motor rules; stepping off
drops the anchor, without an indefinitely retained platform reference.

Initial authored coupling defaults: effective mass 1 kg, push force limit 1200
kg*worldUnits/s^2, contact push speed limit 200 worldUnits/s, support-load force
limit 1200, inherited-speed limit 512 worldUnits/s, maximum support displacement
8 world units/tick, and recovery distance 8 world units. These are tunable game
parameters, not physical character inertia or a universal supported envelope.

Carry and overlap recovery are bounded. If a moved body/platform leaves no
placement within the declared recovery tolerance, keep the last completed pose,
clear support and publish blocked/unsafe status. A clipped carry with a valid
placement is blocked without inventing a crush/death rule. The retained pose may
now be occupied by the mechanism: unsafe explicitly means no feasible placement
was established. Damage, stopping/reversing a platform and respawning are host
policy. Excessive support displacement is diagnosed rather than accepted as a
qualified sweep. Fast bodies that cross the whole capsule between queries remain
outside this bounded algorithm's guarantee.

## Compatibility and acceptance

Standalone `CharacterController2D::Step(const CollisionWorld2D&, ...)` keeps its
explicit query-only path. Coupled native callers use prepare, one body step,
then finish. Scene motors use that sequence automatically. Configuration,
teleport, reset and Play/Stop must not retain a pending operation or old anchor.
Authored motor schema advances to version 3 with defaults for new fields;
version 1 gravity migration remains unchanged and version 2 gains only defaults.

Executable cases must cover translating/rotating/reversing supports, jumping and
stepping off without duplicate carry, invalidated support, light/heavy pushes,
effective load, moving-body reception and blocked/crushing placement. Preserve
legacy movement and gravity fixtures. Passing native checks does not replace
owner feel review or imply arbitrary-speed continuous character/body collision.

## Runtime observations and qualification

`CharacterCoupling2D` returns borrowed native state, like `CharacterMotion2D`.
A nonzero support handle makes its local/world point and velocity meaningful.
`State().velocity` is the motor's velocity without carried motion while riding;
`worldVelocity` is completed position displacement divided by dt, including carry
and overlap recovery. Native character velocity gizmos use that world velocity.
`pushedImpulse` and `supportLoad` describe the current tick, not accumulated totals.
The existing C API movement status reports blocked or unsafe placement; no C ABI
version change or managed physics implementation is required. New authored limits
use ordinary native descriptors, including the generated Inspector.

The two-phase caller must finish before reconfiguring or using the query-only
path. Teleporting the motor cancels its pending phase and clears support; a world
reset between phases reports WorldChanged. An explicit body teleport, enabled-state
or filter change increments its revision; finish rejects that old anchor even
when a teleport is smaller than the carry limit. Body commands are applied before the
single backend step. Character/character collision and motor-trigger events are
outside this increment.

The native regression fixture is
[CharacterCouplingTests.cpp](../tests/runtime/CharacterCouplingTests.cpp).
It exercises horizontal/vertical/reversing supports, local rotation and jump
inheritance, support invalidation, light/heavy and compound-body pushes, bounded
effective load, moving-body reception, jumping against a box without launching it,
obstruction and kinematic/falling-body crush or step-limit failure.
The C API fixture additionally checks authored tuning, Save during Play, Stop,
restarting without stale anchors and exposed movement diagnostics. The older
movement/gravity tests remain independent regression checks.

Historical D3 workload CSVs describe their retained executable/source snapshots;
they do not measure this new coupled path. Coupled performance qualification and
owner movement-feel acceptance remain pending. Unsafe placement is explicit
failure evidence, not a guarantee of continuous collision at arbitrary speed.

The 2026-10-26 bounded increment passed 211 runtime tests and all 15 CTest suites
in Debug and Release. Disabling actual support carry produced 10 failed checks;
byte-for-byte source restoration passed again. A hidden native C API review
session rendered the local room in Edit and Play, observed support carry and
crush diagnostics over 60 ticks, and restored authored poses on Stop without
rewriting its scene. Evidence and the openable local room are retained under
`.local/validation/character-coupling-2026-10-26/`; these are local evidence, not a
published package, shared acceptance artifact or owner feel sign-off.
