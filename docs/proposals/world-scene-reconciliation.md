# World and Scene architecture

**Document ID:** PIXIE-PROPOSAL-018

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** World capabilities, Scene membership and preservation of authored state

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Decision and scope

On 2026-11-02 the owner accepted the following planning direction. World is
the execution context. Scene is an identified scope within it. An entity can
belong directly to World even when Scenes exist. A Scene can be empty and
still carry authored data. An entity keeps its own identity and components
when it changes membership.

This reconciles [proposal 016](world-and-scene-properties.md) and its
[gravity experiment](world-and-scene-implementation.md). The blueprint and its
[acceptance protocol](world-and-scene-acceptance.md) remain records of that
experiment. Promoted and activated as
[PIXIE-TODO-057](../archive/todo-057-world-scene-reconciliation-2026-11-04.md)
on 2026-11-02 and completed on 2026-11-04. The archive records qualification and the approved global-body gravity fallback; the decisions below preserve the accepted scope.

### How to read the decisions

The first reference, `PixieEngine_World_Scene_Proposta_Arquitetural.md` (A),
describes a compositional target. The second,
`PixieEngine_World_Scene_Recuperacao.md` (B), recovers both an experiment
and a longer-term direction. B's experimental constraints apply to the
gravity slice. The later owner decisions below govern this plan where it
differs from either reference.

The following table records the audited 2026-11-02 baseline. Later owner decisions
and the runtime/editor references describe the implemented migration.

| Question | A and B | Verified implementation | Decision and status |
|---|---|---|---|
| Does a Scene have its own simulation? | A leaves ECS ownership open and illustrates an active Scene. B keeps multiple scopes in one simultaneous solve. | [World](../../runtime/src/World.cpp) owns one EnTT registry, RNG, clock and phased systems. [SceneId](../../runtime/include/pixie/runtime/Scene.hpp) identifies an authored entity. | **Accepted initial model.** Share registry, clock and solver across scopes; preserve cross-scope contacts and World-only entities. Independent loading or isolation remains open. No implicit semantics for `SetActiveScene()`. |
| Who owns global settings? | A puts settings with the responsible system. B retains a World-only settings entity for the gravity experiment. | [Movement](../../runtime/src/SceneMovement2D.cpp) rejects duplicate physics settings. [Lighting](../../runtime/src/LightingDrawing.cpp) takes the first. [Particles](../../runtime/src/ParticleStep.cpp) can read a component even when the collision service is invalid. | **Accepted direction.** One authority per property and typed access. A v1 adapter preserves behavior. Duplicate handling and the owner of each render property still need decisions. |
| What is saved together? | A distinguishes World and Scene as authored data. B keeps them in one World snapshot and defers independent assets. | [Scene.cpp](../../runtime/src/Scene.cpp) handles both whole documents and entity fragments. Play captures the document; code rebuild currently captures entities. | **Invariant accepted; envelope preferred.** Integral reconstruction applies to any format. Evaluate v2 in the existing start-scene path; approve its schema and migration operation before enabling writes. |
| What does local gravity promise? | A leaves solver partitioning open. B fixes one solver and proposes force compensation. | Ordinary Box2D integration and continuous collision time-of-impact correction use gravity differently. | **Authored modes accepted for evaluation. Physical technique unapproved.** Qualification may stop local gravity without blocking scopes. |
| How are cameras and environments selected? | A and B suggest camera components and a viewport → Scene → World fallback. | [ProjectSession](../../runtime/src/ProjectSession.cpp) owns a camera by value, uses it for rendering and rebuilds the World for Play/Stop. Authored camera entities would be new. | **Preferred initial default.** Choose an environment explicitly per viewport. A camera fallback chain and the treatment of expired handles remain design choices; editor selection is never an implicit selector. |
| How is global behavior composed? | A illustrates a typed registry; B also considers a World controller or composed systems. | [World::AddSystem](../../runtime/include/pixie/runtime/World.hpp) registers a callback in a phase. Scene movement already offers typed access to an optional service. Neither API defines general module removal. | **Generic registry deferred.** Use concrete capabilities and project/system hooks. There is no demonstrated requirement for a mandatory controller, a 2D/3D subclass hierarchy or a ban on hybrid composition. |

### Engine mechanisms, host adaptation and consumer policy

The runtime already owns start-scene loading, simulation order and session
reconstruction. Keep that path shared by the player and editor. Coordinate
conversion and picking are reusable mechanisms. Which viewport receives a
gesture is a host decision; what a right click spawns is game policy.
[Sparks](../../samples/sparks/src/SparksProject.cpp) turns contacts into
sparks; [Boids](../../samples/boids/src/game/BoidsProject.cpp) uses the
camera bounds as its arena. Neither behavior belongs in a mandatory global
controller. [LightMachine](../../samples/light-machine/src/LightMachineProject.cpp)
is the useful migration case: let it change ambient lighting without finding
a settings entity, while the switch and its effect remain game code.
[Movement](../../samples/movement/main.cpp) must remain free to compose its
own World, collision service and camera directly.

## World, membership and capabilities

The initial model shares one EnTT registry, simulation clock and physics solver
within a World when physics is installed. Entities in different Scenes can
interact through the existing collision filters. Membership is explicit.
Changing it preserves entity identity, pose, velocity and contacts. Transfers
requested during a step are deferred and affect the next tick. Removing an
occupied Scene is refused until its entities have been reassigned.

Editor selection, visibility, simulation participation and input routing are
independent. Selecting a room must not redirect input or pause another room.
The public contract leaves room for future partitions, loading policies and
other simulation arrangements.

For example, a corridor entity can stay World-only while two rooms simulate.
Selecting room B in the editor does not change which room a viewport shows.
Moving an entity from room A to B changes its locally resolved properties on
the next applicable synchronization; it does not change its absolute position.
These are separate operations even if a game chooses to perform them together.

Each property has one authority. Typed access goes through the responsible
system. Querying an absent capability must not install it. A capability can
also be installed but disabled, or active with default settings. Preserve
these distinctions in the runtime, persistence and editor. Legacy scenes with
no lighting settings render unlit; physics already supplies default gravity
without a settings entity. A legacy ECS adapter must preserve both behaviors.
Duplicate or conflicting settings need an explicit migration rule.

**Owner decision (2026-11-02):** Keep v1 scene reads compatible with their
existing behavior when settings are duplicated. Reject a new import or edit
that encounters the duplicate conflict before mutating World, RNG, history or
the file. This does not select the authority of each settings group or the
rule for conflicting old and new representations.

Capabilities can be composed for 2D, top-down, 2.5D and future 3D consumers.
No fixed World subclass hierarchy is required. Defer a generic system registry
until a consumer demonstrates a need for discovery, removal or stable module
identity beyond the existing typed services and phased callbacks.

A consumer could pair 2D physics with a future 3D renderer. A preset can
supply initial settings for that combination. Installing both 2D and 3D
physics would not itself specify how bodies in different solvers interact.

## Persistence and reconstruction

**Owner decision (2026-11-02):** Use a single v2 World snapshot envelope in
the existing start-scene file, with a root `sceneScopes` catalog and an entity
`sceneScope` reference. Scope IDs use the hexadecimal text representation of
entity `SceneId` values, but the two IDs occupy separate domains. Validate
collisions within each domain and references across them. An absent or `null`
entity `sceneScope` means direct World membership; zero is not a Scene scope ID.
Conversion from v1 to v2 is explicit and transactional: validate the complete
result before replacing the original file, and retain a recoverable copy of
the original. The import-conflict policy and individual settings authorities
still require decisions before their corresponding writes are enabled.

**Owner import-admission decision (2026-11-02):** A version 2 operation must
reject every registered custom component that lacks a declarative import
contract before changing the World, entities, RNG, editor history or file.
The diagnostic identifies every incompatible type. There is no partial import
or silent discard. Components with a qualified contract remain importable;
unregistered component payloads stay opaque and preserved. Version 1 reading
and existing compatibility behavior remain intact. This admission rule does
not by itself make callbacks or external registry hooks transactional; the
authoring gate remains closed until that boundary is qualified.

The snapshot must satisfy these invariants:

- Keep the existing entity `SceneId` separate from the stable identity of a
  Scene scope. Names may change. World-only membership is valid. Unknown
  scope references are errors.
- Duplicating entities remaps their IDs and internal references while preserving
  membership. Duplicating a Scene creates a distinct scope. An import may
  reuse an ID with the same authored definition. A conflicting definition
  fails before mutation unless the user explicitly chooses a remap.
- Validate before changing entities, definitions, RNG or editor history.
  A failed operation preserves the prior in-memory state and the prior file.
  Temporary-file replacement alone cannot provide in-memory rollback.
- Read v1 with its original semantics. Opening, playing or rebuilding a v1
  project must not upgrade it. Writing data that v1 cannot represent requires
  an explicit migration. Preserve unknown fields, seed and authored override
  modes and values through a round trip.
- Preserve complete authored state across Play/Stop and code rebuild. A fragment
  used by copy or undo need only carry its affected data and referenced
  definitions. It cannot stand in for the full editor handoff.

The decisive editor case is to create an empty Scene, edit a World default
without saving, then build and reopen. The empty Scene, new value, selection
and unsaved state must survive. The [reopen handoff](../../editor/PixieEditor/Native/CodeReopen.cs)
now captures a complete authored snapshot, ordered selection, its anchor, dirty state and pending
attachment targets. Its version 2 restore uses a candidate World before editor bindings subscribe;
legacy version 1 handoffs remain readable. Qualified editor v2 fragment edits now
prepare a complete proposal on an unbuilt scratch World and publish a validated
candidate; raw live v2 restore is refused before destruction. The low-level legacy
`ReadEntities` path retains its documented registry-hook limits. Uncontracted
modules remain refused, and arbitrary project hooks or external effects are not
made transactional by candidate publication.

| Operation | State that must cross the boundary |
|---|---|
| Open/save a project | Keep `startScene` and the existing open/save route. A whole-document load restores the seed once. Scope creation, assignment and fragment import must not consume or reset simulation RNG. Preserve unknown root, scope, gravity, entity and component fields. Unsupported modes fail before mutation. |
| Duplicate, import, undo or redo | Keep scope definitions needed by the affected entities. `SceneIds::New` remaps entity IDs and references inside the copied set. Match scope IDs by authored definition; reject conflicting imports atomically. Other entities and World defaults survive the operation. |
| Play, Stop or Reset | The session creates a new World and runs the project's build hook again. Restore authored state for the intended mode and reacquire selections and references against the new revision. Never retain pointers into the prior World. |
| Build and reopen with unsaved edits | Compare a whole-authoring snapshot with an explicit handoff envelope carrying all defaults, scopes and selection. Either design must restore empty scopes, dirty state and pending code attachment targets. Failure leaves the saved scene and recoverable edits intact. |

Use the existing [scene round-trip, fragment and unknown-field tests](../../tests/runtime/SceneTests.cpp)
and [legacy motor migration tests](../../tests/runtime/MotorGravityTests.cpp)
as baselines. Invalid scope IDs and unknown gravity modes are structural
errors. Do not silently reinterpret them as inheritance or discard them
during a normal v1 save.

## Gravity, cameras and environments

The proposed modes are `Inherit`, `Multiply` and `Override`. If World
gravity changes from (0, 100) to (0, 200), an inheriting room follows it, a
room with multiplier 0.75 changes from (0, 75) to (0, 150), and a room with
override (30, 40) retains (30, 40). Zero is a value, not an inheritance
sentinel. Preserve the authored mode and inactive values through a mode
change, and compute the effective value natively. The Inspector can then
display both the effective vector and where it came from.

B proposes a force proportional to current mass × authored gravity scale ×
(resolved gravity − World gravity), converted into backend units. That is an
experiment, not an API guarantee. The [Box2D solver](../../external/box2d/src/solver.c)
adds force and gravity in ordinary integration, but its time-of-impact path
corrects fast-body velocity using global gravity alone. This is a concrete
reason to compare trajectories against native uniform-gravity runs before
claiming equivalence. The size of any gameplay-visible difference has not
been measured. The [Engine wrapper](../../runtime/src/CollisionWorld2D.cpp)
also wakes dynamic bodies on a global gravity change, and its public force
operation requests wake.

| Consumer | Qualification before accepting local gravity |
|---|---|
| Dynamic rigid body | Compare with a native reference under World zero, non-parallel overrides, mass edits and fast impacts. Apply scale once and any compensation once per complete physics step. Preserve sleep during unchanged sync, wake affected bodies once after effective changes, and preserve handles, contact continuity and velocity on transfer. Exclude static bodies, kinematics and proxies from compensation. Preserve `NeedsSimulation()`. |
| Particle system | Resolve per owning entity, including without a collision backend. Two pools may use different gravity in the same tick. Maintain physics-before-particles ordering and the existing per-pool scale. |
| Upright character motor | Check resolved gravity for that motor. Downward or zero gravity is supported; an unsupported lateral vector must produce a per-entity diagnostic without disrupting valid bodies or pools. |

Reject invalid modes, negative multipliers, nonfinite values and overflow
before writing native state.

**Owner qualification decision (2026-11-02):** Comparative tests are approved;
force compensation, solver changes and activation of local gravity for bodies
are not. Treat the following as initial experimental limits in backend physical
units, not guarantees of equivalence. Position error after one step must be at
most `0.1 * linearSlop`; after 60 steps, at most `0.5 * linearSlop`. Linear and
angular velocity error must be at most 0.1% relative to the reference, with an
absolute floor of 0.001 in the respective units. Contacts must have no missing
or spurious pairs, CCD must show no additional tunneling, and compensation must
not repeatedly wake sleeping bodies. Transfers must preserve identity, pose,
velocity and contact continuity. Run deterministic comparisons at 1, 60 and
600 steps, including zero and lateral gravity, mass changes, gravityScale, CCD,
cross-Scene contacts and transfer during simulation. Record the 600-step drift;
no numeric 600-step position limit has been approved. A small position error
cannot excuse contact divergence, tunneling or incorrect sleep behavior.
Justify any proposed revision of these limits with observations. If the
experimental technique fails, retain global gravity and the logical Scenes.

Favor one explicitly selected environment per viewport initially. If a view
shows two rooms, both use that environment by default. Separate passes could
give each room its own environment at the cost of defining compositing, shared
lights and post-processing order. Spatial blending would require region and
overlap rules. Keep both options open. Preserve the existing session camera
and editor navigation while evaluating authored camera references and fallback
when their targets disappear.

**Owner viewport decision (2026-11-02):** If a selected Scene reference is
removed, resolve that viewport through the World Environment and emit a
non-blocking diagnostic. Invalidate the live reference safely without
implicitly installing or enabling an absent capability. Retain enough authored
selection intent to restore the Scene Environment if undo restores the same
Scene identity. Distinguish definitive removal from temporary unloading; a
temporarily unavailable Scene must not be treated as permanently deleted.
Editor Scene selection does not change the viewport Environment.

## Repository responsibilities

| Owner | Responsibility |
|---|---|
| PixieEngine runtime and C API | Own identities, validation, effective values and transactions. Keep player, editor bindings and module version checks aligned. |
| PixieEngine editor | Select World, Scene or entity as distinct targets. Bind through native reads and edits, show authored and effective values, and renew bindings after reconstruction. Avoid creating placeholder entities. |
| PixieInspector | Supply generic binding, fields, read-only display and selection editing. A UI lock is independent of capability state. The editor defaults to the ImGui 0.3.0 package; local Inspector changes require explicit adoption. |
| PixieLib | Supply value types such as vectors, rectangles and colors. Engine uses the vendored C++ tree; Inspector references the managed package. Neither dependency automatically adopts changes from the sibling checkout. |

The audit baseline was Engine `ae598b6`, Inspector `c4dccc9` and Lib
`a23ca6d`. See the [editor dependency configuration](../../editor/PixieEditor/PixieEditor.csproj)
and [native dependency configuration](../../external/CMakeLists.txt).

The editor currently builds fields around a [ComponentHandle](../../editor/PixieEditor/Inspection/ComponentHandle.cs)
addressed by entity, type and field. The first expected integration change is
a World or Scene target in the Engine editor. Bind a distinct target through
the Inspector's existing manual binding, show authored values as editable
where appropriate, and show resolved values and their origin as read-only.
The native operation must validate a batch before applying its first write;
a multi-selection gesture should produce one undo action. Dispose or rebind
targets across Play, Stop and rebuild so old handles cannot edit a new World.

Qualify the default editor package path as well as any local Inspector
integration used for development. A change to PixieInspector requires a
demonstrated generic binding or presentation gap. A change to PixieLib
requires a missing reusable primitive and an adoption plan for its actual
consumers: the native Engine uses the vendored tree and the Inspector project
at the audit baseline references managed PixieLib 0.2.0. No sibling checkout
update by itself changes either dependency.

## Incremental migration plan

These are gates for the active work. The scope catalog does not require the
new gravity backend. Editable scopes do require working persistence and
reconstruction. The proposed order gives each change a consumer and a point
at which it can be stopped without converting the owner's project files.

**Order versus dependency.** Establishing typed World access first gives the
team a small migration to qualify against existing v1 content. It is not a
prerequisite for designing membership. Slice 2 has two distinct gates:
the in-memory catalog can proceed once identity and transfer rules are set;
durable authoring must wait for both format and reconstruction guarantees.
Gravity and cameras branch from those contracts independently. No blocking
defect found in the audit justifies moving input behind either branch.

| Slice and prerequisites | Planned change | Evidence required to advance; fallback |
|---|---|---|
| 0. Establish a baseline before edits | Record checkout, module ABI, editor dependency mode and worktree for each participant. Use copies of v1 scenes from LightMachine, LightLab, PhysicsLab and CouplingLab. | Record lit/unlit behavior, default gravity, motor and particle behavior, selection and Play/Stop. Preserve original files. Results from a sibling worktree or a local Inspector build do not qualify the default editor. |
| 1. Establish World authority once legacy conflict rules are agreed | Introduce typed native access backed first by existing v1 storage. Move one real consumer, such as LightMachine, to that access. Keep gameplay logic in the sample. | Absent, disabled and active-with-default capabilities are distinguishable. Player and editor resolve the same values. Duplicate settings have a documented outcome. Preserve the existing restriction on changing physics units after stepping. This slice can retain v1 reads and writes. |
| 2. Add logical scopes and qualify the selected persistence candidate | Add a scope catalog and explicit membership with World-only entities. Apply structural mutations at step boundaries. Stage document and fragment changes before publishing them to the live World. | Transfer a moving entity between rooms without changing its ID, pose, velocity or contacts. Reject removal of an occupied room. Round-trip empty scopes, unknown fields, seed and inactive override values. Exercise import conflicts, duplicate, undo and redo. Failure leaves the prior World and file intact. Keep v1 originals; require explicit conversion to write v2. |
| 3. Expose complete authoring only after the snapshot gates pass | Extend native operations, bindings, version checks and tools in the same slice. Add scope selection and settings history to the editor. Capture entire unsaved authorship for recompilation, including data with no entity owner. | Pass the empty-Scene rebuild case above, Play/Stop, save/reopen, stale binding rejection and atomic batch failure. Check normal Debug and Release player/editor flows and mismatched module diagnostics. If reconstruction fails, withhold the new write path and preserve recoverable state. |
| 4. Qualify local gravity once membership is stable | Implement the authored resolver independently of any per-body technique. Evaluate bodies, upright motors and particle pools separately. | Compare with equivalent native gravity under zero World gravity, lateral override, changed mass, sleep/wake, fast impact and different scopes in one solve. Apply gravity scale once; preserve contacts, solver-before-particles order and per-step cost. If tolerances fail, retain global physics and keep logical scopes available. |
| 5. Add camera and environment selection once the view policy is agreed | Resolve a safe camera reference and environment per view for drawing and picking. This slice can proceed independently of slice 4. | Exercise two viewports, entities from both rooms in one view, target destruction, Stop/rebuild and the free editor camera. Preserve the session fallback. Move lighting fields only after their authority is defined. |

For lighting, evaluate ambient light and fog as environment candidates,
exposure and bloom as view/pass candidates, and quality and light budgets as
renderer candidates. A point light remains entity data. These assignments
require consumer evidence before migration; moving the entire settings
component to World would merely relocate the ambiguity.

**Owner decision (2026-11-02):** Ambient light and fog belong to Environment,
with World defaults and optional Scene overrides. Exposure and bloom belong to
the view or render pass. Quality and resource limits belong to Renderer.
Resolve the Environment explicitly per viewport; selecting a Scene in the
editor never changes the rendered Environment by itself. Keep
`LightingSettings2D` v1 as a compatibility adapter with no second editable
authority. Preserve absent, disabled and configured lighting states and the
appearance of legacy projects. Set duplicate and old/new conflict rules before
migrating writes. This ownership decision does not require every field or
override in the first slice.

For each interop slice, record the native exports, managed binding, MCP probe
and project-module versions used together. At the audited baseline the native
and editor ABI was 20. Run the relevant acceptance against the actual player
and editor for that slice. A source build against a neighboring Inspector
checkout cannot establish that the default package works. Keep module-owned
callbacks and descriptors alive only while their DLL is loaded. A change
that would permit dynamic removal of systems would have to define teardown
and invalidation before exposing that operation.

### Independent TODO-056 track

[TODO-056](../../TODO.md#pixie-todo-056--editor-and-c-authoring-consolidation)
retains independent priority. A locally inspected consumer build resolved distinct
consumer and Engine source roots. Its compile log identified the project source,
generated component registry and authored mover component, shared by player and
module targets. This establishes source-tree provenance, not the exact Engine
commit used for those binaries. Private consumer names and machine paths remain
in local evidence.
The compiled mover polls A in Update and discards the result. Its hand-written
OnKey moves on a D press. Its saved start scene contains a shape and transform;
registration alone does not establish that the behaviour was attached in a
particular editor session. The finding concerns discoverability and generated
boilerplate. Its key binding and movement distance are not gameplay acceptance
criteria. Neither input redesign nor template cleanup depends on Scene
membership, v2 persistence or local gravity.

Tasks 225–227 cover contextual component generation, callback routing and
normal editor adoption. Today's [dispatcher](../../runtime/include/pixie/runtime/World.hpp)
gates device callbacks on enabled KeyboardControl or MouseControl; Update
still runs. The [template](../../templates/project-code/Component.cpp.in)
includes polling whose result is discarded. The promoted work can remove the
extra callback prerequisites and generate only selected callbacks while
preserving event order, opt-out, legacy scenes and hand-edited sources.
Preserve device order, repeat and modifiers, recheck enablement on delivery,
and keep legacy `KeyboardControl2D` scenes usable. Compare declared callbacks
with explicit subscriptions or filtering on the existing World. Editors should
distinguish creating source, attaching a compiled type, and rebuilding. An
uncompiled type remains visibly pending; attaching an existing behaviour
preserves its source. Qualify the loop in normal Debug and Release use and
bound delivery cost for unused handlers. The [input tests](../../tests/runtime/InputControlTests.cpp)
record the baseline; the promoted change deliberately revises the prerequisite
for receiving callbacks.

The actual coordination point is [task 229](../../TASKLIST.md#block-c--worldscene-integration-handoff-p03-proposal-7):
its editor/backend demonstration depends on a frozen contract. It does not
gate tasks 225–227 or viewport corrections. Coordinate shared headers, ABI
changes and rebuild handoffs at integration; keep backend acceptance separate.

## Decisions still requiring owner approval

- **Before migrating a settings group:** apply the lighting ownership decision
  above and choose ownership for any other group. Resolve conflicting old/new
  values before migrating writes. Preserve the v1 duplicate-read rule above
  and retain one writer during the transition.
- **Before enabling writes in the new format:** apply the approved v2 envelope
  and transactional conversion above; select the import conflict/remapping
  policy. Demonstrate rollback and integral editor reconstruction. These gate
  durable edits, not the in-memory scope model.
- **Before local physics is exposed:** use the approved initial comparative
  limits above to qualify CCD, contacts and sleep. The comparison authorizes no
  physical technique or body-gravity activation; approve a mechanism only after
  reviewing its measured results.
- **Before extending viewport policy:** use the approved World Environment
  fallback for removed Scene references. Separately composed passes or blended
  regions still require a rendering contract if a consumer needs either. The
  initial default is one Environment per viewport.
- **Deferred:** streaming, independent solvers/clocks, a generic registry and
  new shared packages need their own use cases. None blocks the first logical
  scope or the independent input work.
