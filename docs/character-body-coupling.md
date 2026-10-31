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
Since the owner's decision of 2026-10-31, leaving a carrying support without jumping
(walking or sliding off it) also adds that clamped velocity once, in the finish that loses
the support, so a character does not drop the platform's motion into a sudden free fall.
A support removed, disabled or explicitly moved is not inherited. Subsequent air
input/braking retains the existing motor rules; leaving drops the anchor, without an
indefinitely retained platform reference.

Initial authored coupling defaults: effective mass 1 kg, push force limit 1200
kg*worldUnits/s^2, contact push speed limit 200 worldUnits/s, support-load force
limit 1200, inherited-speed limit 512 worldUnits/s, maximum support displacement
8 world units/tick, and recovery distance 8 world units. These are tunable game
parameters, not physical character inertia or a universal supported envelope.

Carry and overlap recovery are bounded. If a moved body/platform leaves no
placement within the declared recovery tolerance, keep the last completed pose,
clear support and publish blocked/unsafe status. A carry clipped by other geometry,
with a valid placement, is blocked without inventing a crush/death rule; a carry shortened
only by the support itself (the capsule settling on a rotated surface) is not an obstruction. The retained pose may
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

## Character proxies

Owner decision, 2026-10-31: a scene character is also represented in the physics world by
a character proxy (`CollisionWorld2D::AddCharacterProxy`), so dynamic bodies and particles
stop against it instead of passing through. The proxy is a kinematic upright capsule, smaller
than the character by the mover's touching tolerance, placed where the character completed
the last step and not moved during the solve. It has no mass, raises no contact or sensor
events, never blocks a character (motor queries leave proxies out, so character/character
collision remains unsupported) and is not a second simulated character body. It counts in
the world's body and shape populations; debug views draw the character capsule instead.

- Pushing is unchanged: the motor's bounded impulses move bodies, and the smaller proxy does
  not push what the character only touches.
- What bodies press on the proxy during the solve is read from its contacts and passed to
  `CharacterController2D::ReceiveImpulse`; finish moves the character by impulse divided by
  effective mass, capped at the push speed limit and through collision, without changing its
  velocity. `CharacterCouplingState2D::receivedImpulse` reports it.
- `CharacterMotor2D` has `belongs to` and `collides with` layers like Collider2D's. They filter
  the character's own queries and its proxy, so a body or particle pool whose layers leave out
  the character's passes through it. Particle pools choose this per pool, not per particle.
- A fast body can enter the proxy within the tick it arrives (dynamic/kinematic pairs are not
  swept continuously); from the next tick it stays outside, and the character's overlap
  recovery moves the character out of the way.

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
they do not measure this coupled path. The benchmark's `coupled` workload does: reversing
translating, rising and rotating platforms with riders, pushes into light, heavy and
compound boxes, loaded crates and recurring crushes, measured on 2026-10-30 (see the
[qualification record](physics-qualification.md#workloads-and-stress)). The owner accepted the
movement feel in the [coupling lab](../samples/coupling-lab/README.md) on 2026-10-31. Unsafe placement is explicit failure evidence, not a guarantee of
continuous collision at arbitrary speed.

Grounding ignores separation slower than 1 world unit/s. Pushing a box that is free to
rotate tilts its face slightly, and clipping against it leaves hundredths of a unit/s of
upward velocity; with the earlier 0.001 tolerance the motor lost its ground and published
support for almost every pushing tick. `CharacterPushingARotatableBoxKeepsItsGroundAndSupport`
covers this.

Owner review on 2026-10-30 found two more defects in the coupling lab, both fixed:

- A grounded character crept downhill on every walkable slope (6.6 units/s at 23°): gravity
  was added in full and clipping turned its tangential part into motion. On a tilting
  platform this showed as sliding while the rider's side sank and stopping while it rose.
  Grounded gravity now only presses along the support's normal (identical on flat ground);
  `CharacterStandsStillOnWalkableSlopes` and `CharacterStaysPutOnATiltingAndReversingPlatform`
  cover it.
- A rider on a fast-tilting platform (0.48 rad/s) was reported as carry obstructed on every
  sinking tick, although only its own support shortened the carry.
  `CharacterSettlingOnAFastTiltingSupportIsNotAnObstruction` covers it.

The prepare/solve/finish algorithm is unchanged.

The 2026-10-26 bounded increment passed 211 runtime tests and all 15 CTest suites
in Debug and Release. Disabling actual support carry produced 10 failed checks;
byte-for-byte source restoration passed again. A hidden native C API review
session rendered the local room in Edit and Play, observed support carry and
crush diagnostics over 60 ticks, and restored authored poses on Stop without
rewriting its scene. Evidence and the openable local room are retained under
`.local/validation/character-coupling-2026-10-26/`; these are local evidence, not a
published package, shared acceptance artifact or owner feel sign-off.
