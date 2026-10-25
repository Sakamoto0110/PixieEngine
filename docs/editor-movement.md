# Scene Movement Authoring and Runtime Reuse

**Document ID:** RUNTIME-SCENE-MOVEMENT-GUIDE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** authoring collision/character components, session lifecycle and in-game laboratory boundaries

**Surface:** guide

**Boundary:** runtime

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

The public native owner is [SceneMovement2D.hpp](../runtime/include/pixie/runtime/SceneMovement2D.hpp).
The [low-level movement APIs](collision-movement.md) remain usable independently.
The editor and generic player now use this additional scene layer automatically.

## Using the editor

Open the [movement-lab project](../samples/movement-lab/README.md) without adding
game code. In the Entities panel, **Collider** creates a visible static box and
**Character** creates a motor with keyboard control and a placeholder visual.
The empty-space viewport menu also offers **New collider here** and **New
character here**. Each creation is one undoable step, including its components.

The **Physics...** menu (also under the viewport's **Physics** submenu) adds a
static obstacle, dynamic box, compound box, kinematic platform, sensor region, character or
world settings. A dynamic box already has Collider2D and RigidBody2D; a platform
starts with a 40 units/s horizontal velocity. A sensor is a nonblocking static
region. World settings selects the existing settings entity when one exists.
These presets configure the normal native components; they add no separate simulation.

In Edit, blue outlines show static geometry, gray outlines disabled colliders,
and green capsule outlines the character's collision body. Collider-only entities
can be selected through their gizmos. Shapes and colliders remain separate:
editing a Collider2D dimension changes collision, not the drawable Shape.
Transform edits affect both; character motors require unit scale and zero
rotation, with capsule size edited through radius/half segment instead.

The inspector provides native descriptor-driven settings and help text. For
Collider2D, **kind** chooses Box, Circle or Polygon; only the corresponding
geometry fields are used. Polygon points require a convex hull of 3..8 points.
Circle transforms require uniform absolute scale. Offset is transformed locally;
box/polygon geometry supports rotation and signed scale. Authored category/mask
fields use 31 nonnegative bits, matching the existing integer inspector.
Collider **size X/Y** shows full width/height; the saved `halfSize` values remain
half extents, preserving old geometry. Drawable Shape size is still independent.

Press **Play**, click the viewport for focus and use A/D or arrows and Space.
KeyboardControl2D is optional; without it, the motor still simulates gravity but
game code supplies movement through CharacterCommand2D. The placeholder Shape
is a rectangle; the green Edit outline shows the actual capsule. This is a
mechanical test scene, not final character art or accepted movement feel.

**Stop** restores the scene from before Play, including authored settings.
Saving during Play saves that authored snapshot, rather than runtime movement.
Undo/redo uses the editor's existing scene-ID history. Reopening a saved project
restores components/settings, not backend handles, velocity or buffered commands.

Invalid movement configuration appears in the Session panel. Invalid static
geometry is removed from collision until corrected, rather than retaining a
ghost of its previous shape. The engine also logs changed diagnostics. A
CharacterMotor2D owns its capsule; do not add a static Collider2D to the same
entity. Velocity2D/Acceleration2D do not integrate a motor-owned position.
Add component now disables conflicting motor/body/collider choices and shows
the native reason on hover. C API additions enforce the same rule; loading old
invalid scenes remains permissive so their components can be inspected/repaired.
World settings can be added to only one entity, including batch operations.

Motor capsule radius must exceed 0.01 times the world's units per meter after
backend float conversion (0.32 world units at the default scale of 32). Invalid
sizes are diagnosed in Edit and rejected before backend mover assertions. Increase
radius to recover; a fixed inspector minimum cannot cover every world scale.

## Compound bodies

**Physics... > Compound box** creates one dynamic body with CompoundCollider2D.
For existing geometry, use **Make compound** in Collider2D: the saved collider,
including unknown fields, becomes the first shape. RigidBody2D and drawable data
are retained. Conversion is one undoable operation. Collider2D and CompoundCollider2D
cannot coexist; the native admission query explains the conversion workflow.

Expand each **Shape** under CompoundCollider2D to edit the normal Collider2D
descriptor fields. **Add shape** appends a default box; the shape header's close
button removes it. Numeric gestures, points, conversion and list changes use the
existing scene-ID history. Select one entity to edit its list; batch item editing
is deliberately unavailable. Play/Stop retains the selected authored entity by
scene ID even though native handles change.

Each shape has its own local offset/rotation, material, density and bilateral
filter. Rotation acts before entity scale; rotated boxes under nonuniform scale
become convex geometry. Circles still require uniform absolute scale. Solid masses
add independently, including overlaps; sensors have zero mass. Shapes share one
body pose and velocity, while query/events retain separate shape identities.
Editing geometry rebuilds the body's shapes and preserves its current velocity;
reacquire native handles afterwards. Per-shape list indices change on removal.

Without RigidBody2D the compound is static. An empty/all-disabled list has no body.
Invalid geometry disables the whole body's collision until corrected, rather than
retaining a stale subset. There are at most 256 items. Native outlines and picking
cover separate shapes without inventing edges or collision across gaps. The legacy
MovementOutline2D returns the first valid outline; use MovementOutlines2D for all.
Drawable Shape/Path/Sprite data remains independent; no visual mesh is generated.
The [compound-lab save](../samples/compound-lab/README.md) includes a matching initial
L-shaped Path to make the compound visible in the generic player.

## Native lifecycle and ownership

ProjectSession calls InstallSceneMovement2D after loading and Project::Build,
then synchronizes once. Installation is idempotent. Both the C# host and native
generic player therefore run authored components without sample-only setup.
No Box2D scene is allocated when no movement/collider components exist.

PreUpdate synchronization provides a collider snapshot to game code. The
PostUpdate movement system refreshes changed geometry again after ordinary
velocity integration, then moves characters. Direct authored edits, removal,
entity reuse and reload are handled by the native layer. Motor retuning keeps
motion; externally changing its position teleports and clears runtime commands.
Edit synchronization validates/configures components but advances no time.

SceneCollisionWorld2D exposes a borrowed read-only query view of the last sync;
SceneColliderEntity2D maps a current collider hit back to an entity. Reacquire
hits after geometry edits. CharacterMotion2D exposes borrowed diagnostic state;
reacquire after synchronization/removal. SceneMovementErrors2D returns owned
diagnostic text. These APIs and authoring outlines/picking do not depend on C#.

C API **version 12** adds named layer queries, layer editor metadata and value-type
flags. Version 11 added reflected record lists and typed item access. Version 10
added admission queries and singleton metadata; version 9 added movement diagnostics
and Edit-mode gizmo picking/bounds.
Rebuild game C API libraries before loading them into the updated editor. The
scene format remains version 1: the new components are additive data, and old
libraries retain unknown components through the existing compatibility rules.

## Named collision layers

Collider2D **belongs to** and **collides with** now show checkboxes rather than raw
integers. The same controls appear in each compound shape. Membership needs at
least one bit; a collision mask may be empty. Mixed selections show mixed values,
and toggling a checkbox preserves each entity's other bits. Both objects must
permit a collision; naming a layer does not grant permission automatically.

Use **Edit layer names...** under either collider inspector to name slots. The
first committed edit creates one `collision layers` entity with CollisionLayers2D;
its inspector also edits names. This configuration is independent of world gravity.
Slots 1..31 always mean the same native bit, including the high bit at slot 31.
Renaming or clearing a name does not rewire any collider. Unnamed bits remain
visible as **Unnamed layer N**. Save/reopen, undo/redo and Play/Stop retain names
and filters. Undo of the initial configuration creation is one step.

The normal name UI never inserts or deletes bit positions. Imported duplicate or
out-of-range name records produce native diagnostics; invalid/duplicate records
can be removed from the configuration inspector. Unknown saved fields stay attached
to their original name record. An imported duplicate configuration must be removed
before authoring names. Duplicate display names are allowed; slot numbers distinguish them.

This delivers the naming/checkbox portion of PHY-011. The native query API still
accepts explicit numeric filters and sensor inclusion. Motor filtering and full
in-game query tooling are not added by this editor slice.

## An editor inside the game

A test laboratory is viable with the same native scene, component descriptors,
collision services, gizmo helpers, queries and scene/entity serialization. A
runtime UI can create/move/configure the real entities; a second simulation or
an embedded C# editor is not required. Use World::Defer for structural changes
made during a step, and define which inputs the tools consume before commands
reach a character.

The existing selection gestures, keyboard shortcuts and undo history are still
implemented in the C# editor. They have not become an embeddable runtime editor
in this delivery. A later laboratory UI should reuse native editing commands
and snapshots, extracting a shared command/history layer when that consumer is
implemented. The game decides which tools, components and constraints players see.

Define persistence and time policy explicitly: edit with simulation stopped,
edit a live world, or test a copy. ProjectSession's normal Play/Stop behavior
discards Play edits; a live laboratory needs an explicit commit/reset rule for
authored changes rather than accidentally saving transient character positions.
No full in-game editing UI or new GUI dependency is introduced here.

[Dynamic bodies and world settings](body-dynamics.md) now extend this scene
service, including [contact/sensor events](body-events.md) for physics shapes.
Player-capsule sensor registration, character-to-character collision, one-way platforms and
moving-platform attachment remain outside the current movement foundation.
