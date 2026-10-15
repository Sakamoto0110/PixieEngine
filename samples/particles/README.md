# particles

Anchor 3 of [PIXIE-PROPOSAL-004](../../docs/proposals/scene-model-and-anchors.md): particles in
orbit around attractors that the user places, drags, and removes, from one particle to a hundred
thousand. It uses the `runtime` module's particle pools
([`docs/modules/runtime/REFERENCE.md`](../../docs/modules/runtime/REFERENCE.md#particles)).

- The particles are not entities. One `particles` entity holds a `ParticleSystem`, a contiguous
  pool, with two batch affectors: an `AttractorAffector`, which pulls every particle toward every
  attractor with a force falling with the square of the distance, and an `InteractionAffector`, a
  short-range push between particles closer than 4 world units, found through a spatial grid.
- Attractors are entities (`Transform2D`, `Attractor`, and a gold ring `Shape`), so they can be
  placed and moved like anything else. The ring is the attractor's body (`Attractor.size`); with
  "die on contact" set in the editor, particles that fall into it disappear.
- A `spawner` entity on the left feeds the pool: 150 particles per second leave a small ring
  outward (`ParticleSpawner`, shape `Ring`, direction mode `ShapeNormal`), live 8 seconds, and fall
  toward the attractors. In the editor, its direction and spread turn on a dial, and its system
  and target are chosen among the entities. More spawners can be placed with Alt + click; a new
  one is a copy of this one with a rate of 0, so it emits nothing until its rate is set.
- The interaction's values live in a `ParticleControls` component on the `particles` entity, which
  the project copies into the affector before each step, so a tool can change them.
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
| Alt + left click on empty space | Adds a spawner there (up to 16), at a rate of 0, and drags it while held |
| Left drag on an attractor or a spawner | Moves it |
| Right click on an attractor or a spawner | Removes it |
| Middle drag, wheel | Pan, and zoom around the cursor |
| 1 to 5 | Respawn 1, 10, 100, 10,000, or 100,000 particles; 100,000 turns the interaction off |
| I | Turn the particle-to-particle interaction on or off |
| R | Respawn the current count around the current attractors |

Changes are logged. Once per second the log also shows how long the World's steps took.

## The project

This folder is a project ([editor's README](../../editor/README.md#projects)): `particles.pixie`, and
`scenes/main.scene`, which holds the scenario, the `particles` entity with its system and controls, the attractor, and the spawner. The starting values (the count and whether the interaction starts on) live in the
`ParticlesScenario` component of the `scenario` entity: edit them in the editor and press "Rebuild the
scene", or save them. The particles themselves are generated from the scenario when the World is
built, and are not saved. A World built without the scene, as the C API test builds it, starts with
the same entities, made in code.

The scene's entities and the project's code are the sample's; its player and its C API library are
built by `pixie_game` ([runtime reference](../../docs/modules/runtime/REFERENCE.md#projects-with-code)).

## Layout

- `src/game/`: the game, built as the static library `particles-game`. `ParticlesProject` is its `Project`
  (see the [runtime reference](../../docs/modules/runtime/REFERENCE.md#projects-and-sessions)):
  it finds its entities in the scene, generates the rest, handles the pointer and the keys before
  each step, and describes the game's components for tools. `pixie::CreateProject` returns it.
- `particles`: the player, the engine's, which opens this folder and plays its scene.
- `particles-capi`: the same game as a shared library with the
  [C API](../../docs/modules/capi/README.md), for hosts such as the editor.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/particles
```

The player opens `samples/particles`. To edit the scene, open the folder in the editor, which builds it
as a project of its own (into `build/` and `bin/` here, which git ignores):

```bash
editor/PixieEditor/bin/Release/net10.0/PixieEditor --project samples/particles
```

`--game particles` still opens the game from the engine's `bin/`, without the scene.

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
