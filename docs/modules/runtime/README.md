# Runtime

**Document ID:** RUNTIME-README

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** introduction to the runtime module: the World, entities, Behaviours, and update phases

**Surface:** introduction

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

The runtime module holds the live simulation: a `World` of entities whose components are stored
contiguously by EnTT, with game logic written as `Behaviour` classes and shared work done by
systems. Players and C API hosts use the same project/session runtime.

Start with `pixie/runtime/World.hpp`: create a World, create entities, add components with
`Entity::Add<T>()` and logic with `Entity::AddBehaviour<T>()`, and call `World::Step` from the
App's fixed update. For a hosted game, start with `Project` and `ProjectSession`;
`ProjectApp` supplies the player wrapper and the C API supplies the host boundary.
The [reference](REFERENCE.md#projects-and-sessions) owns their lifecycle.

Entities marked with `AddToScene` are saved to and loaded from scene files
(`pixie/runtime/Scene.hpp`), through the same field descriptors the C API uses. The module does not
own the window, input implementation, or renderer implementations; its drawing
adapters use the provisional engine library. See the
[drawing contract](REFERENCE.md#drawing) for submission order and culling scope.

- [Manifest](MANIFEST.md): build target, dependencies, and routes.
- [Technical reference](REFERENCE.md): the contract.
