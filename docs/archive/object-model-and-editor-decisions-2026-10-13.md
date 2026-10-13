# Object Model, Anchors, Editor, and Postponed Serialization

**Document ID:** ARCHIVE-OBJECT-MODEL-EDITOR-20261013

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's answers on the object model, the anchor scenarios, the editor, dependencies, and scene serialization

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-013

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-13

**Commits:** with this record (parent `b73e76b0f2d94f6ead732b9c9fe32c63ccd4e875`)

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#current-direction), [`TODO.md`](../../TODO.md), and [PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md)

## How the decisions were reached

The owner opened the discussion with serialization (scenes, worlds,
components, objects) and asked for the alternatives to ECS. Claude laid out
the models (inheritance, Entity-Component as in Unity, a node tree as in
Godot, and ECS) and recommended data-only components with systems, because a
scene file stores the same thing under every model, and because the C API,
the inspector's proxies, and serialization all need the same field
descriptions.

On performance outside I/O, Claude's answer was that ECS wins clearly only
when a system iterates many entities over the same components (contiguous
memory, no virtual call per object), and that a 2D game of hundreds to a few
thousand entities is usually bound by rendering instead; the case for ECS
here is data-only components, with speed as a by-product.

The owner preferred EC, for writing logic as a class with `Update()`, and
asked whether both models could be supported, chosen at boot. Claude answered
that the file format is shared, so serialization needs one mode, but two
runtimes would double every component and system; it recommended an EC-style
API over ECS storage. The owner confirmed that writing logic as a class with
`Update()` is what EC meant to them.

The owner then set three anchors to find out what the engine needs, in any
order and all with user interaction and an editor: boids steered by the user,
flocks, and particle systems with interactions, from one particle to
thousands. Claude derived the requirements (input and a camera, a spatial
grid and an order-independent step, particle pools outside the entity
system, compute shaders for the long-range case) and what a scene file would
and would not hold.

The owner said that the editor starts as an ImGui view, through
Hexa.NET.ImGui, asked for a recommendation on every open question, and
accepted them all on 2026-10-13, postponing serialization.

## Decisions

Answered in the unversioned working list of open questions, and promoted
here:

- **Object model:** an EC-style API over ECS storage; logic as C++
  `Behaviour` classes with `Update()`; one runtime, no mode chosen at boot.
  Promoted to `ROADMAP.md` as a guardrail.
- **Update phases:** `PreUpdate` (engine), `Update` (Behaviours, writing only
  their own state), `PostUpdate` (engine). Promoted to `ROADMAP.md`.
- **Particles:** a pool owned by an emitter entity, batch affectors, and
  attractors as entities. Promoted with anchor 3.
- **Anchors and scale targets:** three anchors recorded in
  [PIXIE-PROPOSAL-004](../proposals/scene-model-and-anchors.md), with 1,000
  boids, 5,000 flocking boids, 100,000 particles against up to 16 attractors,
  and 10,000 with short-range interaction, each at 60 updates per second; the
  anchors run in the player first and are not blocked by the editor.
- **Component storage:** EnTT, vendored (v4.0.0, the newest release on
  2026-10-13; it requires C++20).
- **Reflection:** the engine's own field descriptors (name, type, offset),
  feeding the C API, the proxies, and later serialization; not `entt::meta`.
- **PixieLib:** v0.2.0 vendored with its GLM, so a build never needs the
  network. Recorded in `DEPENDENCIES.md`.
- **Gameplay code:** `Behaviour` in the first cut, without hot reload; a
  project's game code is a static library linked into its player and into a
  per-project `capi` library. C# scripting comes later.
- **Editor viewport:** the engine renders into an offscreen framebuffer in
  the editor's own OpenGL 4.5 core context, and the editor draws its texture
  with `ImGui.Image`; mouse positions over the image reach the engine as input
  events. On the editor side the engine library never calls GLFW.
- **Editor host:** a C# application under `editor/`, in its own solution,
  referencing `PixieInspector.ImGui` from NuGet, set up like PixieInspector's
  ImGui host (Hexa.NET.GLFW and the GLFW and OpenGL3 backends of
  Hexa.NET.ImGui) with a 4.5 core context. Promoted to `ROADMAP.md`.
- **Proposals:** PIXIE-PROPOSAL-001, the `runtime` boundary now and `capi`
  with the bridge, the others when their code is promoted; PIXIE-PROPOSAL-002,
  accepted; PIXIE-PROPOSAL-003, superseded by the anchors.
- **`capi`:** the name is kept (short for "C API"), defined at its first
  mention.
- **Documentation verifier:** a small script, scheduled before the first
  module so that every later documentation change is checked.
- **Serialization postponed:** the scene format (JSON through nlohmann/json),
  stable u64 entity IDs written as hex strings, unknown components kept
  verbatim, prefabs deferred, and spawners with explicit entities are the
  recommendations on record in PIXIE-PROPOSAL-004, part 3; none is promoted.

## Ordering

The owner asked for the blocks to be ordered by simple tasks and by the order
that tasks require. Recommendation 25 had put the editor before anchors 2 and
3; under that instruction the anchors that need no editor come first, and the
editor block covers all three anchors.
