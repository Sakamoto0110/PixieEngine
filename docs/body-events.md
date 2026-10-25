# Contact Events, Sensors and Dynamics Qualification

**Document ID:** RUNTIME-BODY-EVENTS-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** body pair lifetime, sensor queries and bounded material/fast-body qualification

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Events and ownership

[CollisionWorld2D.hpp](../runtime/include/pixie/runtime/CollisionWorld2D.hpp)
owns the public event/query API. Contact pairs represent blocking solid shapes;
sensor pairs represent nonblocking overlap. Each event copies two shape/body
identities, kind, reason, physics step and service generation into engine storage.
No backend pointers or temporary Box2D event arrays are returned.

Call StepBodies once per fixed tick, then consume Events(). Its span remains
valid until the next StepBodies or Reset. RemoveBody, SetBodyEnabled(false),
SetCollisionFilter and TeleportBody queue pair ends without modifying the last
published span. The next valid step publishes those ends before fresh begins.
Unchanged filters are a no-op. End reasons are:

| Reason | Meaning |
|---|---|
| Separated | Native contact/overlap ended through simulation |
| Removed | Body/shape was removed |
| Disabled | Body or authored collider was disabled |
| Reconfigured | Scene geometry/properties or motion owner required replacement |
| FilterChanged | Collision membership/mask changed |
| Teleported | Explicit native transform/teleport invalidated the pair |
| Reset | Service reset closed the old generation |

Reset publishes old-generation ends immediately, removes bodies and starts a
new generation/step counter. Destruction of an entire World invalidates spans;
it does not forward events to a replacement World. Consumers must clear their
own active-pair state at that lifecycle boundary. A copied identity is not a
promise its body or entity is still alive: check Contains/ContainsBody before
using it for commands. Backend IDs reused during removal cannot revive engine
identities.

Solid contact endpoints sort by engine shape ID. Sensor events keep the sensor
in endpoint a and visitor in b. Two sensors can produce two directed overlaps.
Compound visitors produce individual shape pairs; no implicit entity aggregation
or per-frame Stay event is supplied. A game can aggregate deliberately using
both shape IDs and generation. Contact begin/end is not an impact-force event;
this API does not offer hit impulses or thresholded impact sounds yet.

## Sensors and filtering

BodyShape2D.sensor and authored Collider2D.sensor make geometry nonblocking.
Sensors contribute zero mass/center/inertia regardless of authored density.
A dynamic body still requires valid non-sensor geometry with positive mass;
a sensor-only region can use an implicit static collider or kinematic body.
Sensor visitors include static, kinematic and dynamic shapes when both layer
masks permit them. Shapes on the same body do not produce overlap events.

RayCast and OverlapBox exclude sensors by default; includeSensors explicitly opts
in. Both directions of the layer filter apply. OverlapBox tests actual geometry,
not only its broadphase bounds, and returns shape IDs sorted by engine identity.
Ray hits identify whether their shape is a sensor. MoveCapsule always ignores
sensors for casting, depenetration, velocity clipping and grounding.

The pinned backend mover convenience functions cannot exclude sensors directly.
The adapter retains their normal fast path in worlds without sensors. With
sensors, it uses the native swept AABB tree query, skips sensor candidates and
casts the capsule against solid geometry with the same encroachment policy.
Plane callbacks also skip sensors. This uses public backend APIs; vendored source
is unchanged. Runtime tests compare the resulting capsule path and grounding to
the same solid scene without sensors.

Sensor detection is discrete at the outer physics step. A fast visitor can pass
completely through a narrow sensor between samples; continuous body collision
does not promise swept sensor events. Use an explicitly sensor-inclusive cast
when gameplay requires detection along a fast path.

## Scene and editor use

Enable sensor in the Collider2D inspector for an authored region. The existing
generated field travels through the normal C API metadata; no game-specific
simulation bridge or new C ABI export is needed. Solid/sensor-specific creation
presets, colors and event/center-of-mass debug views remain authoring work.

ScenePhysicsEvents2D(World&) exposes the last completed scene tick. Each copied
event retains entity identities, including an entity destroyed before publication.
Native lookup tables are retired only after copying those endpoints. Filter or
geometry edits close old pairs before publishing begins for replacement shapes.
Invalid/duplicate world settings queue Reset ends even if the service cannot
continue stepping. A later scene tick clears the published list, including when
no remaining body requires a backend step. Edit synchronization never advances
time or invents overlap begins.

Gameplay Update reads the previous completed tick, before the next PostUpdate
publication. Structural changes use World's normal deferral rules. Sensors only
see actual physics shapes: the query-driven CharacterMotor2D capsule is not yet
a sensor visitor. Player-trigger integration belongs to the pending motor/body
coupling stage, together with pushing and platform support.

## Qualified cases and limits

The fixtures in [BodyQualificationTests.cpp](../tests/runtime/BodyQualificationTests.cpp)
use 32 units/m, a 1/60 s outer step and four backend solver substeps. They are
bounded scenarios, not a Cartesian guarantee over every shape/speed/mass combination.

| Fixture | Checks |
|---|---|
| Ten aligned 32×32 boxes, 60 simulated seconds | Unit density and 10× bottom density; settled penetration ≤0.5 units, last-second vertical drift ≤0.25 units and angle variation ≤0.5 degrees; all sleep, then an impulse wakes them |
| Same 0.25-radian slope | Friction 0 slides; friction 1 holds; body rotation locked to isolate friction |
| Falling circle | Restitution 0 settles and restitution 1 bounces; low-speed contact below the default bounce threshold settles |
| Fast 8×8 box against an 8-unit wall | Speeds 64/256/512/1024 units/s, mass overrides 0.1/1/1000 kg, static wall or kinematic wall moving toward it at 128 units/s; every case reaches contact, never crosses the wall, and penetration recovers to ≤0.5 units within 0.5 seconds |
| Free rotation | 8 radians/s remains within the declared angular integration tolerance |

The fast-body test distinguishes initial soft-contact overlap from tunneling and
settled penetration. The measured maximum initial overlap was approximately
**2.4 world units** in the approaching-wall case; the center remained outside
the wall and overlap recovered. This is a visible precision limitation at that
small geometry scale, not a claim of subpixel collision throughout the impact.
The 0.5-unit settled tolerance is not applied retroactively to that first frame.
Rotating impacts, arbitrary corners, greater speeds and extreme interacting mass
ratios are not qualified by these axis-aligned fast-body fixtures.

The backend's default low-speed restitution threshold is 1 m/s (32 units/s at
the default scale). Surface friction mixes by geometric mean and restitution by
maximum. Continuous enables the pinned backend's bullet flag with its default
activation policy; no dependency patch or hidden multi-step accumulator is used.

## Regression evidence and telemetry

[PhysicsEventsTests.cpp](../tests/runtime/PhysicsEventsTests.cpp) covers pair
lifetime, directional sensors, compounds, deletion/reused entities, disable,
filter edits, geometry replacement, teleport, reset, query inclusion, motor
nonblocking and serialization. A test-only omitted-sensor fixture must fail;
production code does not inspect its environment flag. The C API body-authoring
fixture saves/reopens a sensor and verifies a falling/spinning body passes through
it during Play, then restores the authored pose at Stop.

Telemetry separates physics.solve from physics.events.collect and reports
bodies, shapes, sensors, awake bodies, active contact/sensor pairs and published
event count. The stable path collects native begin/end arrays rather than
rescanning all body pairs. Full stress budgets, synchronization/coupling timing,
allocation measurements and owner movement-feel acceptance remain later gates.
Machine-local verification is retained under `.local/validation/body-events-2026-10-22/`;
portable regression inputs are the source fixtures linked above.
