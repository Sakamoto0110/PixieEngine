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

The World is built in code (`src/game/BoidsScene.cpp`); nothing is saved or loaded.

The starting values (the count) live in a scenario struct of the project, mirrored in a `BoidsScenario` component on
the `scenario` entity; the options below fill it, and the editor rebuilds the scene from it.

## Layout

- `src/game/`: the game, built as the static library `boids-game`. `BoidsProject` is its `Project`
  (see the [runtime reference](../../docs/modules/runtime/REFERENCE.md#projects-and-sessions)):
  it builds the World, handles the pointer and the keys before each step, draws the overlay, and
  describes the game's components for tools. `pixie::CreateProject` returns it with its defaults.
- `src/player/`: the player, `boids`, which runs the project in its own window through
  `ProjectApp`.
- `boids-capi`: the same game as a shared library with the
  [C API](../../docs/modules/capi/README.md), for hosts such as the editor.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/boids
```

## Options

| Option | Default | Meaning |
|---|---|---|
| `--count <n>` | `1000` | Number of boids at the start, 0 to 200,000 |
| `--seed <n>` | `1` | Seed of the World's random numbers |
| `--ups <hz>` | `60` | Updates per second |
| `--fps <hz>` | `0` | Frame cap; `0` means none |
| `--vsync on\|off` | `on` | Vertical sync |
| `--quit-after <seconds>` | `0` | Closes the window after this long; `0` means never |
| `--screenshot <file.png>` | none | With `--quit-after`, saves the last frame as a PNG |

An invalid option prints the usage and exits with code 2.
