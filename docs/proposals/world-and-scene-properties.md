# World and scene property scopes

**Document ID:** PIXIE-PROPOSAL-016

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** distinct World and Scene scopes, with explicit property inheritance

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## State and owner direction

For the Luna implementation experiment, see the detailed
[execution blueprint](world-and-scene-implementation.md). It fixes a bounded
gravity/scope implementation design, file map, ordering, and acceptance gates;
it does not start execution or settle future loading/streaming policy.
The owner also requires a separate [independent acceptance run](world-and-scene-acceptance.md)
by GPT-6.1-sol Medium after Luna's implementation. This is a task-specific rule,
not an existing global VAW policy.

State: idea, recorded 2026-11-01. Documentation only; no implementation or
scheduler promotion is authorized by this proposal.

The owner distinguishes World from Scene. World is the global scope holding
absolute/default values. It can contain multiple scenes, such as rooms or
corridor segments. A scene's effective properties need not equal the World's.
Gravity motivated the proposal; the full property inventory is not yet known.

The owner suggested either scene multipliers or explicit scene overrides with
a sentinel indicating inheritance. The modes below are a design recommendation,
not an accepted final API.

Concurrent loading and simulation policy is intentionally undecided. The owner
may want the next scene prepared in some situations and not in others. Do not
equate prepared, loaded, visible, active, or simulating, and do not assign a
technical meaning to "baked" without a later decision.

## Current implementation versus proposed model

Today `World` owns the ECS and step lifecycle. `ProjectSession` loads the
project's start scene into its World. Scene serialization and `SceneId` identify
authored entities; they do not establish multiple independent property scopes.
`PhysicsSettings2D` supplies World-level gravity, with consumers in scene
movement, collision physics, and particle stepping.

The proposal must define these scopes without assuming that each Scene needs
its own ECS registry or physics backend. Those are implementation decisions.
See the [runtime reference](../modules/runtime/REFERENCE.md),
[`SceneMovement2D.hpp`](../../runtime/include/pixie/runtime/SceneMovement2D.hpp),
and [`ProjectSession.hpp`](../../runtime/include/pixie/runtime/ProjectSession.hpp).

## Recommended property resolution

Represent inheritance as an explicit state, never an ordinary numeric value
such as `uint32_t(-1)`. Negative and zero values can be meaningful; a sentinel
also obscures types, validation, serialization, and Inspector presentation.

| Scene mode | Effective value | Example with World gravity magnitude 100 |
|---|---|---|
| Inherit | Current World value | 100 |
| Override | Scene's absolute value | Override 75 gives 75 |
| Multiply | Current World value times scene factor | Factor 0.75 gives 75 |

Use one mode at a time per property. An inherited value follows later World
changes; an override remains absolute; a multiplier continues following its
World base. Multiplier 1 preserves the base, and 0 is a valid zero-gravity
setting. A nonzero override can supply gravity even when the World value is 0.

For vector gravity, the initial recommendation is an absolute vector override
or a scalar multiplier of the World vector, retaining its direction. Rotation,
axis-specific factors, and blending are not implied. A negative factor's
meaning/validation must be specified before implementation.

The schema for each property declares which modes it supports. Do not invent
multiplication for flags, layer names, enums, or solver configuration. Candidate
scopes to investigate include ambient lighting and background/environment
settings; these are inventory leads, not approved changes. Exposure may belong
to a camera/view instead. Fixed-step duration and solver substeps require
separate analysis when scenes can interact; scene-local time is not implied.

## Entity membership and transitions

A property resolver needs a well-defined scene context for each consumer.
Proposed starting point: one owning Scene per scene-local entity, with an
explicit World-only scope for persistent/global entities. This is not decided.
Ownership must not be inferred from spatial overlap or rendering visibility.

Moving a player through a doorway does not by itself define when its scene
context changes. Scene transitions, persistent entities, cross-scene references,
collision interaction, and property timing at transfer remain open decisions.
Do not introduce nested-scene inheritance or blended boundary regions implicitly.

All relevant consumers must use the same resolved gravity: rigid bodies,
character motors, and dynamic particles. Existing per-body/per-system gravity
scales must compose once with the resolved scene value, not apply the scene
multiplier again. Character constraints and gravity direction support must be
checked separately; a vector setting alone does not provide a top-down controller.

The current physics service exposes a backend-global gravity setting. Distinct
simultaneously simulated scenes cannot be implemented by repeatedly overwriting
that setting for each scene. A later design must choose and qualify per-body
forces/scales, separate simulation spaces, or another coherent mechanism,
including cross-scene contacts and events.

## Authoring and persistence direction

The Inspector should show the mode, authored value/factor, and effective value
with its source. Inherit must not copy the current World value into an override.
Persist authored settings and stable scene identity, not resolved caches.
World settings need a global home rather than depending on a particular room
remaining loaded. Existing scenes must preserve their behavior through an
explicit migration/default policy, retaining unknown data.

## Decisions before promotion

- Confirm the modes and inventory of properties with their units and scope.
- Define scene identity/membership and global entities independently of ECS layout.
- Define optional preparation/loading/simulation lifecycle and transition rules.
- Choose physics-space and cross-scene interaction semantics if scenes simulate together.
- Define serialization, compatibility, Inspector editing, history, and property resolution timing.
- Qualify inherited World edits, zero gravity, independent overrides, consumer
  agreement, transfers, unloads, and unknown-data round trips when implementation is authorized.

This proposal does not expand the already authorized entity input and C++
component authoring work.
