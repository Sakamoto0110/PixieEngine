# Runtime Technical Reference

**Document ID:** RUNTIME-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the contract of the runtime module: World, Entity, Behaviour, systems, phases, time, randomness, and the built-in components

**Surface:** technical-reference

**Boundary:** runtime

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

## Mental model

The object model is an EC-style API over ECS storage
([`ROADMAP.md`](../../../ROADMAP.md#active-guardrails)). Components are data-only structs, each
type in its own contiguous EnTT storage. Game logic is a `Behaviour`: a class with `Update()` that
is itself a component, so every Behaviour class also has contiguous storage. Work shared by many
entities (integration, and later spatial queries) is a system: a function over the whole World.

## Public entry points

All under `runtime/include/pixie/runtime/`:

| Header | Provides |
|---|---|
| `World.hpp` | `World`, `SystemPhase`, `System`, and the template members of `Entity` |
| `Entity.hpp` | `Entity`, a handle (World pointer and EnTT entity) |
| `Behaviour.hpp` | `Behaviour`, the base of logic classes |
| `Random.hpp` | `Random`, the seeded generator |
| `Transform2D.hpp` | `Transform2D` and `PreviousTransform2D` |
| `Velocity2D.hpp` | `Velocity2D` |
| `Acceleration2D.hpp` | `Acceleration2D` |
| `SpatialGrid.hpp` | `SpatialGrid`, radius queries over points |
| `SpatialIndex.hpp` | `AddSpatialIndex`, the system that keeps a grid filled |
| `ParticleEmitter.hpp` | `ParticleEmitter`, a component owning a particle pool |
| `ParticleAffector.hpp` | `ParticleAffector`, the base of batch logic over a pool |
| `ParticleSystem.hpp` | `AddParticleSystem` and `DrawParticles` |
| `Attractor.hpp`, `AttractorAffector.hpp` | Attractor entities and the affector that pulls toward them |
| `InteractionAffector.hpp` | Short-range push between the particles of a pool |
| `Shape.hpp` | `Shape`, an entity's look |
| `Name.hpp` | `Name`, an optional label |
| `ShapeDrawing.hpp` | `SubmitShapes`, the render system for shapes |

Use `Entity`'s templates through `World.hpp`, which defines them.

## The step

`World::Step(delta, input)` advances the World by `delta` seconds:

1. `PreUpdate` systems, in the order added. The first is the World's own: it copies every
   `Transform2D` into the entity's `PreviousTransform2D`.
2. Behaviours: for each Behaviour class, in the order the classes were first added to the World,
   every instance in its storage's order. An instance's `Start` runs right before its first
   `Update`. A class first added during a step is updated from the next step on.
3. `PostUpdate` systems, in the order added. The first is the World's own: it adds each
   `Acceleration2D` to its `Velocity2D` (`linear += acceleration * delta`), then integrates every
   `Velocity2D` into its `Transform2D` (`position += linear * delta`, `rotation += angular *
   delta`), so the position moves with the new velocity (semi-implicit Euler).
4. The destructions requested during the step, then the deferred actions, in request order.

Each system and Behaviour receives the World's `FrameTime`: `delta`, `elapsed` (the World's
simulated seconds before this step), and `index` (the World's step count before this step); `alpha`
is 0. `Steps()` and `Elapsed()` report the totals. The World's time is its own: two Worlds, or a
paused one, keep separate clocks.

## Behaviours

- A Behaviour derives from `Behaviour` and overrides any of `Start`, `Update(time, input)`, and
  `Stop`. `Self()` is its entity; `GetWorld()` its World.
- `Entity::AddBehaviour<T>(args...)` constructs it in place, replacing a `T` already there.
  (`Entity::Add<T>` returns the component, or nothing for an empty tag type, which EnTT stores
  without instances.)
- `Stop` runs once, if `Start` ran, when the Behaviour leaves: removed with `Remove<T>()`, its
  entity destroyed, or the World destroyed. At World destruction, Behaviours stop class by class
  before any other component is removed, so `Stop` can still read its entity.
- `Update` should write only its own entity's components and read the rest. Shared results come
  from systems; that keeps a step independent of update order.
- Behaviours move when their storage grows or shrinks. Keep `Entity` handles, not pointers or
  references to Behaviours or components, across steps.

## Structural changes and destruction

- `World::Destroy(entity)` and `Entity::Destroy()` take effect at the end of the step when called
  during one, and immediately otherwise. Until then the entity stays alive and readable.
- Creating entities, or adding and removing components, while a step runs may move storage that a
  system or a Behaviour class is iterating. Do it through `World::Defer(action)`, which runs
  `action` at the end of the step (immediately outside a step).
- `Entity::Add<T>` replaces an existing `T`. An `Entity` stays valid to copy after its entity is
  gone; `IsAlive()` tells.

## Built-in components

- `Transform2D`: `position` (`pxPoint`, world units), `rotation` (radians, from +X toward +Y, so
  clockwise on screen, since Y points down), `scale` (`pxVec2`).
- `PreviousTransform2D`: added with every `Transform2D` and refreshed at each step's start;
  `World::ResetInterpolation(entity)` makes it equal the current transform, so a jump is not
  drawn as motion.
- `Velocity2D`: `linear` (`pxVec2`, units per second) and `angular` (radians per second).
- `Acceleration2D`: `linear` (`pxVec2`, units per second squared). It persists until changed; a
  steering Behaviour overwrites it every step.
- `Shape`: a `ShapeKind`, a half extent `size` before the transform's scale, and a `pxColorRgba`.
- `Name`: a string, added by `World::Create(name)` when the name is not empty.

## Order independence and neighbours

A step's result does not depend on the order Behaviours run in when each Behaviour writes only its
own components and reads others' state that no Behaviour writes during the step. The runtime is
shaped for that: positions and velocities change only in `PostUpdate`, and a steering Behaviour
writes its `Acceleration2D`.

Neighbour queries read a snapshot. `SpatialGrid` is a uniform grid of square cells whose occupied
cells are found through a hash table sized to the items, so it has no bounds. Fill it with `Clear`,
`Add`, and `Build`; `ForEachWithin(center, radius, fn)` then calls `fn(id, position,
distanceSquared)` once for each item within `radius`, the boundary included. A cell size near the
usual query radius keeps queries cheap. `AddSpatialIndex<Member>(world, owner, cellSize)` gives
`owner` a `SpatialGrid` component and adds a `PreUpdate` system that refills it, at the start of
every step, from each entity with a `Transform2D` and a `Member` component.

## Particles

Particles are not entities. A `ParticleEmitter` component owns a contiguous `std::vector` of
`Particle` (32 bytes, in float: position, previous position, velocity, age, lifetime; declared by
the engine library, whose `ParticleRenderer` reads it as its vertex format). The emitter is
move-only, since it owns its affectors.

`AddParticleSystem(world)` adds the `particles.step` system to `PostUpdate`, after the World's
integration, so emitters and attractors are where the step left them. For each emitter, in turn:

1. every particle's `previous` becomes its `position` and its age grows by the step; a particle
   past a non-zero lifetime leaves the pool, and the last particle takes its place, so pool order
   is not stable;
2. `rate` emission adds particles at the entity's `Transform2D` (the origin without one), within
   `spread` around its rotation, at a speed in `[speedMin, speedMax]`, while the pool is below
   `capacity`; `Emit` adds one particle directly;
3. the enabled affectors run, in the order added;
4. every particle moves by its velocity.

A `ParticleAffector` is the particles' counterpart of a Behaviour: a class whose
`Apply(world, emitter, particles, time)` receives the whole pool. Affectors should change
velocities and read positions, which none of them changes, so their result does not depend on
particle order. The runtime provides two:

- `AttractorAffector`: every entity with a `Transform2D` and an `Attractor` pulls each particle
  with `strength * offset / (distance^2 + softening^2)^(3/2)`, ignoring particles beyond a
  non-zero `radius`. A circular orbit at distance `r` has speed `sqrt(strength / r)`.
- `InteractionAffector`: particles closer than `radius` push each other apart with
  `stiffness * overlap`, and `damping` pulls their velocities together while in contact. It builds
  a `SpatialGrid` over the pool every step (the grid's ids are then pool indices: grid ids are
  opaque 32-bit values), computes every change from the state before it, then applies them all.

`DrawParticles(world, renderer, camera, alpha)` draws every pool with its `style`, interpolated on
the GPU.

## Drawing

`SubmitShapes(world, renderer, alpha)` submits each entity with `Transform2D` and `Shape` to a
`ShapeRenderer`, at the transform interpolated `alpha` of the way from `PreviousTransform2D`:
position and scale linearly, rotation the short way around. The caller flushes the renderer with
its camera. It needs the GL context current, as the renderer does.

## Randomness

`World(seed)` seeds the World's `Random`. `Random` is xoshiro256** seeded through splitmix64; its
sequences depend only on the seed, on every compiler and platform. The first value for seed 0 is
pinned by a test, so a change to the generator does not go unnoticed.

## Threading

None. A World and its Behaviours are used from one thread at a time.

## Non-goals

No saving or loading (serialization is postponed), no prefabs or hierarchy, no physics, no
multithreaded systems, no long-range all-pairs forces between particles.

## Validation

`tests/runtime/` (CTest `runtime-tests`) covers the phase order, the Behaviour lifecycle,
deferred destruction and actions, the World's time, velocity integration with the previous
transform, the Entity handle, the generator's determinism, the spatial grid against a brute-force
search, a step's independence from the Behaviours' update order (with a negative control that
breaks the rule and is caught), particle emission, aging, expiry, and capacity, a circular orbit
that keeps its radius, and the interaction's independence from particle order. `SubmitShapes` is exercised by the
samples that draw a World.
