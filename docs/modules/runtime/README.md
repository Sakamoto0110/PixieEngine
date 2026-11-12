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

The Runtime owns the live simulation. A `World` holds EnTT component storage, `Behaviour`
instances run per-entity logic, and systems perform shared work. Native players and the Editor’s
C API host Worlds through `ProjectSession`.

Start with `pixie/runtime/World.hpp`: create a World, create entities, add components with
`Entity::Add<T>()` and logic with `Entity::AddBehaviour<T>()`, and call `World::Step` from the
App's fixed update. Draw entities that have a `Shape` with `SubmitShapes`.

`AddToScene` assigns an authored identity. Scene persistence uses the same reflected field
descriptors as the C API. Runtime consumes the Core, Platform, Render and App boundaries in the
shared engine. A host supplies the window, input and graphics context.

- [Manifest](MANIFEST.md) for build targets, dependencies and the SDK boundary.
- [API map](REFERENCE.md#public-entry-points) for headers and symbol routes.
- [Step phases](REFERENCE.md#the-step) and [Behaviour lifecycle](REFERENCE.md#behaviours) for input and simulation order.
- [World and entity lifetime](REFERENCE.md#entity-identity-and-lifetime) for handles across reconstruction.
- [Reflection](REFERENCE.md#reflection) for descriptors and Inspector fields.
- [Scene families](REFERENCE.md#scene-families) and [parenting](REFERENCE.md#2d-parenting) for persistence and pose edits.
- [Projects and sessions](REFERENCE.md#projects-and-sessions) for opening, Edit, Play and save.
- [Physics composition](REFERENCE.md#authored-scene-movement) for collision layers, bodies and motors.
