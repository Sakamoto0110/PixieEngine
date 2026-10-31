# Physics Qualification and Requirement Coverage

**Document ID:** GLOBAL-PHYSICS-QUALIFICATION

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** PHY-001 through PHY-021 coverage, executed physics qualification, residual limits and the owner review that remains

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

This is the D5 consolidation record for
[PIXIE-TODO-043](../TODO.md#pixie-todo-043--rigid-body-dynamics-and-authoring). It links each
requirement of the [physics specification](rigid-body-physics-requirements.md) to its
mechanism and executable evidence, and states what only the owner can accept. Behavior is owned
by the public headers and the [runtime reference](modules/runtime/REFERENCE.md); the guides
linked below own each mechanism's contract. Nothing here is owner acceptance.

## Baseline and what ran

The 2026-10-30 consolidation started from `main` at
`7e49d4e7ed24cf5503b27911a54e2ac1ef862a85` (the shared engine in `pixie.dll`, C API 18 and the
MCP development channel). Its code changes are commit
`c82401886b1703170e3eaecc023ef3f94cac3183`; every run below used that code. Results below are local Windows evidence (Ryzen 7 5700X,
RX 7600, Windows 11 Pro 10.0.26300, Visual Studio 2026); they are not clean-clone CI.

| Executed on 2026-10-30 | Result |
|---|---|
| Baseline at `7e49d4e`, vs2026 Debug and Release | Builds current; CTest 20/20 each (262.0 s, 237.5 s); 235 runtime tests, 0 failed checks |
| After the changes, vs2026 Debug and Release | 238 runtime tests, 0 failed checks; `coupling-lab-module` and `-project` pass |
| After the owner's first review (slope and obstruction fixes, room additions) | 241 runtime tests, 0 failed checks; each fix's mutation fails only its new tests |
| Test-only mutations, each restored byte for byte (SHA-256 checked) and rerun | Old grounding tolerance: 104 failed checks, only in the new push test. Unsafe placement never reported: 4 failed checks in the two crush tests and the new mixed workload |
| [No-code editor physics workflow](#no-code-editor-workflow) over MCP, Release | 42/42 cases |
| [Mixed and stress workloads](#workloads-and-stress) | See that section |

The matrix that closes this increment (vs2026 and ci, Debug and Release, CTest, the MCP
qualification and the documentation check) is recorded in [Final matrix](#final-matrix).

Historical results keep their own baselines and are not rerun here: the
[TODO-048 archive](archive/todo-048-dynamic-contact-sparks-2026-10-27.md) (235 runtime tests and
17 suites per configuration before the shared-engine migration), the 2026-10-26 workload
qualification (144 runs, 172,800 warm ticks) and the D4 coupling report with its review room.
Those reports live in the machine-local `.local/validation/` folders named by the
[telemetry](physics-telemetry.md) and [coupling](character-body-coupling.md) guides. Their
allocation counts describe the statically linked engine: since the engine moved into
`pixie.dll`, the benchmark's counter sees only the host executable's `new`/`new[]`, so those
counts are not comparable with new runs and no claim about engine allocations is made here.

## Requirement coverage

"Native" means executable checks of the engine. "Owner" means the review only the owner can
give; it is pending everywhere it appears.

| Requirement | Mechanism | Executable evidence | Owner review | Status |
|---|---|---|---|---|
| PHY-001 one simulation owner | One `CollisionWorld2D` service per scene World; standalone stepping unchanged ([body dynamics](body-dynamics.md#scene-integration)) | `SceneBodyUsesSharedQueryWorldAndExclusiveIntegration`, `SceneBodiesDoNotCoupleTwoWorldsAndOldHandlesNeverRevive`; the mixed workload checks one coupling preparation and one solve per tick | None | Met |
| PHY-002 body and collider separate | `RigidBody2D` with `Collider2D` or `CompoundCollider2D` on one entity | `CompoundSceneUsesOneBodyWithIndependentShapeMassAndIdentity`, `PhysicsAuthoringAdmissionIsNativeSymmetricAndDoesNotDropLegacyData`; editor compound cases | None | Met |
| PHY-003 exclusive integration | Legacy integration skips bodies and motors; motor plus body is refused | `SceneMovementInstallationIsIdempotentAndDoesNotDoubleIntegrateVelocity`; editor case `motor-on-a-dynamic-body-is-refused`, whose refusal names the entity and the fix | None | Met |
| PHY-004 explicit units | World units, kg, kg/m², squared conversion at the backend; Inspector unit suffixes | `BodyMassAndTorqueUseSquaredLengthConversion`, `BodyCircleCapsuleMassAndOneStepForceHavePhysicalUnits`, `SceneScaleChangeRequiresResetAndCannotResumeOnTheFollowingTick` | None | Met |
| PHY-005 world gravity | `PhysicsSettings2D`; one per scene; motors scale it | `BodyGravityTypesAndScaleRemainDistinct`, `SceneWorldGravityScalesMotorsAndDetectsDuplicateSettings`; editor cases for the world preset, duplicate settings and recovery | None | Met |
| PHY-006 computed mass | Density-derived mass; sensors excluded; no invented unit mass | `BodyAreaCountsSolidGeometryIndependentlyOfDensityAndMotionType`, `SensorMassIsExcludedFromCompoundDistribution`, `BodyInvalidDataDoesNotCreateGhostGeometry`; editor case: zero density disables the body, undo recovers it | None | Met |
| PHY-007 explicit mass | `massOverride` scales the distribution; read-only effective mass in the Inspector | `BodyMassOverrideScalesInertiaAndImpulseResponse`, C API `body-authoring`; editor mass-override edit, undo and redo | Readability of the read-only rows | Met natively |
| PHY-008 body types | Static, kinematic and dynamic bodies in one service | `BodyGravityTypesAndScaleRemainDistinct`, `StaticCompoundsDoNotIntroduceSolverWorkWithoutMotionSensorsOrPendingEvents`, platform cases | None | Met |
| PHY-009 commands | Forces for one step, impulses, explicit teleport | `BodyRotationLockAndTeleportAreExplicit`, `SceneBodyRetuneAndTeleportHaveDifferentVelocityPolicy` | None | Met |
| PHY-010 shapes and materials | Boxes, circles, capsules, convex polygons, compounds; friction and restitution | `BodyRampContactsProduceRotationWithoutScriptedAngularMotion`, `BodyTenBoxStackQualifiesPenetrationSettlingAndSleepAtTenToOneMassRatio`, `BodySurfaceFrictionDistinguishesHoldingFromSlidingOnTheSameSlope`, `BodyRestitutionControlsBounceAndLowSpeedContactsSettle`, `BodyFastModeQualifiesThinStaticAndMovingWallsThrough1024UnitsPerSecond` | None | Met within the [qualified envelope](rigid-body-physics-requirements.md#9-acceptance-matrix) |
| PHY-011 layers | Named slots, two-sided filters, native bits kept | `CollisionLayerNamesAndUnknownItemDataRoundTripWithoutRewritingLegacyFilters`, C API `collision-layers`; editor layer-checkbox cases including the last-membership guard | None | Met |
| PHY-012 events and identities | Copied begin/end events with scene identities and generations ([body events](body-events.md)) | The `Sensor*`, `Contact*` and `Scene*` cases in `PhysicsEventsTests.cpp` | None | Met; body hit events are not offered |
| PHY-013 support motion | Local support point, one carry per step, inherited velocity once ([coupling](character-body-coupling.md)) | `CharacterFollowsTranslationReversalAndVerticalSupportExactlyOnce`, `CharacterRotatingSupportUsesLocalPointAndInheritsClampedVelocityOnce`, `CharacterDropsRemovedDisabledTeleportedOrRebuiltSupport`, `CharacterStaysPutOnATiltingAndReversingPlatform`, `CharacterSettlingOnAFastTiltingSupportIsNotAnObstruction`, `CharacterStandsStillOnWalkableSlopes`; the mixed workload | Movement feel in the [coupling lab](../samples/coupling-lab/README.md); the owner's first observations led to the [slope fixes](#defects-found-and-fixed) | Native met; owner pending |
| PHY-014 pushing and receiving | Bounded push impulses and support load | `CharacterPushBudgetDistinguishesLightAndHeavyBoxesWithoutLaunchingThem`, `CharacterPushingARotatableBoxKeepsItsGroundAndSupport`, `CharacterEffectiveSupportLoadIsBoundedAndDoesNotChangeNativeMass`, `CharacterReceivesMovingBodyWithoutASecondSimulatedCharacter`, `CharacterJumpAgainstBoxDoesNotLaunchTheBox` | Push strength and caps in the coupling lab | Native met; owner pending |
| PHY-015 blocked and crushing | Last valid pose, blocked and unsafe placement reported, platform not reversed by the engine | `CharacterCarryObstructionIsBlockedWithoutUnsafePlacementOrPlatformReversal`, `CharacterCrushAndUnsupportedCarryRetainLastPoseAndReportUnsafePlacement`, `CharacterFallingBodyCrushReportsFailureWithoutReversingTheBody`; recurring crush in the mixed workload; the room's crusher reverses on crush as game policy | Whether the reported statuses and the game-side policy suit games | Native met; owner pending |
| PHY-016 fixed timeline | Host fixed stepper; one prepare, one solve, one finish per tick; shortest-path interpolation | `FixedStepTraceIsIndependentOfTheRenderRate` (30, 60 and 144 render frames per second give bitwise-equal traces; stepping by the frame delta does not); the engine's `FixedStepper` tests | Smooth interpolated rotation, seen | Native met; visual check pending |
| PHY-017 persistence and reset | Authored save; Play uses runtime state; Stop rebuilds | `SceneBodyAuthoredStateRoundTripsWithoutBackendMotion`, `SceneSensorRoundTripsWithoutSavingActivePairs`, C API Save during Play; editor Play/Stop, save/reopen and second-session cases | None | Met (bounded) |
| PHY-018 migration | Full Width/Height presentation; v1 motor gravity converted | `LegacyMotorGravityBecomesWorldGravityScaleWithoutChangingTheFall`, `OnlyOlderMotorVersionsConvertLegacyGravity`, `MovementLabVersionOneSaveLoadsWithItsOriginalGravity`; editor cases: full width shown over half extents, v1 gravities 1650 and 0 become scales 1.5 and 0 | None | Met |
| PHY-019 authoring workflow | Presets, grouped Inspector, handles, compound and layer UI | The [no-code editor workflow](#no-code-editor-workflow); `ColliderHandlesFollowGeometryUnderReflectedScaledRotatedTransforms` and the other collider-tool cases | Physical-pointer use of the editor | Native and programmatic met; owner pending |
| PHY-020 debug views and errors | Native debug lines; diagnostics once per change; recovery by undo | `PhysicsDebug*` cases; editor debug captures in Edit and Play; duplicate-settings and zero-density recovery cases | Debug-view readability | Native met; owner pending |
| PHY-021 telemetry | Stage timings, populations, collection on/off, headless workloads ([telemetry](physics-telemetry.md)) | [Workloads and stress](#workloads-and-stress) | None | Met with the stated allocation limit |

## No-code editor workflow

`tests/mcp/qualify_mcp.py` runs the `editor-mcp-physics` suite
(`tools/pixie-mcp.tests/EditorPhysicsQualification.cs`). It starts the editor on an empty
project without code and authors physics only through the editor's existing gestures and
Inspector rows over the local MCP channel:

- the world, static, dynamic, kinematic, compound and sensor presets, each one history step;
  a second world preset selects the existing settings;
- the Inspector's full size over native half extents, density in kg/m² and a mass override in
  kg, with undo and redo; a negative density clamped to zero disables the dynamic body, and
  undo recovers it;
- collider resize and rotate handles with undo, redo and cancel; named layers and their
  checkboxes, including the last-membership guard; compound items added, edited and undone;
- a motor refused on a dynamic body; debug views captured in Edit and Play;
- Play, a fall onto the authored floor and Stop restoring the authored poses; duplicate world
  settings disabling the simulation until undone;
- save, reopen, a second editor session stepping without touching the first, and a version 1
  movement scene whose motor gravities 1650 and 0 become gravity scales 1.5 and 0.

These are programmatic gestures. They prove native state, history and the editor's handlers;
they do not prove that the workflow is comfortable with a pointer, which remains the owner's
review. The diagnostics panel text is observed only in captures, not asserted.

## Review room

[`samples/coupling-lab`](../samples/coupling-lab/README.md) is the room for the owner's
movement-feel and debug-view review: translating, rotating and rising platforms that reverse,
a platform that tilts past the walkable limit, 25° and 55° ramps, a carry obstruction, a
crusher, a ball, light, heavy and compound boxes, a stack the ram knocks over, dynamic particles
raining on bodies and platforms, R to reset, the mouse to drag and drop bodies, and a panel
listing each character's support, velocities, push impulse, support load and status. Its README
lists what to judge. Shuttle reversal, including reversing when a carried character is crushed,
is the sample's code (game policy), not an engine feature. Release captures of the room at fixed steps, driven over MCP, were inspected
during qualification for layout and reported status only; they are not owner acceptance.

## Defects found and fixed

All three are in `runtime/src/CharacterController2D.cpp`, each with a regression that fails when
the fix is reverted (test-only mutation, then byte-for-byte restoration). The coupling algorithm
is unchanged.

**Pushing a box free to rotate.** While checking the room, pushing a box that is free to rotate left the motor ungrounded and
without support in 103 of 104 pushing ticks (none with `fixedRotation`): the pushed face tilts
slightly, clipping against it leaves a few hundredths of a unit/s of upward velocity, and the
grounding test treated any separation faster than 0.001 units/s as leaving the ground. Jumping
still worked, but `grounded`, the published support and any support load were wrong during
such pushes. Grounding now ignores separation slower than 1 unit/s (a jump is hundreds).
`CharacterPushingARotatableBoxKeepsItsGroundAndSupport` covers it.

**Creeping down walkable slopes** (found in the owner's review of the room). The owner saw a
rider on the tilting platform slide while its side sank and stop when it rose. The cause was not
the platform: a grounded character crept downhill on any walkable slope, static or moving (3.1,
6.6 and 9.0 units/s at 10°, 23° and 40°), because a full tick of gravity was added and clipping
against the slope turned its tangential part into motion. Grounded gravity now only presses
along the support's normal, which is unchanged on flat ground. A standing character stays put;
on the tilting platform the rider returns to the same local point every cycle (net drift under
0.05 units per cycle at 40°, against 12 units before).
`CharacterStandsStillOnWalkableSlopes` and `CharacterStaysPutOnATiltingAndReversingPlatform`
cover it; the mutation failed 611 checks in exactly those two.

**False carry obstruction on a fast-tilting support.** At 0.48 rad/s the capsule meets the
rotated surface before the full carry, and every such tick was reported as carry obstructed.
Obstruction now requires other geometry opposing the carry.
`CharacterSettlingOnAFastTiltingSupportIsNotAnObstruction` covers it; the mutation failed it
alone.

**Persistent overlap after a crush** (the owner's second observation). This is the specified
behavior, not a defect: when no placement exists the engine keeps the last valid pose and reports
unsafe placement, and the mechanism may then pass through the capsule; stopping or reversing the
mechanism, damage and respawn are game policy (PHY-015). The room now shows that policy: its
crusher reverses as soon as its rider is crushed, and the owner can switch that off to see the
engine's own behavior. An engine-side ejection of the capsule would change the pinned D4
algorithm and needs an owner decision.

## Workloads and stress

The headless benchmark gains a `coupled` workload: ten lanes, each a reversing kinematic
platform (translating, rising under a ceiling, or rotating) carrying a jumping rider, a pusher
walking into a row of light, heavy and compound boxes, and a stander loading a dynamic crate;
riders are put back on their platforms every 480 ticks so riding and crushing recur. CSV schema
3 adds the coupling-preparation stage and per-tick motor support, blocked and unsafe counts. The
[telemetry guide](physics-telemetry.md#extended-native-workloads) owns the fixture definitions.

The run: Release, seed 12345, 1/60 s ticks, four substeps, no window or rendering; for each
workload, 0, 100 and 500 dynamic bodies, requested awake and still, collection on and off,
three serial replicates interleaved in seeded order, 60 cold ticks, 300 warm-up ticks and
1,200 recorded warm ticks per run: 180 runs and 216,000 warm ticks, every raw row validated
(populations, motors, finiteness, no reconstruction while collecting). No build or test ran
alongside; the process sampler observed no build tools. Machine and toolchain are those named
under [Baseline and what ran](#baseline-and-what-ran), with MSVC 19.51.36260.0.

Warm ticks with collection on; each value is the median of the three runs' nearest-rank
percentiles, and worst is the worst tick of the three (milliseconds per World tick):

| Workload | Bodies | Awake p50 / p99 / worst | Still p50 / p99 / worst |
|---|---|---|---|
| sparse | 500 | 0.120 / 0.198 / 0.425 | 0.088 / 0.155 / 0.304 |
| compound | 500 | 0.153 / 0.273 / 0.512 | 0.112 / 0.229 / 0.259 |
| stack | 500 | 0.319 / 0.410 / 0.627 | 0.090 / 0.271 / 0.384 |
| sensors | 500 | 0.477 / 0.595 / 0.742 | 0.324 / 0.440 / 0.526 |
| coupled | 0 | 0.118 / 0.188 / 0.218 | 0.097 / 0.132 / 0.220 |
| coupled | 100 | 0.154 / 0.219 / 0.369 | 0.127 / 0.198 / 0.304 |
| coupled | 500 | 0.289 / 0.394 / 0.539 | 0.253 / 0.378 / 0.526 |

- The 30 coupled motors alone cost about 0.12 ms per tick: preparation 0.046 ms and finish
  0.061 ms (stage means, awake). Adding 500 mostly resting boxes adds about 0.17 ms; the
  backend solve stays near 0.006 ms because most of them sleep. Awake runs report the three
  crushed riders as unsafe and recover them each cycle; still runs report none.
- A smaller active set measurably reduces work: the 500-box stack drops from 0.319 to 0.090 ms
  when left still. Still bodies keep their per-body synchronization and publication scans,
  which is why sparse still runs cost 0.088 ms rather than nothing.
- Cold ticks include backend initialization: their p99 reaches 3.4 ms and their worst tick
  3.6 ms (500-box stack). They are reported separately and never mixed into warm figures.
- Collection on versus off changes warm p50 by at most ±0.009 ms. This is the enabled
  collection cost; compiled-but-disabled instrumentation was not measured again, and its
  2026-10-26 reference-build estimate belongs to the statically linked engine.

The same 180-run measurement ran again after the owner's first review (slope and obstruction
fixes, working tree on `b46b7d0`). Every workload, including those without characters, measured
slower and with longer tails: warm p50 for 500 bodies awake rose from 0.120 to 0.133 ms
(sparse), 0.477 to 0.519 ms (sensors) and 0.289 to 0.323 ms (coupled; p99 0.662, worst 1.278).
With 0 bodies, coupled rose from 0.118 to 0.125 ms. The process sampler saw no build tool in
either run, so the uniform shift is unattributed background activity on the machine, not a
measured cost of the fixes; neither run is a budget.

Every row, per-run summary, the full warm and cold table, machine metadata, source and binary
hashes, and process samples, for both runs, are retained under
`.local/validation/physics-completion-2026-10-30/workloads/`. The figures describe this
machine and these fixtures only; they are not a gameplay, rendering or universal budget.

## Final matrix

Run serially on 2026-10-30 after all code changes, from the working tree on `7e49d4e`:

| Step | Debug | Release |
|---|---|---|
| vs2026 build, no warnings | Pass | Pass |
| vs2026 CTest | 22/22 (270.3 s) | 22/22 (233.0 s) |
| Runtime tests | 238, 0 failed checks | 238, 0 failed checks |
| `qualify_mcp.py`, vs2026 (with the cold code project) | 177 cases | 177 cases |
| ci (Ninja) build | Pass | Pass |
| ci CTest | 22/22 (222.2 s) | 22/22 (186.9 s) |
| `qualify_mcp.py`, ci (`--skip-cold`) | 170 cases | 170 cases |

After the owner's first review the slope, carry-obstruction and room changes were made and the
same matrix ran again, serially, from the working tree on `b46b7d0`: vs2026 CTest 22/22 (Debug
275.4 s, Release 244.2 s), 241 runtime tests with no failed check in both, MCP 177 cases each;
ci CTest 22/22 (224.8 s, 190.4 s) and MCP 170 cases each; the editor built with no warnings.

The 22 CTest suites are the 20 of the baseline plus `coupling-lab-module` and
`coupling-lab-project`; the MCP counts include the 42 `editor-mcp-physics` cases. The editor
built in Release with no warnings, `scripts/check_docs.py` checked 167 documents with no
problem, and `git diff --check` was clean. The 2026-10-26 D4 review room also opened and ran
60 ticks in the current Release `pixie-player`, with its scene unchanged.

## Residual limits

- Owner reviews are pending: movement feel (PHY-013 to PHY-015), pointer use of the editor
  (PHY-019), debug-view readability (PHY-020) and interpolated rotation (PHY-016). The coupling
  lab and the captures prepare them; nothing here replaces them.
- Character/character collision, motor trigger events, joints and arbitrary-speed continuous
  character/body collision are outside this increment. Unsafe placement reports failure; damage,
  reversal and respawn are game policy.
- No universal no-tunneling guarantee, particularly for moving sensors; the fast-body cases
  qualify only their declared envelope.
- Engine and backend allocations are not observed by the benchmark after the move to
  `pixie.dll`; no zero-allocation claim is made.
- Sparse and mixed headless timings describe this machine and these fixtures, not a gameplay or
  rendering budget.
- Programmatic MCP gestures do not establish pointer usability; the editor's diagnostics text is
  observed in captures, not asserted. The editor workflow opens a legacy scene but does not save
  it again; preserving unknown fields on save is covered by the native migration and record-list
  tests, not by the editor run.
