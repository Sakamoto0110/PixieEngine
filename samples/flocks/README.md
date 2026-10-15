# flocks

Anchor 2 of [PIXIE-PROPOSAL-004](../../docs/proposals/scene-model-and-anchors.md): five thousand
boids that flock with their neighbours and react to the mouse. It uses the `runtime` module's
spatial grid and shows how a step stays independent of update order
([`docs/modules/runtime/REFERENCE.md`](../../docs/modules/runtime/REFERENCE.md)).

- Each boid is an entity with `Transform2D`, `Velocity2D`, `Acceleration2D`, a triangle `Shape`,
  a `FlockMember` tag, and a `Flocker` Behaviour.
- At the start of each step, the `flocks.grid` system rebuilds the flock's `SpatialGrid` from the
  members' positions. `Flocker::Update` finds the neighbours within `neighbourRadius` in that
  snapshot and combines three rules, each a steering toward a desired velocity limited by
  `maxForce`: separation from neighbours closer than `separationRadius`, alignment with their
  average heading, and cohesion toward their center; plus the pointer, toward it or away from it
  within `pointerRadius`. It keeps the speed between `minSpeed` and `maxSpeed` and writes the
  result only as its own `Acceleration2D`.
- After every Flocker has run, the World integrates acceleration into velocity and velocity into
  position. The `flocks.orient-and-wrap` system then points each boid along its velocity, colors it
  by heading (boids moving together share a color), and wraps it at the view's edges.
- The tunable values live on the `flock` entity (`FlockControls`, `Pointer`, `Bounds`).

## Keys

| Key | Effect |
|---|---|
| Q / A | Separation weight up / down by 0.1 |
| W / S | Alignment weight up / down by 0.1 |
| E / D | Cohesion weight up / down by 0.1 |
| Space | Switch the pointer between attracting (green ring) and repelling (red ring) |
| Up / Down | Add or remove 500 boids |

Changes are logged. Once per second the log also shows how long the World's steps took.

The flock lives in the visible area, so adding boids makes it denser and each boid's neighbourhood
larger: the step's cost grows faster than the count.

## The project

This folder is a project ([editor's README](../../editor/README.md#projects)): `flocks.pixie`, and
`scenes/main.scene`, which holds the scenario and the `flock` entity with its controls. The starting values (the count) live in the
`FlocksScenario` component of the `scenario` entity: edit them in the editor and press "Rebuild the
scene", or save them. The boids themselves are generated from the scenario when the World is
built, and are not saved. A World built without the scene, as the C API test builds it, starts with
the same entities, made in code.

The scene's entities and the project's code are the sample's; its player and its C API library are
built by `pixie_game` ([runtime reference](../../docs/modules/runtime/REFERENCE.md#projects-with-code)).

## Layout

- `src/game/`: the game, built as the static library `flocks-game`. `FlocksProject` is its `Project`
  (see the [runtime reference](../../docs/modules/runtime/REFERENCE.md#projects-and-sessions)):
  it finds its entities in the scene, generates the rest, handles the pointer and the keys before
  each step, and describes the game's components for tools. `pixie::CreateProject` returns it.
- `flocks`: the player, the engine's, which opens this folder and plays its scene.
- `flocks-capi`: the same game as a shared library with the
  [C API](../../docs/modules/capi/README.md), for hosts such as the editor.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/flocks
```

The player opens `samples/flocks`. To edit the scene, open the folder in the editor, which builds it
as a project of its own (into `build/` and `bin/` here, which git ignores):

```bash
editor/PixieEditor/bin/Release/net10.0/PixieEditor --project samples/flocks
```

`--game flocks` still opens the game from the engine's `bin/`, without the scene.

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
