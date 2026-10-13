# Scene Model, Anchors, and Serialization

**Document ID:** GLOBAL-PROPOSAL-SCENE-MODEL

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** the object model, the anchor scenarios that drive the first cut, and the postponed scene serialization

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-004

**State:** promoted

**Idea:** Three parts.

1. **Object model.** An EC-style API over ECS storage. Game logic is written
   as C++ `Behaviour` classes with `Update()`; components are data-only
   structs in contiguous storage (EnTT), which engine systems iterate. A step
   runs in phases: `PreUpdate` (engine: spatial grid, input), `Update`
   (Behaviours read their neighbours and write only their own state),
   `PostUpdate` (engine: integration, bounds). Particles are not entities: an
   emitter entity owns a contiguous pool, user logic runs as batch affectors,
   and attractors are entities so an editor can place them. Each World has its
   own seeded random number generator.
2. **Anchors.** Three scenarios, each with user interaction and an editor,
   drawn with shapes or generated minimal sprites:
   1. Boids, steered by the user (for example, mouse proximity).
   2. Flocks: boids also steered by other boids.
   3. Particle systems with interactions (for example, attractors producing
      orbits), from 1 to thousands of particles.

   Scale targets at 60 updates per second, measured on the owner's machine:
   1,000 boids; 5,000 flocking boids through a spatial grid; 100,000 particles
   against up to 16 attractors and 10,000 with short-range particle-to-particle
   interaction. Long-range all-pairs gravity is outside the first cut.
3. **Scene serialization** (postponed). The Scene is the document saved to
   disk; the World is the live instance, and play mode copies one into the
   other. A JSON text format through nlohmann/json, with `format`, `version`,
   and a flat `entities` list: each entity has a stable ID (a random u64
   written as a hex string), a `name`, a `parent` by ID, and `components` by
   type name, each with its own `v`. Unknown components are kept as raw JSON
   and written back unchanged. Prefabs and nested scenes are deferred. A
   spawner (count, area, seed) and explicitly placed entities both have a
   place in the format, and the World's seed is saved. Serialization reads and
   writes components through the same field descriptors that the C API uses.

**Rationale:** A scene file stores the same thing under EC and ECS (entities
with components and their fields), so one format serves both; the difference
is the runtime. Data-only components make serialization, reflection, the C
API, and copying a Scene into a World direct; Behaviours give the authoring
style the owner wants from EC. The anchors span the model: anchor 1 is
per-entity logic, anchor 2 needs an engine service (neighbour queries) and an
order-independent step, and anchor 3 needs batch processing over a pool.

**Constraints:** Serialization is postponed by the owner (2026-10-13); until
it is promoted, Worlds are built in code and nothing is saved. The stable
entity ID, the `scene` boundary, and the adoption of nlohmann/json wait with
it.

**Promotion target:** parts 1 and 2 promoted to `TODO.md` on 2026-10-13
(PIXIE-TODO-005 to PIXIE-TODO-011); part 3 requires an owner decision.

**Source:** [decision record](../archive/object-model-and-editor-decisions-2026-10-13.md)

**Outcome:** parts 1 and 2 promoted on 2026-10-13 into
[`TODO.md`](../../TODO.md), as recorded in the
[promotion record](../archive/anchors-promotion-2026-10-13.md). Part 3
postponed.
