# Gameplay in C++

**Document ID:** GLOBAL-CONSUMER-GAMEPLAY
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** public runtime lifecycle and game component authoring
**Surface:** guide
**Boundary:** runtime
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

[Handbook](README.md) · [API map](api-map.md)

## The code you own

Use **Code > Add code** in an open project. It generates a CMake project,
`src/<Id>Project.hpp`, `src/<Id>Project.cpp`, a reusable component registry and
the example `Spinner` Behaviour. `pixie::CreateProject()` constructs your
`Project`; keep exactly one definition in the game.

Use **Inspector > Add component > Create a C++ component** to generate a new
type. The name is an ASCII C++ identifier beginning with a letter, at most 64
characters. The generated `src/components/<Name>.hpp` and `.cpp` are yours to
edit. The registry registers their `Describe` methods and compiles their sources.

Do not hand-edit `PixieComponents.generated.*`, `.pixie-code/Components.cmake`,
their recorded hashes or the template-owned CMake text to add another component.
The automatic build/generation path checks ownership and refuses custom wiring
it cannot safely recognize. An advanced custom CMake project is possible, but
is not interchangeable with the Editor-managed path.

The public API is provided by headers under the selected SDK's `include/`.
Include `pixie/runtime/World.hpp` when using `Entity`'s component templates.
Use `pixie::` for Engine types; PixieLib value types such as `pxPoint`, `pxVec2`
and `pxColorRgba` have the spellings shown here without that namespace prefix.

## Project hooks

| Hook | What belongs here |
|---|---|
| `Name() const` | A stable display name for the game |
| `Describe(ComponentTypes&) const` | Register custom component names, fields and metadata; built-in types are already supplied |
| `Build(World&, Camera2D&)` | Add systems and runtime-generated objects after the scene's authored entities have loaded |
| `BeforeStep(World&, Camera2D&, const Input&)` | Game-wide input, camera policy and structural work outside the World step |
| `Capture(World&)` | Explicitly carry selected game-owned configuration into a later rebuild when appropriate; not automatic save-game persistence |
| `DrawOverlay(World&, ShapeRenderer&, const Camera2D&)` | Extra world-space visuals |
| `DrawScreen(World&, ScreenCanvas&)` | Screen-space HUD, panels, labels and indicators |
| `SpawnAt(World&, pxPoint)` | Optional response to the Editor's request to create something at a world point |

`SceneProject` implements middle-drag/wheel camera navigation in `BeforeStep`.
If you override that hook, call its base implementation only if you want those
controls. `DrawScreen` runs in Edit as well as Play: keep rendering free of
game-state mutation. `Build` also participates in reconstruction; do not use it
to send messages, spend currency, write external saves or perform other
irreversible game actions.

## World, Entity and lifetime

```cpp
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Scene.hpp"

void AddMarker(pixie::World& world) { // Called outside a step, for example in Build.
    const auto marker = world.Create("marker");
    marker.Add<pixie::Transform2D>().position = {100.0, 80.0};
    auto& shape = marker.Add<pixie::Shape>();
    shape.kind = pixie::ShapeKind::Quad;
    shape.size = {12.0, 8.0}; // Half extents: the rectangle is 24 by 16.
    shape.color = {100, 190, 255};
    pixie::AddToScene(marker); // Only if this is authored content you intend to save.
}
```

Do not call that function unconditionally on every rebuild if the marker is
already saved: locate the authored entity instead, or keep it generated and
omit `AddToScene`. A matching name is not a unique identity. `SceneEntityWith<T>`
is useful for a known singleton-like authored configuration component, not a
general unique-name lookup.

| API | Rule |
|---|---|
| `world.Create(name)` | Creates a live entity; does not automatically make it persistent |
| `entity.Add<T>(...)` | Adds or replaces a data component |
| `entity.AddBehaviour<T>(...)` | Adds a Behaviour and registers its lifecycle |
| `entity.Has<T>()` / `TryGet<T>()` | Check optional data before using it |
| `entity.Get<T>()` | Use only when the entity is alive and that component exists |
| `world.Each<A, B>(fn)` | Iterate entities having all listed components; callback receives `(Entity, A&, B&)` |
| `entity.Destroy()` | During a step, destruction occurs at its end |
| `world.Defer(fn)` | Apply structural changes at the end of a step, or immediately outside a step |

Store handles, not component/Behaviour pointers. Pools may move components when
they grow; reconstructing the World invalidates handles to the old World too.
`Entity::Number()` is a runtime index/version value, not a persistent save ID.
For persisted references use scene identities and the scene API. Resolve them
again after Play/Stop, reset, reopen, or candidate reconstruction.

Callbacks and iteration must not directly create entities or add/remove
components while a step is running. For example:

```cpp
const auto source = Self(); // In a Behaviour callback.
GetWorld().Defer([source](pixie::World& world) {
    if (!source.IsAlive()) return;
    const auto created = world.Create("runtime marker");
    created.Add<pixie::Transform2D>();
    // Runtime-only: do not add SceneId unless explicitly authoring content.
});
```

Capture required values by value. Never capture a temporary event reference or
`this` as a long-lived Behaviour pointer. Keep World operations on one thread.

## Updates and input

The World executes `PreUpdate`, Behaviours, `Physics`, `PostUpdate`, then
deferred structural work. See the [World step contract](../modules/runtime/REFERENCE.md#the-step) for parenting refreshes, deferred work and failure behavior.
Use `FrameTime::delta` in seconds for per-second
changes. Normal sessions use fixed simulation steps; drawing can happen at a
different rate. Never multiply by render FPS to obtain game motion.

A Behaviour has `Start`, `Update`, `Stop`, `OnKey`, `OnMouseButton`,
`OnMouseMove`, and `OnMouseScroll`. `Start` runs once before its first callbacks
and Update; it runs **inside** a step. The [Behaviour contract](../modules/runtime/REFERENCE.md#behaviours) covers instance order, device admission and removal. `Stop` runs if Start ran, when removing
the Behaviour/entity or tearing down the World. A member named `enabled` in
your Behaviour is your own flag: check it in your callbacks.

Attach `KeyboardControl` and/or `MouseControl` to opt an entity into device
callbacks. Their enabled flags also gate their polling helpers:

```cpp
if (const auto* keyboard = Self().TryGet<pixie::KeyboardControl>()) {
    const bool held = keyboard->IsDown(input, pixie::Key::D);
    const bool pressed = keyboard->WasPressed(input, pixie::Key::Space);
    // held is for sustained control; pressed is for a once-per-press action.
}
```

`Input::Events()` preserves event order. A `KeyEvent` carries `key`, `action`,
`modifiers`, `scanCode` and `Pressed()/Released()/Repeat()`. Mouse positions are
viewport framebuffer pixels, origin top-left, Y down; use `Camera2D::ScreenToWorld`
when a world point is needed. Editor input requires the viewport's Game-input
mode/focus policy. A focused text field or modal can consume/cancel input.

The host calls `Input::EndUpdate()` after each simulation update. Game code
must not consume the shared input halfway through the World step. Polling
describes the final accumulated state; individual callbacks describe transitions.
`KeyboardControl` provides no built-in meaning for a key. The older
`KeyboardControl2D` is a distinct legacy movement adapter, not the generic type.

List the game's actual bindings before testing input. An on-screen Left/Right
button does not imply that the keyboard arrows or A/D are bound. If a synthetic
keypress has no visible effect, check the binding, Play/Game-input state and
viewport focus before diagnosing the engine. Compare a manual keypress when
available and report the two results separately; an injection failure alone
does not establish a keyboard defect.

## Complete example: free top-down movement

Create a C++ component named **PlayerController** through the Editor. Replace
its two user-owned files with the complete sources supplied here:

- [PlayerController.hpp](examples/PlayerController.hpp)
- [PlayerController.cpp](examples/PlayerController.cpp)

The `.Source("src/components/PlayerController.cpp")` metadata matches where
the Editor generates it. Keep the generated registry unchanged.

On one scene entity attach `Transform2D`, a visible `Shape` or `Sprite`,
`Velocity2D`, `KeyboardControl` and `PlayerController`. Remove `Spinner` if you
do not want rotation. Build and reopen, save, then Play and use WASD in the
viewport. Expected behavior: movement at the configured speed, normalized
diagonals and immediate stop when input is released or disabled. Stop returns
to the authored pose.

This example deliberately implements free movement: `Velocity2D` alone does
not collide. Do not also add `RigidBody2D` or `CharacterMotor2D` and expect this
velocity writer to control those owners. Do not move `Transform2D` manually as
well; the World already integrates the velocity once per step.

## Optional example: a collision-aware platform character

Create **PlatformerController** and use its supplied
[header](examples/PlatformerController.hpp) and
[implementation](examples/PlatformerController.cpp). Attach it to the
**Character motor starter** preset. That preset supplies Transform, visible
Shape, CharacterMotor2D and generic KeyboardControl. The example creates its
runtime-only `CharacterCommand2D` through deferral after the first tick; you do
not add that command through the Inspector.

Add a static collider floor below the character. Use A/D and Space in Play.
The controller writes horizontal intent and a jump edge; `CharacterMotor2D`
owns movement and collision. Tune speed, acceleration and jump settings on the
motor, not in a second integrator. Keep its transform upright at unit scale;
edit motor dimensions rather than applying a rotated/scaled transform.

## Reflection and state

```cpp
types.Add<MyComponent>("MyComponent")
    .Source("src/components/MyComponent.cpp")
    .Field<&MyComponent::enabled>("enabled").Tooltip("Enable this game feature.")
    .Field<&MyComponent::speed>("speed").Range(0.0, 1000.0).Scrub(1.0);
```

Register with `types.Add<T>` in `Describe`, and keep the generated
`pixie::generated::DescribeComponents(types)` call. Registered fields become
Inspector controls and scene serialization data. A C++ member not described
does not magically appear in the Editor or persist in a scene.

After adding a reflected field, an older scene may have no stored value for it
and use its C++ default. Building new code is not a migration of every saved
scene. In the observed consumer workflow, saving an unchanged scene did not
materialize newly added fields. To author a specific value, edit it in the
Inspector, save, reopen and verify it. Distinguish a serialized setting from a
default when comparing source, scene and export.

Supported field kinds include booleans, supported numeric values/enums,
points/vectors/rectangles/colors, entity references, strings, point lists and
registered record lists. Unsupported types need an explicit representation;
do not assume an arbitrary container or pointer can be reflected.

Use `Degrees()` to display a stored radian value in degrees; `Unit()` similarly
changes presentation. A field's name is part of saved data: keep it stable.
`Version()` and `Upgrade()` support explicit field evolution. `.ReadOnly()`
changes editing metadata, not persistence. `.Asset("png")` marks an asset
path; it does not guarantee the exporter can discover custom code-owned assets.

Keep authored tuning separate from transient state. For example, expose a
speed limit or cooldown duration; calculate the current animation phase,
timers or mode state in game code. Decide separately what a player's save
should retain. Per-instance component state supports independently behaving
objects; a global/static variable would make them share it unintentionally.

## Persistence and rebuilding

Only entities with a nonzero `SceneId` belong to saved scene content, and their
registered fields are serialized. `AddToScene(entity)` assigns that identity
outside a step. Generated objects without it are reconstructed by `Build`.
`WriteScene`/`SaveSceneFile` report failures; check the returned result. Unknown
serialized components/fields are preserved, but cannot execute while their
code is unavailable.

`UserDataFolder(projectName)` locates a writable per-game data directory for
your own save system. The authoring scene and the player's runtime save are
different things. Saving in the Editor does not persist arbitrary C++ member
state, physics handles, component pointers, or live particle pools.

**Build and reopen** replaces the process/module. It can carry unsaved authored
content and selection via a private handoff; Undo/Redo history and arbitrary
mutable Project state do not survive recompilation. Saving first is the
simplest recovery point. An uncompiled source change cannot affect the
already-loaded DLL.

## Scene versions and custom code

New ordinary projects begin as v1. Keep that format for the Behaviour examples
and a first game with custom Behaviour components.

v2 introduces logical Scene scopes and candidate-based whole-document edits.
Custom module participation is explicit (`CANDIDATE_AUTHORING`), and custom
authored fields require the restricted `AddData<T>` declarative contract.
Ordinary authored Behaviour registrations are not supported v2 payloads in
this baseline. Turning on a CMake option alone does not make them admissible.

Conversion is an explicit **Convert and save** operation with a `.v1-backup`;
ordinary save does not silently upgrade the format. Do not delete unknown
components, switch off checks, or edit version numbers to force a refused
scene to load. The existing scene, backup and diagnostic are the recovery inputs. Logical Scene
membership groups entities and policy independently of visibility and parenting.

v3 stores authored parent links and attached child-local poses in the unscoped
family. A change to v3 requires a new destination. v1 and v2 projects can
continue in their existing formats. The [scene-family contract](../modules/runtime/REFERENCE.md#scene-families)
covers admission and the [parenting contract](../modules/runtime/REFERENCE.md#2d-parenting)
covers pose and component restrictions.

For exact declarations see the SDK headers and
[runtime reference](../modules/runtime/REFERENCE.md). Older delivery sections
may describe their historical ABI; the selected SDK identity/header owns the
ABI you compile against.
