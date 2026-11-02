# World and Scene: execution blueprint for the first experiment

**Document ID:** PIXIE-DESIGN-WORLD-SCENE-EXPERIMENT

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** implementation decisions and acceptance gates for the World/Scene experiment

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Authority and execution boundary

Parent: [PIXIE-PROPOSAL-016](world-and-scene-properties.md).
Independent acceptance: [GPT-6.1-sol Medium review protocol](world-and-scene-acceptance.md).
Luna's own verification is required but cannot accept its implementation; only
the separate reviewer can issue the technical acceptance decision for this experiment.
Prepared on 2026-11-01 for an implementation experiment assigned by the owner
to the Luna model. This document prepares that assignment; it does not start
implementation, approve a release, or promote a scheduler entry by itself.

The numbered decisions below are the proposed design for this experiment.
They replace open-ended architecture choices for its implementer once the
owner authorizes execution. Do not reinterpret them as already implemented.
If a decision cannot be implemented safely, report the exact conflict and
stop the affected step rather than inventing a different architecture.

Only Engine changes are expected. Do not modify Inspector or Lib, change the
physics backend, publish, commit, or push without separate authorization.
Preserve all existing dirty work, especially entity input/code authoring and
field interactions. Re-read HEAD, status, and the relevant sources before work:
this blueprint was written while the input/code implementation was in progress.

## 1. Fixed scope and terminology

1. Keep one `World`, one EnTT registry, one clock and one CollisionWorld2D.
   World owns global settings. A Scene is an identified grouping of entities
   plus local property settings inside that World, not another World.
2. Implement gravity only. Lighting, exposure, time scale, solver settings,
   layer names, and input routing do not gain scene overrides in this experiment.
3. All entities keep absolute World coordinates. A Scene has no transform,
   spatial bounds, nested scenes, or coordinate offset.
4. Multiple Scene scopes may coexist and simulate in this experiment's shared
   World. This is a core capability, not a selected streaming/loading policy.
   There are no automatic portals, distance-based activation, preload, baking,
   per-scene pause, or independent simulation rates.
5. Cross-scene bodies collide using existing filters. Scene membership alone
   must not introduce a collision partition. Each participant uses its own gravity.
6. For this first experiment, persist the World and its Scene scopes together
   in the current authored scene document. This is explicitly a World snapshot
   envelope, not independently streamable Scene assets. Keep the existing project
   `startScene` path and editor open/save entry points. Asset splitting and lifecycle
   remain later work; do not build a second file-loading pipeline now.

## 2. Data model and public operations

Add `runtime/include/pixie/runtime/SceneScope.hpp` and
`runtime/src/SceneScope.cpp`; add the source to runtime's CMake source list.
Use these concepts and names; do not reuse `SceneId` for scope identity:

```cpp
using SceneScopeId = std::uint64_t; // 0 reserved for World-only membership
enum class SceneGravityMode { Inherit, Override, Multiply };
struct SceneGravity2D {
    SceneGravityMode mode = SceneGravityMode::Inherit;
    pxVec2 value{0, 0}; // used only for Override
    double multiplier = 1; // used only for Multiply
};
struct SceneMembership { SceneScopeId scope = 0; };
```

World registry context owns a table of scene definitions: nonzero stable ID,
UTF-8 display name, authored SceneGravity2D, and unknown JSON preservation data.
Names are not identities and need not be unique. Sort serialized definitions by
ID, not hash iteration order. A new scope ID uses a separate identity generator,
never World::Random. IDs must be unique within their World; collisions are refused.

Provide exported native functions for create, enumerate, rename, remove,
read/update gravity, assign membership, and resolve effective gravity. Return
owned/copy results for public definition reads; do not expose map iterators or
mutable references. Match existing error/report style rather than adding a new
exception hierarchy. C API wrappers must return errors and never throw across ABI.

- No SceneMembership or membership 0 means World-only, with inherited gravity.
- Membership in an unknown nonzero scope is an error, not silent inheritance.
- Removal of a nonempty scope is refused. Reassign its entities explicitly first.
- Mutation outside World::Step takes effect at the next synchronization, before
  the next physics solve. Mutation during a step must use World::Defer; direct
  structural mutation is refused consistently with existing lifecycle contracts.
- Deferred membership changes affect the following tick, not half of the current solve.
- Scope assignment never teleports an entity, clears velocity, or changes SceneId.
- Never infer ownership from an entity's position, component type, name, or camera.

`SceneId` remains the stable authored ENTITY identity used by history/references.
Scene scope IDs are independent; scene definitions are not hidden ECS entities.

## 3. Gravity resolution and validation

World's absolute gravity remains `PhysicsSettings2D::gravity`. Keep its single
World-wide instance and all existing units/substeps validation. It must be
World-only; refuse assigning its entity to a local scope. A world with no such
instance uses today's default `(0, 1100)`.

Resolution, before entity-specific scales:

| Mode | Effective scene gravity |
|---|---|
| Inherit | World gravity |
| Override | `SceneGravity2D.value` |
| Multiply | World gravity * `SceneGravity2D.multiplier` |

All stored numbers must be finite, including inactive mode values. Multiplier
must be >= 0; zero is valid. Override vectors may be any finite vector within
the existing backend coordinate limits. Validate resolved vectors as well as
stored data; multiplication overflow is an error before any native write.
Do not clamp invalid input silently. Setter failures leave the old setting intact.

For every consumer, apply its existing gravityScale exactly once:
`effectiveAcceleration = resolvedGravity(entity) * consumer.gravityScale`.
Dynamic particles use the scope of their ParticleSystem entity. They do not
acquire separate scope membership per particle or change scope by spatial position.

The existing upright CharacterMotor2D supports only downward or zero gravity.
Check the RESOLVED scene gravity, not World gravity: x == 0 and y >= 0. Retain
the existing per-entity error/skip contract for unsupported characters; do not
silently ignore x, use vector magnitude, or implement arbitrary gravity controllers.
Rigid bodies and particles can still use a sideways override in the same World.

## 4. Physics integration: fixed mechanism

Keep backend-global gravity equal to the World gravity. Keep each native rigid
body's authored gravityScale. Do not alternate backend gravity between scenes
and do not multiply the World gravityScale by a scalar ratio (World zero and
nonparallel override vectors make that incorrect).

Add a native per-body gravity-acceleration override to CollisionWorld2D. Store
an optional resolved scene vector on its body record and update it in place.
Before each b2World_Step, for dynamic enabled bodies with an override, apply a
center-of-mass compensation force in backend units:

`force = bodyMass * authoredGravityScale * (sceneGravity - worldGravity)`.

The backend supplies `worldGravity * authoredGravityScale`; the two terms sum
to the desired acceleration. Keep this implementation inside CollisionWorld2D,
which already owns unit conversion, mass, and backend handles. Do not call the
public force API unconditionally if it wakes sleeping bodies. Use the existing
backend force conversion and apply with wake=false for routine stepping.

When the effective acceleration changes (scope gravity, membership, global
gravity, or scale), wake affected dynamic bodies once. Reapplying an unchanged
setting must not wake them. Clearing an override restores backend World gravity.
Do not replace body/collider handles, clear contacts, reset controller velocity,
or emit fake contact-end events merely because a gravity value changes.
Static bodies, kinematic bodies, and character proxy bodies receive no compensation.
Use current mass after density/mass edits. Remove cached overrides with their body.

Apply compensation once per existing complete physics step, not once per
substep plus again per frame. Preserve existing NeedSimulation/sleep behavior.
Changing World gravity must also re-evaluate compensation for absolute overrides.

Update SceneMovement2D to resolve each rigid body's gravity and each motor's
settings. Update ParticleStep to resolve per ParticleSystem instead of reusing
one global vector for all systems. Keep one physics solve before particle stepping.

## 5. Persistence and compatibility

Extend the existing authored scene envelope to version 2. Add a root
`sceneScopes` array and an entity-level `sceneScope` ID. Example shape:

```json
{
  "format": "pixie-scene",
  "version": 2,
  "sceneScopes": [
    {"id": 101, "name": "Room A", "gravity":
      {"mode": "multiply", "value": [0, 0], "multiplier": 0.75}}
  ],
  "entities": []
}
```

This example omits existing required fields; retain them, including World seed
and all established entity/component encoding. Mode strings are `inherit`,
`override`, `multiply`. Encode IDs using existing stable integer-ID conventions.
Persist PhysicsSettings2D through its existing component descriptor exactly once
on its World-only entity; do not duplicate those settings in each scope.

Read version 1 as before: no local scopes, all entities World-only, same gravity.
No automatic room generation or data migration on opening. Version 2 saves mode
and authored values, not resolved caches. Validate the whole incoming scope table,
membership references, duplicate IDs, and numeric data before modifying the World.
Reuse existing scene-reader staging/rollback patterns. Do not introduce a partial
load in which entities or RNG state have already changed when a scope error is found.

Preserve unknown root/scope/gravity/entity/component fields using the existing
unknown-data conventions. An unknown future mode causes a clear unsupported-mode
load error before mutation, preserving the original file; do not substitute Inherit.
Keep SaveTextFile's temporary-file replacement; never overwrite unknown user files.

`WriteEntities`/`ReadEntities`, duplicate, undo, redo, restore, and Play/Stop snapshots
must preserve membership and referenced scope definitions. Entity fragment imports
reuse an existing scope only if its authored definition is equal; conflicting IDs
fail before mutation. Missing definitions may be imported from fragment metadata.
`SceneIds::New` remaps entity IDs only, never scope IDs. Duplication keeps membership.

World seed is restored once on whole-document load. Creating a scope, assigning
membership, and importing entity fragments must not reseed the World.

## 6. Editor and C API integration

Extend descriptor/C API facilities as needed with bounded operations for scope
listing/settings/membership. Bump ABI/API according to existing project rules when
layouts or exports require it; update managed bindings and probes together.
No independent managed resolver: effective values come from native code.

Add a Scenes panel with scope list, Create, Rename, Remove, and gravity controls.
Expose World gravity through the existing settings component; show which entity
owns it, or provide the existing component-creation route when absent. Do not
invent a second authoritative World gravity setting in C#.

For a selected scope, show mode choice, override vector or multiplier as relevant,
and a read-only effective vector with source. Switching modes retains inactive
authored values; it never writes a copy of the inherited value into an override.
Entity Inspector shows a Scene dropdown: World plus all scope names and IDs.
Multi-selection applies through the existing authoring/recorder flow, with one
undo step. Create/rename/remove/settings/assignment are undoable. Invalid batch
assignment (including PhysicsSettings2D) fails before any target changes.

New scopes start Inherit; new entities start World-only. No automatic attachment
to whichever scope happens to be selected in the panel. Existing entity selection,
chain/lock state invalidation, stale binding revision, save/unsaved prompts, and
Play/Stop behavior must keep their current contracts.

Default VS Debug and Release editor builds must expose the feature through the
same output paths the user opens. An isolated executable alone is not delivery.
No mandatory external package publication is needed for this Engine-only change.

## 7. Ordered implementation steps and gates

Before each step, inspect the named files and their actual dependencies; paths
below are a map, not permission to overwrite concurrent work. Keep helpers small
and typed. Do not introduce a generalized arbitrary-property inheritance framework.

| Step | Main files | Required result before continuing |
|---|---|---|
| A | SceneScope.hpp/.cpp; World lifecycle; runtime/CMakeLists.txt | IDs, CRUD, membership, validation, resolver tests |
| B | CollisionWorld2D.hpp/.cpp; SceneMovement2D.cpp; ParticleStep.cpp | All three consumer types observe correct resolved acceleration |
| C | Scene.hpp/.cpp and its serialization helper files; ProjectSession snapshots | Version 1 compatibility, version 2 round trip, fragments, failure preservation |
| D | capi/include/pixie_capi.h; capi/src; Native/PixieCapi.cs and EngineSession.cs | Native/managed parity, bounded errors, ABI synchronization |
| E | Ui/ScenesPanel.cs; InspectorPanel; EditorWindow; Editing/history | Real normal-build UI, assignment/settings history and lifecycle |
| F | runtime/C API/editor fixtures; docs/modules/runtime/REFERENCE.md; editor guide | Acceptance matrix below, final evidence report |

Use existing runtime-tests, C API, and editor fixtures instead of a new unrelated
test framework. Read their CMake targets and harness entry points before editing.
An unexpected unrelated failure is reported separately, never "fixed" by dropping
the case, loosening the assertion, or replacing the test with a source-text grep.

## 8. Mandatory acceptance matrix

1. World `(0,100)` with World-only entity and Inherit scope gives `(0,100)`;
   Multiply 0.75 gives `(0,75)`; Override `(30,40)` remains `(30,40)`.
2. Change World to `(0,200)`: Inherit becomes `(0,200)`, Multiply `(0,150)`,
   Override unchanged. World zero plus nonzero Override works.
3. Multiply zero works. Negative multiplier, NaN/Inf, unknown IDs, duplicate
   scope IDs, invalid mode and overflow fail without partial mutation.
4. For body/particle gravityScale 0.5 and resolved `(0,75)`, applied acceleration
   is `(0,37.5)`, not 75 or 18.75. Character tuning uses the same resolved base.
5. Two bodies in different scopes fall differently in one World/one solve.
   Test observed velocities over fixed ticks with known damping/collisions,
   including World zero + Override and a sideways vector; use tolerances.
6. Two particle systems use their own scope gravity. Supported motors in two
   scopes get different tuning; unsupported sideways motor reports an error
   without invalidating unrelated rigid bodies or supported motors.
7. Reassigning a moving entity preserves pose, velocity, IDs, body handle and
   contacts, with new gravity from the next solve. A deferred assignment changes
   only the following tick. Ordinary callbacks still obey World::Defer.
8. Unchanged gravity does not keep a resting body awake. A changed effective
   gravity wakes it and affects the next solve. Density/mass changes do not
   change gravity acceleration. Static/kinematic/proxy bodies are unaffected.
9. Nonempty scope removal and World-settings assignment are refused. Removing
   an empty scope succeeds. There is no position-based scope transfer.
10. Version 1 project/scene fixtures behave as before. Version 2 save/reopen
    preserves IDs, settings, membership, unknown data, seed and effective values.
    Invalid loads preserve old World/file state and RNG. Test fragment ID conflicts.
11. Real editor create/rename/mode/assignment undo and redo work; toggling
    Inherit follows World edits. Save/reopen and Play/Stop restore authored scopes
    and membership. Runtime-produced changes do not rewrite authored snapshots.
12. Build native and normal editor Debug/Release; run focused fixtures and the
    existing relevant runtime/C API/project-code/field-interactions regression
    suites. Run serial configurations where fixtures share temporary workspaces.
    Validate documentation and whitespace. No unsupported CI/platform claim.

Before declaring success, break a copy of the resolver or compensation on
purpose and confirm the corresponding behavioral test fails; restore exact bytes
and rerun the affected tests. Do not mutate user scenes for sensitivity checks.

## 9. Required delivery and stop conditions

Keep a durable checkpoint under `.local/validation/world-scene-experiment/`
with baseline HEAD/status, touched files, completed steps, commands/exit codes,
fixture results, normal executable paths, and remaining limits. Return completion
to the assigning chat for review, with screenshots from the real editor panel.
Owner visual acceptance is separate from automated acceptance.

Do not call the entire proposal implemented: this delivers the gravity/scope
experiment in a shared World snapshot. Independent scene assets, optional loaded
states and streaming/baking still require a later design.

Stop and report if existing serialization cannot preserve unknown fields or
validate before mutation, the backend cannot apply compensation without breaking
sleep/mass semantics, or concurrent source changes conflict. Give exact evidence
and a bounded proposed adjustment; do not silently create multiple Worlds,
remove compatibility, omit a consumer, or hide the feature behind an isolated build.
