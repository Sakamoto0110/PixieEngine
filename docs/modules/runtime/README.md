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
systems. Samples and, later, the player and the editor's C API build Worlds with it.

Start with `pixie/runtime/World.hpp`: create a World, create entities, add components with
`Entity::Add<T>()` and logic with `Entity::AddBehaviour<T>()`, and call `World::Step` from the
App's fixed update. Draw entities that have a `Shape` with `SubmitShapes`.

Entities marked with `AddToScene` are saved to and loaded from scene files
(`pixie/runtime/Scene.hpp`), through the same field descriptors the C API uses. The module does not
own the window, input, or rendering; it uses the provisional engine library for those.

- [Manifest](MANIFEST.md): build target, dependencies, and routes.
- [Technical reference](REFERENCE.md): the contract.
