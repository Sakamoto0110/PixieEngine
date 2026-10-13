# particles

Anchor 3 of [PIXIE-PROPOSAL-004](../../docs/proposals/scene-model-and-anchors.md): particles in
orbit around attractors that the user places, drags, and removes, from one particle to a hundred
thousand. It uses the `runtime` module's particle pools
([`docs/modules/runtime/REFERENCE.md`](../../docs/modules/runtime/REFERENCE.md#particles)).

- The particles are not entities. One `particles` entity holds a `ParticleEmitter`, a contiguous
  pool, with two batch affectors: an `AttractorAffector`, which pulls every particle toward every
  attractor with a force falling with the square of the distance, and an `InteractionAffector`, a
  short-range push between particles closer than 4 world units, found through a spatial grid.
- Attractors are entities (`Transform2D`, `Attractor`, and a gold ring `Shape`), so they can be
  placed and moved like anything else.
- A preset puts particles in nearly circular orbits, spread over the attractors in turn, between 40
  and 260 units out, at the circular speed `sqrt(strength / r)` give or take 8%. With one attractor
  and the interaction off, the disc keeps its shape while it turns; with the interaction on, the
  particles push each other into a puffier disc that gravity holds together. Attractors close to
  each other trade particles, and their orbits turn chaotic.
- Particles are drawn as additive discs straight from the pool; the GPU interpolates them between
  steps.

## Mouse and keys

| Input | Effect |
|---|---|
| Left click on empty space | Adds an attractor there (up to 16), and drags it while held |
| Left drag on an attractor | Moves it |
| Right click on an attractor | Removes it |
| Middle drag, wheel | Pan, and zoom around the cursor |
| 1 to 5 | Respawn 1, 10, 100, 10,000, or 100,000 particles; 100,000 turns the interaction off |
| I | Turn the particle-to-particle interaction on or off |
| R | Respawn the current count around the current attractors |

Changes are logged. Once per second the log also shows how long the World's steps took.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/particles
bin/Release/particles --count 100000 --attractors 16 --interaction off
```

## Options

| Option | Default | Meaning |
|---|---|---|
| `--count <n>` | `10000` | Particles at the start, 0 to 200,000 |
| `--attractors <n>` | `1` | Attractors at the start, 0 to 16: one in the middle, or a ring of them |
| `--interaction on\|off` | `on` | Particle-to-particle interaction |
| `--seed <n>` | `1` | Seed of the World's random numbers |
| `--ups <hz>` | `60` | Updates per second |
| `--fps <hz>` | `0` | Frame cap; `0` means none |
| `--vsync on\|off` | `on` | Vertical sync |
| `--quit-after <seconds>` | `0` | Closes the window after this long; `0` means never |
| `--screenshot <file.png>` | none | With `--quit-after`, saves the last frame as a PNG |

An invalid option prints the usage and exits with code 2.
