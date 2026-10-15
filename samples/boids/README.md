# boids

Anchor 1 of [PIXIE-PROPOSAL-004](../../docs/proposals/scene-model-and-anchors.md): a thousand
boids that the user steers with the mouse. It is the first sample built on the `runtime` module
([`docs/modules/runtime/`](../../docs/modules/runtime/README.md)).

- Each boid is an entity with a `Transform2D`, a `Velocity2D`, a triangle `Shape`, and a `Boid`
  Behaviour. Within the pointer's radius (the ring around the cursor) it steers toward the pointer,
  or away from it, at up to `maxSpeed`, changing its velocity by at most `maxForce` per second;
  out of reach it cruises, turning a little at random.
- `Boid::Update` writes only its own velocity and heading. The World integrates velocities after
  every Behaviour has run, and the sample's `boids.wrap` system moves a boid that leaves the view to
  the opposite edge.
- The tunable values live on a `controls` entity (`BoidControls`, `Pointer`, `Bounds`), which the
  project updates from the camera and the cursor before each step.
- Space switches between attracting (green ring) and repelling (red ring). Up and Down add or
  remove 100 boids. While attracting, boids gather into a ball on the cursor; while repelling, they
  leave an empty disc around it.
- Once per second the log shows how long the World's steps took, average and slowest.

## The project

This folder is a project ([editor's README](../../editor/README.md#projects)): `boids.pixie`, and
`scenes/main.scene`, which holds the scenario and the `controls` entity. The starting values (the count) live in the
`BoidsScenario` component of the `scenario` entity: edit them in the editor and press "Rebuild the
scene", or save them. The boids themselves are generated from the scenario when the World is
built, and are not saved. A World built without the scene, as the C API test builds it, starts with
the same entities, made in code.

The scene's entities and the project's code are the sample's; its player and its C API library are
built by `pixie_game` ([runtime reference](../../docs/modules/runtime/REFERENCE.md#projects-with-code)).

## Layout

- `src/game/`: the game, built as the static library `boids-game`. `BoidsProject` is its `Project`
  (see the [runtime reference](../../docs/modules/runtime/REFERENCE.md#projects-and-sessions)):
  it finds its entities in the scene, generates the rest, handles the pointer and the keys before
  each step, and describes the game's components for tools. `pixie::CreateProject` returns it.
- `boids`: the player, the engine's, which opens this folder and plays its scene.
- `boids-capi`: the same game as a shared library with the
  [C API](../../docs/modules/capi/README.md), for hosts such as the editor.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/boids
```

The player opens `samples/boids`. To edit the scene, open the folder in the editor, which builds it
as a project of its own (into `build/` and `bin/` here, which git ignores):

```bash
editor/PixieEditor/bin/Release/net10.0/PixieEditor --project samples/boids
```

`--game boids` still opens the game from the engine's `bin/`, without the scene.

## Options

| Option | Default | Meaning |
|---|---|---|
| `--project <folder>` | this folder | The project to play |
| `--ups <hz>` | `60` | Updates per second |
| `--fps <hz>` | `0` | Frame cap; `0` means none |
| `--vsync on\|off` | `on` | Vertical sync |
| `--quit-after <seconds>` | `0` | Closes the window after this long; `0` means never |
| `--screenshot <file.png>` | none | With `--quit-after`, saves the last frame as a PNG |

An invalid option prints the usage and exits with code 2. The count, the seed, and the rest of the
starting values are the scene's.
