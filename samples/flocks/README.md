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

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/flocks
```

## Options

| Option | Default | Meaning |
|---|---|---|
| `--count <n>` | `5000` | Number of boids at the start, 0 to 100,000 |
| `--seed <n>` | `1` | Seed of the World's random numbers |
| `--ups <hz>` | `60` | Updates per second |
| `--fps <hz>` | `0` | Frame cap; `0` means none |
| `--vsync on\|off` | `on` | Vertical sync |
| `--quit-after <seconds>` | `0` | Closes the window after this long; `0` means never |
| `--screenshot <file.png>` | none | With `--quit-after`, saves the last frame as a PNG |

An invalid option prints the usage and exits with code 2.
