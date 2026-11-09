# Consolidate the editor and C++ authoring workflow

**Document ID:** PIXIE-PROPOSAL-017

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** proposed priorities for usable C++ authoring, predictable editing and coherent editor surfaces

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## 1. State and owner direction

**State:** promoted on 2026-11-02 by the owner to
[`PIXIE-TODO-056`](../../TODO.md#pixie-todo-056--editor-and-c-authoring-consolidation).
**Source:** owner feedback and editor screenshots on 2026-11-02.
**Idea:** make the existing C++/editor workflow coherent and dependable before
adding more surfaces. Preserve the full C++ authoring capability the owner
now wants, while reducing the boilerplate and hidden setup around it.
**Promotion outcome:** accepted scope and gates now belong to `PIXIE-TODO-056`;
its linked TASKLIST section owns blocks A-G and task states. This proposal
preserves the originating rationale and priority map, not a second scheduler.

Owner direction retained in this proposal:

- C++ authoring remains a first priority. Its expansion beyond the original
  event-code viewer request is welcome and must be made usable.
- Treat section 4.1 as a substantial authoring rework, not a template-only cleanup.
  Behaviour input callbacks must not require KeyboardControl or MouseControl
  as an additional event-unlocking dependency. Group Behaviours coherently.
- Investigate the reported middle-click jump and the confusing fields/presets.
- World backend work is intended for the separately prepared Luna experiment.
- Putting Play/Game/Select controls inside the viewport is a candidate to
  evaluate, not an approved final layout.
- Improve documentation alongside code authoring. Advanced session telemetry,
  an editor asset manager and an engine/runtime resource manager come later.

This proposal unifies the product direction without replacing the technical
ownership of [C++ authoring](../entity-input-and-code-components.md),
[field interactions](../editor-field-interactions.md), or
[World/Scene scopes](world-and-scene-properties.md).

## 2. Observations and technical baseline

The owner reported a camera shift after a middle click with no intended drag.
The screenshots show a displaced view, narrow/clipped panels and a dense
Inspector. They do not establish whether the interaction changed camera pose,
entity transform, or both. Preserve this as a reported defect until a minimal
reproduction measures both; the root cause is not established here.

Current [entity creation](../../editor/README.md#source-provenance) (`editor/PixieEditor/Ui/Authoring.cs`) confirms that
Collider, Character and Physics actions construct component bundles. They are
creation presets, not additional ECS object kinds. The Character starter adds
a motor and a generic keyboard component; that does not itself define movement
bindings. The current input/Behaviour and code rebuild contracts are described
in the [authoring guide](../entity-input-and-code-components.md).

Passing builds and automated tests establishes bounded technical evidence.
It does not establish that the owner can discover dependencies, distinguish
editor modes or predict an action. Usability acceptance needs its own exercise.

## 3. Proposed priority order

| Priority | Outcome | Completion signal |
|---|---|---|
| P0.1 | Finish usable C++ Behaviour authoring and input | One documented create/attach/code/build/play loop works without hidden prerequisites or unnecessary generated handlers |
| P0.2 | Make viewport gestures predictable | A stationary middle click changes neither the camera nor any entity; focus, dragging and capture follow one explicit contract |
| P0.3 | Agree World/Scene integration boundaries | The bounded backend experiment and editor agree on state, identity, reset and property ownership; acceptance follows proposal 016 |
| P1.1 | Make Inspector fields explain their effect | Reviewed inventory covers active conditions, units, ownership, defaults and preserved inactive data |
| P1.2 | Organize creation, Session and viewport controls | Presets are named as presets; session state and input/edit tools are distinguishable in an owner-reviewed layout |
| P2 | Extend diagnostics and resource workflows | Add advanced telemetry and the two distinct managers after the core authoring loop is accepted |

P0.3 can proceed as a separately coordinated backend track. It must not turn
into a requirement to finish a World rewrite before fixing P0.1/P0.2. Document
each delivered slice as part of that slice, starting at P0.1. Defer unrelated
feature expansion while these first usability gates are unresolved.

## 4. C++ authoring and Behaviours (P0.1)

### 4.1. One clear workflow, ordinary C++ underneath

Owner clarification on 2026-11-02: expect a substantial rework of this workflow.
Preserve its useful C++ capabilities and user data, not the current setup burden
or UI mechanics. In particular, the existing KeyboardControl/MouseControl
prerequisite for Behaviour callbacks is not a constraint to retain. Coordinate
the event contract with section 4.2 as part of this same rework.

Keep reusable C++ types, actual project files, native registration/CMake,
external IDE support and IntelliSense. Offer event-oriented entry points:
choose a needed callback and navigate to or create that method. Generate only
the selected handlers, with a small starting example and the required public
headers. Keep a documented full template available without filling every new
Behaviour with unused polling examples and empty callbacks.

Document `Self()`, typed `Get<T>()`/`TryGet<T>()`, required includes, lifetime,
delta time and deferred structural mutation in the first working example.
When creating a new Behaviour from an entity selection, generate the public
headers for the components shared by that selection, using declared native
header provenance rather than guessed type-to-filename mappings. For example,
creation from an entity with Shape and Transform2D should include
`pixie/runtime/Shape.hpp` and `pixie/runtime/Transform2D.hpp` in its `.cpp`.
Report unavailable header metadata rather than inventing an include. These are
authoring conveniences, not required-component declarations: examples should
use `TryGet<T>()` unless the Behaviour explicitly establishes a requirement.
For multiple selected entities use their common component types, not the union.
Do not inject includes into an existing user-edited Behaviour when attaching
it to another entity, and do not generate gameplay mutations from the selection.
The current template does not yet perform this selection-aware header generation.
Evaluate a small public authoring header if it meaningfully reduces recurring
include friction; do not pull the entire engine or editor into every script.
Do not introduce a second script language, per-entity source copies or another
code store that competes with the C++ files. An event viewer is a navigation
surface over those files, not a second IDE implementation.

Distinguish three operations in the UI: define a reusable type, attach an
instance to selected entities, and compile/reload code. Recommendation: creating
a type for a selection carries an explicit pending attachment through the
existing rebuild path. Show the target entities, build state and attachment
result. Existing types can be attached without regenerating source. Whether
creation starts a build immediately remains a review decision; names and
confirmation text must match whichever behavior is chosen.

Build/registration/attachment errors remain visible until resolved, with the
source/diagnostic and retry action available. A missing compiled type must not
look like a successful attachment. Retain unsaved authored state, exact target
IDs and selection; preserve user C++ and project configuration.

### 4.2. Input should not require detective work

Owner direction: enabled Behaviour callbacks receive the input already admitted
by the host without requiring KeyboardControl or MouseControl to unlock those
callbacks. This supersedes the earlier conditional option to keep that prerequisite.
Optional input Behaviours may retain independently useful filtering/control
roles, but they must not become a hidden prerequisite for ordinary event handlers.
The rework must preserve intentional per-entity filtering/opt-out, avoid
broadcasting every event to unused handlers, and define migration for existing
scenes. Removing a setup burden must not introduce an unbounded dispatch cost
or silently change a saved project's input behavior.

Required presentation and diagnostics:

- Group attached Behaviours in one clear entity Inspector section. Keep each
  instance's enabled state, fields, source and removal action accessible.
- Show enabled state and any explicit input filtering in that same area.
  Do not scatter an essential switch among unrelated component sections or
  ask the user to attach another component merely to receive callbacks.
- Explain why a callback is not receiving input: editing, paused, viewport not
  receiving game input, disabled Behaviour, or an explicit input filter.

Grouping changes presentation, not ECS storage. Use reflected classification
rather than type-name guesses. Keep game input separate from text editing and
editor shortcuts, release held controls on focus/capture loss, preserve event
order and one-time fixed-step delivery, and avoid hardcoded gameplay bindings
in generic input. Examples must distinguish press/release/repeat from continuous
movement driven by held state and delta time.

## 5. Predictable viewport interaction (P0.2)

Define a small gesture contract before changing its implementation:

- Middle press/release at the same location is inert. Panning starts from the
  press position after a deliberate movement threshold, with a fresh baseline.
- A pan changes the editor camera only. Selection and entity transforms remain
  unchanged. A canceled or interrupted gesture leaves no stale drag state.
- Distinguish editor navigation, entity manipulation and game input. Toolbar
  clicks, text entry, another window's focus and captured pointer transitions
  cannot become unintended scene gestures.
- Verify coordinate conversion for the viewport content rectangle, docking,
  resizing and DPI. A local toolbar must not accidentally shift the input origin.
- Keep editor navigation separate from a future gameplay camera; give the
  editor a discoverable frame-selection/reset-view action.

Reproduce the report with camera pose and entity transform readback, not only
screenshots. Cover stationary clicks, sub-threshold motion, first entry into the
viewport, focus return, held-button capture loss and repeated pans. Check Edit
and Play, Game and Select, and a moved/resized dock. Do not attribute this defect
to World/Scene work without evidence of that dependency.

## 6. Inspector, creation presets and session controls (P1)

### 6.1. Fields that communicate their meaning (P1.1)

Use the required expansion of PIXIE-TODO-053 to inventory fields before broad
widget changes. For each field record its purpose, owner, units, default,
editable/computed status, valid range, applicability, interaction with peers,
and behavior through undo, type changes and save/reopen.

Start with the confusing combinations visible in the screenshots:

- Rendering shape and physical collider are distinct. Explain that difference;
  make deliberate matching/conversion an explicit action rather than silently
  changing physics when a visual kind or size changes.
- Show polygon points for polygon editing, radius where relevant, and box
  dimensions for box editing. Hide or clearly disable inactive fields with a
  reason. Preserve inactive/unknown serialized data for reversible changes.
- Label displayed full dimensions versus native half extents and local versus
  world offsets. Do not expect the owner to infer those from variable names.
- Present layer summaries compactly, with expansion for detailed masks and
  unnamed bits. Preserve raw bit values and names, including unknown bits.
- Keep chains and manual edit locks discoverable and distinguish their state
  from a read-only/computed or inapplicable field. Preserve existing history,
  multi-selection, cancellation and readback semantics.

Engine descriptors/native code own field meaning and validation; PixieInspector
owns reusable generic presentation/interaction; the Engine editor owns domain
composition. A generic library must not learn physics behavior from class names.

### 6.2. Entities, presets and Session (P1.2)

Recommend one discoverable creation entry with an explicit **Presets** group.
Describe each bundle and let the owner inspect/edit its resulting components.
An empty entity, static collider, dynamic body and character starter must not
look like unrelated object systems. If a character preset promises playable
movement, include a visible configured project Behaviour; otherwise label it
as a body/motor starter and state what remains to be added.

Keep Session focused on project/scene identity, authored/playing state, dirty
state, loaded code/build status and concise diagnostics. Leave space for later
telemetry through a dedicated expandable diagnostic surface, rather than
filling this panel with every metric now. Make panels readable at practical
widths and DPI; clipped paths need a useful inspection/copy affordance.

Recommend prototyping a compact toolbar at the top of the viewport:
Play/Stop/Pause/Step in one group, Game-input versus editing tools in another,
and snap/handles next to their manipulation tools. Keep global menus global.
Clarify which viewport/session receives each action, show active modes
unambiguously and adapt to narrow docks. The owner must evaluate the prototype
before this placement becomes an accepted layout.

## 7. World and Scene coordination (P0.3)

Use [proposal 016](world-and-scene-properties.md), its
[execution blueprint](world-and-scene-implementation.md) and
[independent acceptance rules](world-and-scene-acceptance.md) for the separately
prepared Luna experiment. This consolidation neither claims that experiment
has run nor replaces its design or reviewer.

Agree the editor/backend boundary for entity and scene identity, current scene,
effective versus inherited properties, Play/Stop/Reset, dirty authored data and
rebuild/reopen. Runtime owns World/Scene semantics; the editor consumes them.
Avoid separate UI-only copies of effective settings and avoid deriving the
entire object model from the convenience presets.

Integrate through a small reviewed handoff and a scene demonstrating those
contracts. Local viewport gesture correctness and C++ discoverability remain
independently testable. New loading/streaming policies are not prerequisites
for this consolidation.

## 8. Documentation and later managers

Documentation ships with every change. Start with a short path from a blank
scene to an entity moved by a small C++ Behaviour: creation, attachment,
includes/component access, input routing, build/reload, Play and save/reopen.
Give an event table with signatures, timing and input conditions, plus examples
of held movement, a discrete press and component access. Put explanations and
diagnostics at the point where the owner encounters missing input, missing
includes, failed builds or pending attachment. Link the technical reference
for lifecycle/storage details instead of making that reference the tutorial.

Keep the two later managers separate:

| Surface | Responsibility | Boundary |
|---|---|---|
| Editor asset manager | Browse, search, import, preview and assign project assets; expose broken references | Editor authoring workflow, consuming runtime resource identities |
| Engine/runtime resource manager | Load, cache, share, release and diagnose resources with explicit lifetimes | Runtime/game service, usable without editor UI |

Agree resource identity, project-relative references, missing-resource behavior
and reload/lifetime boundaries before building both. Existing asset loading
continues to serve the P0 exercise; these managers are not prerequisites for
the C++ or camera fixes. Advanced telemetry is also a later extension, using
the existing [telemetry service](../telemetry.md) rather than a competing one.

## 9. Delivery and acceptance

Proposed order: establish the baseline exercise; close C++/input and camera
failures; integrate the bounded World contract when accepted; review field and
preset semantics; validate the viewport/Session prototype; then expand managers
and telemetry. Deliver small usable slices under this single proposal.

The acceptance exercise must demonstrate:

1. Create an entity, attach a reusable Behaviour, open correct source and
   obtain working IntelliSense without discovering hidden setup through logs.
2. Use one documented event and access an existing component. Creating a new
   Behaviour from a Shape/Transform2D entity supplies both public headers; a
   multi-selection supplies only shared components. Attaching an existing
   Behaviour preserves its source bytes. Dependencies are either unnecessary
   for that path or explicitly shown/configured.
3. Build/reopen with unsaved changes and the same selected IDs. Verify actual
   attachment, then Play/Stop and save/reopen without losing authored changes.
4. Middle-click without dragging, pan, change focus and resize the viewport.
   Read back camera and entity state and confirm only intended changes.
5. Create a physics preset, explain its component bundle, change visual and
   collider geometry deliberately, and identify every active/inactive field.
6. Undo/redo, cancel a field gesture and reopen saved data. Run the compiled
   game through the supported path; editor conveniences must not supply hidden
   gameplay or resource dependencies.

For each promoted slice require a regression that detects its failure, relevant
build/ABI checks, evidence from the normal editor outputs, the owner's hands-on
UX review and the updated short guide. Full native/MCP matrices are selected
by affected contracts, not blindly repeated for every documentation/layout
change. Automated qualification and independent review do not replace that
hands-on acceptance. Claims must state the tested configuration and remaining
limits.

## 10. Decisions before promotion

Promotion is recorded above. For execution, settle only the decisions needed
for the first slice: input delivery/filtering and compatibility under the
no-extra-controller requirement; Behaviour grouping and
minimal event template; create/attach/build semantics; the gesture contract;
and the World/Scene integration handoff. Review toolbar placement in a prototype.
Keep the asset/resource managers and advanced telemetry deferred.

Relate approved work to existing scheduler owners: PIXIE-TODO-055 for its
authoring scope, PIXIE-TODO-053 for the gated field inventory/overhaul, and
PIXIE-TODO-054 for preserving the existing interaction work. Promote genuinely
new scopes explicitly; do not silently broaden or close those entries. This
proposal does not independently change code, API, archive or release state;
the promotion's scope and gates are owned by `PIXIE-TODO-056`.
