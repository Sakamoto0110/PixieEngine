# Public API map

**Document ID:** GLOBAL-CONSUMER-API-MAP
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** finding public SDK declarations by consumer task
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

[Handbook](README.md)

Paths below are relative to the selected SDK's **include/** directory. Read the
actual installed declaration before using an overload not shown in the handbook.
The headers own signatures and contracts; this map is a navigation aid, not an
independent ABI specification. A public header can expose host-integration or
advanced services without making them necessary for ordinary game code.

## Start here

| Task | Header | Entry points / responsibility |
|---|---|---|
| Supply the game | `pixie/runtime/Project.hpp`, `SceneProject.hpp` | Project hooks, CreateProject; generated code supplies the player/module entry |
| Create/iterate objects | `pixie/runtime/World.hpp`, `Entity.hpp` | Create, Each, Add, TryGet, AddBehaviour, Defer; World.hpp defines component templates |
| Per-object behavior | `pixie/runtime/Behaviour.hpp` | Start, Update, Stop and ordered input callbacks |
| Expose and save fields | `pixie/runtime/ComponentTypes.hpp` | Describe registration, Field metadata, supported serialization kinds |
| Pose and simple motion | `pixie/runtime/Transform2D.hpp`, `Velocity2D.hpp`, `Acceleration2D.hpp` | Position/rotation/scale, one built-in motion integrator |
| Input | `pixie/platform/Input.hpp`, `pixie/runtime/InputControl.hpp` | Key/MouseButton, held/edge polling, events, KeyboardControl/MouseControl |
| Time and reproducible randomness | `pixie/core/FrameTime.hpp`, `pixie/runtime/Random.hpp` | delta in seconds; World::GetRandom owns per-World generator |
| Authored identities/content | `pixie/runtime/Scene.hpp`, `ProjectFile.hpp` | SceneId, AddToScene, scene read/write, project manifest |
| Player save location | `pixie/runtime/UserData.hpp` | UserDataFolder; your game defines its save format |

For a first game, use the Editor-generated Project/Behaviour path. Do not
reimplement the window, GL context, host loop or module loader to consume it.

## Graphics and presentation

| Task | Header | Entry points / responsibility |
|---|---|---|
| Primitive shapes | `pixie/runtime/Shape.hpp` | ShapeKind, size/color/visibility |
| PNGs | `pixie/runtime/Sprite.hpp`, `SpriteImages.hpp` | Sprite data and project-relative image cache |
| Fonts and world labels | `pixie/runtime/Text.hpp`, `Fonts.hpp` | Text data and font cache |
| Lines and progress paths | `pixie/runtime/Path.hpp` | Path points, width and fill |
| Camera conversion | `pixie/render/2d/Camera2D.hpp` | ScreenToWorld, WorldToScreen, Viewport and camera state |
| Authored camera selection | `pixie/runtime/ViewportCamera2D.hpp` | Free/World/Scene camera contracts; selection is not a second render window |
| Player HUD | `pixie/runtime/ScreenCanvas.hpp` | Rect, Circle, Ring, Shape, Sprite, Path, Text, MeasureText |
| World lights/materials | `pixie/runtime/Lighting2D.hpp` | Light2D, Emission2D, receiver/occluder/normal-map data, legacy settings |
| World/Scene environment | `pixie/runtime/Presentation2D.hpp`, `LightingEnvironment2D.hpp` | Canonical presentation/resolution; observe scene-version limits |
| Extra world rendering | `pixie/render/2d/ShapeRenderer.hpp`, `ShapeInstance.hpp`, `PathRenderer.hpp` | DrawOverlay integrations |
| Low-level render surface | `pixie/render/View.hpp`, `RenderTarget.hpp`, `Screenshot.hpp` | Host/render integration; not normal component authoring |

Use [drawing and controls](drawing-and-controls.md) for coordinate conventions,
the complete HUD helper and distinctions between emission and illumination.

## Physics and effects

| Task | Header | Entry points / responsibility |
|---|---|---|
| Scene colliders/motors | `pixie/runtime/SceneMovement2D.hpp` | Collider2D, CompoundCollider2D, CharacterMotor2D, CharacterCommand2D, scene physics service/events |
| Rigid-body settings | `pixie/runtime/RigidBody2D.hpp` | RigidBody2D, body types, PhysicsSettings2D |
| Body commands and queries | `pixie/runtime/CollisionWorld2D.hpp` | Body handles, forces/velocity/teleport, RayCast, OverlapBox, contact manifolds |
| Standalone character algorithm | `pixie/runtime/CharacterController2D.hpp` | Lower-level character integration; normal scenes use CharacterMotor2D |
| Diagnostic geometry | `pixie/runtime/PhysicsDebug2D.hpp`, `ColliderEditing2D.hpp` | Presentation/edit helpers; not game rules |
| Emit short-lived visuals | `pixie/runtime/ParticleSystem.hpp` | Bounded particle pool; Emit/EmitDynamic |
| Continuous emitters | `pixie/runtime/ParticleSpawner.hpp` | Rates, ranges and target system |
| Update particles | `pixie/runtime/ParticleStep.hpp` | AddParticleStep, diagnostics; normal sessions already install it |
| Colliding particles | `pixie/runtime/ParticlePhysics2D.hpp` | Per-pool physics configuration |
| Affect particle motion | `pixie/runtime/ParticleAffector.hpp`, `AttractorAffector.hpp`, `InteractionAffector.hpp` | Affector interface and supported implementations |
| Spatial helpers | `pixie/runtime/SpatialGrid.hpp`, `SpatialIndex.hpp`, `Picking.hpp` | Queries/picking; inspect precise coordinate/lifetime contracts |

Scene services own borrowed data. Reacquire after sync/reset and never keep a
physics handle across a new World. Gameplay owns how contacts become damage,
score, object removal or cosmetic feedback.

## Advanced integration and diagnostics

| Task | Header / surface | Responsibility |
|---|---|---|
| Host a Project | `pixie/runtime/ProjectSession.hpp`, `ProjectApp.hpp` | Session lifecycle and host integration |
| Native player loop | `pixie/app/App.hpp`, `Run.hpp`, `RunConfig.hpp`, `RunOptions.hpp` | Window/input/run composition; generated player normally supplies this |
| Logs and timings | `pixie/core/Log.hpp`, `Telemetry.hpp` | Log messages and CPU metrics; not a GPU timing guarantee |
| External language/tool bridge | SDK C API headers and `bindings/` | Versioned native ABI; use matching SDK-generated bindings |
| CMake consumption | SDK `cmake/PixieProject.cmake`, `lib/cmake/PixieEngine/` | pixie_game or imported targets; no Engine checkout dependency |
| Compatibility identity | SDK `sdk.json` and selected `profiles/*/*/profile.json` | Version/build/toolchain/profile/feature/dependency identity |

The public SDK is sufficient for consumer builds. Private channels, producer
scripts, source-tree internals and local release rehearsal artifacts are not
consumer requirements. A consumer handbook cannot grant compatibility to a
mixed set of DLLs from different SDK installations.

## How to resolve an unknown API

1. Identify the responsibility above and open the installed header.
2. Confirm the namespace, argument types, return value and lifetime contract.
3. Follow the existing generated Project/registry integration rather than
   assuming another engine's method names.
4. Compile the smallest use in the game against the selected SDK.
5. Observe the feature through the Editor and later the exported player.
6. If no supported API exists, report the gap and choose a game-side alternative
   within scope. Do not patch Engine/Editor/SDK to make a guessed API exist.

The Engine's deeper [runtime reference](../modules/runtime/REFERENCE.md) and
[module index](../README.md#native-modules-and-api) explain additional contracts.
Some historical delivery sections discuss older ABI revisions; they do not
override the installed package's identity.
