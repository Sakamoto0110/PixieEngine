# Drawing, controls and effects

**Document ID:** GLOBAL-CONSUMER-DRAWING
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** public graphics, screen controls, collision feedback and particles
**Surface:** guide
**Boundary:** runtime
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

[Handbook](README.md) · [Gameplay](gameplay.md) · [API map](api-map.md)

## Pick a space before drawing

| Content | Use | Units and origin |
|---|---|---|
| Objects in the level | Transform2D with Shape, Sprite, Text or Path | World units, +Y down |
| Camera | Camera2D / authored viewport camera | Position in world units; zoom is pixels per world unit |
| HUD, buttons, status lights | Project::DrawScreen and ScreenCanvas | Viewport framebuffer pixels, top-left origin |
| World annotations | Project::DrawOverlay or canvas.WorldToScreen | Explicit world-to-screen conversion |

`Transform2D::rotation` is **radians**. Positive rotation turns +X toward +Y,
which appears clockwise in the usual Y-down view. The Inspector displays degrees.
The generated Spinner is a complete simple rotation example: change rotation by
`speed * time.delta`. Use either that explicit pose writer, `Velocity2D::angular`,
or a physics body as the motion owner; do not make several of them compete.

`Shape::size` is half extents. A Quad of size `{30, 10}` is 60 by 20 world units
before Transform scale. Available kinds are Triangle (points along +X), Circle,
Ring and Quad. Colors use byte channels, 0 to 255. `visible` controls drawing,
not simulation, collision or entity lifetime.

Sprites name project-relative PNG files; `region` selects source pixels,
`pivot` is normalized and Transform supplies placement/rotation/scale. Use
forward-slash relative paths such as `assets/indicator.png`, not a developer's
Desktop path. Text requires a readable font file; there is no automatic default
game font. Keep font licensing with the game, including the
[exported folder](export-and-troubleshooting.md#assets-and-export-limits). Native text/sprite drawing is not
an HTML or ImGui widget system.

Use [2D parenting](../modules/runtime/REFERENCE.md#2d-parenting) for supported
hierarchy compositions. Authored links use v3 in the unscoped family. For a
portable visual composition in a flat scene, keep offsets in game-owned data
and derive world poses from a common pose. Use stable scene identity to resolve
a new handle after reconstruction. [World, Entity and lifetime](gameplay.md#world-entity-and-lifetime)

## A lower control panel

HUD buttons are game-owned hit rectangles and input state, not physics triggers
or built-in Editor widgets. Use pointer containment plus press/release or held
state for a clickable/held control; a visual indicator needs no click handler.
Keep this separate from world-space sensor overlap events.

Use one shared layout function for drawing and hit testing. Do not use desktop
screen coordinates or include the Editor's menu/panel offsets: `Input::Cursor()`
and `ScreenCanvas` use the same viewport-local framebuffer coordinates.

The complete [ControlPanel.hpp](examples/ControlPanel.hpp) draws two held-direction
buttons and one toggle/indicator without external assets. This is a plain C++
helper, **not** a component to register. Copy it to your game's `src/` and integrate
it into the generated Project class:

```cpp
// In the Project header, include "ControlPanel.hpp" and add these members.
ControlPanel panel_;
void BeforeStep(pixie::World&, pixie::Camera2D&, const pixie::Input&) override;
void DrawScreen(pixie::World&, pixie::ScreenCanvas&) override;
```

```cpp
// Replace MyGameProject with your actual generated class name.
void MyGameProject::BeforeStep(pixie::World&, pixie::Camera2D& camera,
                               const pixie::Input& input) {
    const auto size = camera.Viewport();
    panel_.Read(input, {double(size.width), double(size.height)});
    // Publish panel_.direction / panel_.active to your game's own state here.
}
void MyGameProject::DrawScreen(pixie::World&, pixie::ScreenCanvas& canvas) {
    panel_.Draw(canvas);
}
```

Preserve the generated Describe, Build and CreateProject definitions. This
override intentionally owns camera/input policy; call `SceneProject::BeforeStep`
only if you also want its middle-drag/wheel navigation. The helper uses current
viewport dimensions each time, so resizing does not leave stale hit rectangles.
It supplies input intent, not movement, game rules or persistent save data.
Store persistent/transient game state according to [the state guidance](gameplay.md#reflection-and-state);
plain Project members are not automatically restored by Stop or saved to scenes.

The opaque lower panel covers part of the view; it does **not** create a second
viewport or clip the world simulation. Keep game objects within the intended
play area through the game's camera/bounds policy. Do not draw the entire game
as HUD just to avoid using scene entities.

For a label, `canvas.Text("Ready", {20, 20}, {"assets/ui.ttf", 18, {240, 240, 255}})`
uses pixels for position and font size. Missing font files draw no text and log a
diagnostic. `MeasureText` measures that same style. Shapes are submitted before
paths, then sprites/text together: do not rely on arbitrary cross-category call
order as a layering API. Built-in authored Text/Sprite asset paths are discovered
by export; code-only paths need an explicit export asset route (see [export](export-and-troubleshooting.md#assets-and-export-limits)).

## Visual indicators and world lights

A button can be only a visual indicator: derive its color from game state and
give it no click action. ScreenCanvas is outside world lighting, so a bright HUD
indicator normally uses color, rings or layered translucent shapes.

For world objects, distinguish these components:

| Component | Effect |
|---|---|
| Emission2D | Makes a Shape/Sprite/Text appear emissive; does not itself illuminate neighbors |
| Light2D | Point, Spot or Directional light illuminating eligible receivers |
| LightReceiver2D | Receiver/layer policy, including unlit content |
| Occluder2D | Shadow geometry from configured geometry/source |
| NormalMap2D | Surface normal detail for the supported material path |
| ParticleLight2D | Particle emission/illumination policy with bounded light contribution |

Lighting must be configured: adding Light2D alone to an unlit scene is not enough.
For the v1 first-game route use one `LightingSettings2D` owner, enable the lighting
path and choose ambient low enough to see a light's contribution. Inspect the
actual reflected settings rather than inventing property names. Keep masks,
receiver/occluder settings and light range consistent. A high ambient value can
hide the visible effect of a light. v2 has a separate World/Scene presentation
contract; do not add both competing settings models to work around a warning.
Use the Editor's lighting guide for handles and diagnostics. OpenGL 4.3 VM
experiments do not establish support for that renderer.

## Collision-aware objects

Choose the owner of motion first:

| Need | Components/API |
|---|---|
| Simple noncolliding movement | Transform2D + Velocity2D; World integrates it |
| Static obstacle | Transform2D + Collider2D or CompoundCollider2D |
| Dynamic/kinematic rigid body | Transform2D + RigidBody2D + one collider representation |
| Upright platform character | CharacterMotor2D + CharacterCommand2D; separate optional controller example |
| Geometric query | SceneCollisionWorld2D then RayCast/OverlapBox/etc. |

`Shape` and `Sprite` are visual components, not automatic colliders. A collider
can be a sensor; masks must permit the pair. `CompoundCollider2D` replaces the
single Collider2D on that entity. A CharacterMotor owns its capsule; do not also
attach a rigid body or independent collider to the same entity.

Normal ProjectSession hosts install scene movement. Standalone World consumers
must call `InstallSceneMovement2D`. Get `ScenePhysicsWorld2D(world)` and
`SceneBody2D(world, entity.Id())` only after synchronization has created bodies;
check the service/handle and reacquire them after reset/reconstruction. Body
commands include SetBodyVelocity, ApplyForce/Impulse and TeleportBody. Do not
step this same backend yourself inside the normal session or also overwrite
its Transform as an independent integrator.

## From collision to a visual burst

This optional recipe applies when the game uses scene physics and requests a
particle effect. It does not require every game to add colliders, physics events
or particles. A game-owned interaction can request its own cosmetic effect;
it does not become evidence that the engine's physics-event route was tested.

There is no `OnCollisionEnter` Behaviour callback. Add a named `PostUpdate`
system in Project::Build and inspect `ScenePhysicsEvents2D(world)` there. It is
the last completed scene tick; Behaviour::Update precedes Physics and sees the
previous snapshot. A record has `entityA`, `entityB` and `event.kind` (Begin/End),
`event.pair` (Contact/Sensor). Determine which game objects it represents, check
their liveness and respond only to the intended Begin event.

Events identify pairs; **they do not contain an impact position**. For a simple
cosmetic effect choose an involved live entity's Transform position. When actual
contact geometry matters, obtain `BodyContacts` from the current physics service;
those manifolds can include speculative points. Do not pretend an entity-center
effect is a measured contact point. Multiple shapes can produce multiple pairs;
deduplicate by your game object's identity if one burst per object is wanted.

Create a ParticleSystem pool outside the step (e.g. in Build). On the chosen
event, retrieve the existing pool and call Emit. For example:

```cpp
#include "pixie/runtime/ParticleSystem.hpp"
#include <cmath>
#include <numbers>

void EmitBurst(pixie::ParticleSystem& pool, pxPoint position) {
    for (int i = 0; i < 12; ++i) {
        const float angle = 2.0f * std::numbers::pi_v<float> * float(i) / 12.0f;
        const pxVec2f velocity{40.0f * std::cos(angle), 40.0f * std::sin(angle)};
        if (!pool.Emit({float(position.x), float(position.y)}, velocity, 0.4f)) break;
    }
}
```

For a complete integration, copy [ContactBursts.hpp](examples/ContactBursts.hpp)
into your game's `src/`, include it from the Project implementation and call
`InstallContactBursts(world)` once in `Build` after the base call. It creates a
runtime-only pool and a PostUpdate listener, and emits cosmetic particles for
each Begin Contact pair at entity A's center. It does not destroy objects or
implement damage. Normal sessions supply the physics and particle steps. Filter
the listener for your own game component types when only some contacts should
produce feedback; deduplicate if compound shapes need one response per object.

These are cosmetic particles in a bounded pool, not entities. The normal session
installs `AddParticleStep`; a standalone World must install it. Set a positive
capacity and finite lifetimes; lifetime zero means no age expiry. `Emit` returns
false when full. `EmitDynamic` plus ParticlePhysics2D is the distinct colliding
particle route. `ParticleSpawner` supplies continuous/ranged emission and needs
a valid target pool; it does not subscribe to collision events automatically.

Use `world.Defer` if an event response needs new entities/components; copy event
values needed by deferred work. Do not hold borrowed spans across another step
or destruction. Character proxies do not emit the same contact/sensor events.
Fast sensors can miss an overlap between sampled steps; the event API is not a
promise of swept trigger detection.

## Mapping a small game's requirements

| Requirement | Engine building block | Game-owned responsibility |
|---|---|---|
| Rotating decoration/object | Transform2D or a single motion owner | Desired angle and animation rules |
| Independent status per object | Component instances, reflected tuning | Timers, state changes, fictional heat/resource values |
| Manual/assisted control modes | Input, Behaviour/Project update hooks | Mode selection and abstract game rules |
| Buttons and status panel | ScreenCanvas + explicit hit testing | Layout, enabled state, labels and actions |
| Objects responding to hits | Physics events/queries | Which interaction counts, health and lifetime |
| Impact feedback | ParticleSystem::Emit | Cosmetic style, position and emission budget |

The handbook supplies engine usage. It does not prescribe the prototype's
mechanical model, targeting behavior, balance or full game implementation.
