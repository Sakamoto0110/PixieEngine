# Native entity parenting in PixieEngine and PixieEditor

**Document ID:** PIXIE-PROPOSAL-023
**Schema version:** 1
**Kind:** reference
**Lifecycle:** current
**Subject:** native 2D entity parenting, hierarchy authoring, persistence and staged acceptance
**Surface:** proposal
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

## Status and intended outcome

**State:** promoted by the owner on 2026-11-07 under
[PIXIE-TODO-061](../../TODO.md#pixie-todo-061--native-entity-parenting).
The owner asked
to plan native parenting for both products before changing Weapons-Lab, to
validate it first in a simple scene, and to use Weapons-Lab for acceptance
afterward. The initial implementation starts with the in-memory runtime
contract. The validation order below governs the later integration slices.

An author should be able to assemble a group of entities, move a parent and see
its descendants follow, and edit each descendant's local pose. The assembly
should survive save/reopen, Undo/Redo, Play/Stop, code rebuild and export. A
project that has never used parenting should open with its existing poses and
authoring behavior.

The motivating game has a fixed root, a rotating assembly and repeated parts
with separate state. That establishes a need for identity and composition.
Inspector field groups can make a long component easier to read and can be
designed independently. Prefabs, arbitrary nested Inspector records, nested
scene assets, joint simulation and inherited gameplay policies are outside this
proposal.

## Existing contracts and ownership

The current [Transform2D](../../runtime/include/pixie/runtime/Transform2D.hpp)
is a world pose in a Y-down coordinate system. Existing consumers rely on that
meaning. Game code can already store entity references and calculate the poses
of related objects. The [scene format](../modules/runtime/REFERENCE.md#scenes)
stores a flat entity list with stable `SceneId` values. The format preserves an
unknown `parent` key without applying a relationship. The Editor lists entities
and has independent UI for logical Scene scopes. Scene membership defines scope
policy under [PIXIE-PROPOSAL-018](world-scene-reconciliation.md).

| Owner | Proposed responsibility |
| --- | --- |
| Engine runtime | Parent relations, admission, local/world pose evaluation, child queries and lifetime rules |
| Engine scene and snapshot paths | Stable relationship identity, round trip, validation and remapping during restore and copy |
| Engine C API and SDK | Versioned query and mutation surface, diagnostics and bindings for an independently built Editor |
| PixieEditor | Entity tree, selection, reparenting interaction, local/world authoring affordances and history integration |
| Game | Meaning of attachments and signals, availability, degradation and any simulation driven by those signals |

Keep the [installed SDK boundary](engine-sdk-editor-separation.md) and
[independent Editor boundary](standalone-editor-extraction.md). The Editor
gets authoritative relationships and poses from the selected SDK. It owns tree
expansion and selection state. SDK compatibility and capability admission must
be explicit, including the Editor's behavior when its selected SDK lacks the
feature. The [D09 freeze](../../ROADMAP.md#d09-glfw-distribution) applies.

## Proposed runtime contract

- **One parent per child.** Relationships form a forest in one World. Nodes
  retain their own entity identity, components and lifetime. The parent edge
  is authoritative. Derive child enumeration from it and define a stable
  sibling order. Entity names can repeat and serve as labels.
- **Admit before mutation.** Reject self-parenting, cycles, dead or missing
  entities, cross-World references and unsupported poses or movement owners.
  For the first slice, authored links stay in the same document and the same
  logical scope where explicit scopes apply. Treat a scope change that would
  invalidate an edge as a rejected operation until the user detaches the edge
  or an explicit compatible branch operation is available.
- **Use one authored pose per node.** An unattached entity keeps its existing
  world-space `Transform2D`. A child has an authoritative pose relative to its
  parent. Engine evaluation produces the child's world pose. Existing
  world-space readers must continue seeing a world pose. Choose the concrete
  storage, API and binding path together. Direct writes to child world
  transforms need an explicit admission policy and a supported edit route.
  They must never be silently accepted and then overwritten by evaluation.
- **Start with representable 2D transforms.** Proposed first slice inherits
  translation, rotation and positive uniform parent scale. Leaves retain the
  scale range supported by the admitted transform path. Reject an attach or
  scale edit that would require an unsupported parent scale, a singular
  inverse or a pose the runtime cannot represent. Existing flat entities keep
  their current scale behavior. Affine shear and nonuniform parent scale can
  be evaluated as later extensions.
- **Define when poses are current.** Evaluate a parent before its descendants,
  independent of entity creation order. Establish visibility of edits for
  simulation consumers and publish a consistent world pose for rendering and
  picking. Specify how previous poses are captured and reset for reparent,
  restore and teleport. Interpolate the authored poses and compose them
  through the hierarchy, or demonstrate an equivalent result. The fixture must
  catch a one-frame lag or a visible separation during parent motion.
- **Bound the first slice's movement support.** Start with a code-driven parent
  and visual children edited through the supported local-pose path. Reject
  independently simulated bodies, colliders and movement owners on attached
  children until their synchronization and write ownership have a reviewed
  contract. Preserve existing motion of flat entities. Additional movement
  combinations require an explicit support matrix and tests.
- **Handle runtime and authored lifetimes.** Runtime code can use the same
  relation operations within their defined mutation phase, including a
  deferred operation where required. A parented authored entity requires a
  persistable parent in the same authored document. Runtime-only children can follow a
  parent in Play and are rebuilt by game code when needed. Session restore
  returns to the captured authored graph.

The game can read the relation graph in both directions. For example, a parent
can publish availability to its descendants, and a descendant can report a
fault to its owning assembly. The project defines how it evaluates those
signals, how faults clear and when an aggregate status changes. Engine
parenting supplies the relation and query order. Automatic fault propagation
is outside the runtime contract.

## Authoring and structural edits

The Entities panel lists roots. The Inspector shows a compact tree immediately
above **Add component**. An expanded node shows its children and, when it is
the sole selection, its component panels. Clicking a node selects the same
entity as viewport picking. Right-click a tree row for **Set parent** and **Duplicate
branch**. The parent picker preserves the world pose. The Inspector's
Transform2D fields display and edit world values.

| Operation | Recommended first behavior |
| --- | --- |
| Attach or change parent | Preserve the current world pose by default. The native API also supports keeping the local pose. The Editor keeps that mode out of the parent picker. A multi-selection keeps edges within the selected branches and reparents their roots. A rejected operation leaves the entire selection untouched. |
| Detach | Make the entity a root while preserving its current world pose. Its descendants remain attached to it. |
| Move or rotate | A root edit moves its branch. A child local edit changes its pose within its parent. Viewport manipulation resolves the edit into the appropriate authored space. For a selected ancestor and descendant, one move acts once on the selected branch. |
| Delete | A normal delete removes the selected entities. Surviving children whose parent was deleted become roots with their world poses preserved. Any subtree delete is an explicit command. Undo restores the original IDs and edges. |
| Duplicate | Keep the current selection semantics for ordinary duplication. Remap parent edges within the copied set and keep an admissible external parent. An explicit branch copy includes descendants. Apply any placement offset once to the copied roots. |

Use the Editor's existing authoring transaction and history path. Each accepted
operation records the complete relevant edge and authored-pose change as one
Undo entry. Invalid or cancelled edits leave the World, selection, dirty flag
and history cursor unchanged. Keep tree selection and expansion coherent after
an operation or reconstruction. In Play, use the existing authored-edit policy
and keep runtime changes scoped to the live World.

## Persistence and compatibility

Keep stable authored IDs for edges and resolve them after all admitted entity
identities are known. Input order must have no effect. A hierarchy document
stores the parent identity and the child's authoritative local pose. An
evaluated world pose is derived data. A whole document with a missing parent,
duplicate identity, cycle or inadmissible pose must fail before publication.
Provide a diagnostic naming the affected entity and relation. Preserve the
previous scene and current authored session on failure.

The [existing snapshot contract](../modules/runtime/REFERENCE.md#entity-snapshots)
distinguishes restore with kept IDs from copy with new IDs. Apply that
distinction to parent links. When copying a selection, the target of an
external edge may be resolved only to an admitted entity in the destination
World. For a standalone clipboard or fragment whose parent is absent, require
an explicit detach-with-world-pose operation or reject the paste. The chosen
fragment behavior must preserve Undo data for the original edit. Snapshot
round trips cover Play/Stop and the unsaved code rebuild handoff. Undo history
retains its current lifetime across process replacement.

The first persisted slice uses version 3 with `parent2D` stable IDs and local
`Transform2D` fields on children. It retains the legacy component admission
route for custom C++ components. Version 2 scope and presentation ownership
stays separate. The Editor enables the hierarchy commands for version 3 projects with a supporting SDK.
Legacy flat v1 and v2 files remain readable with their existing transforms and
scope semantics. Files that require parenting must be distinguishable before
an older reader can silently load them flat. Use an explicit recognized format
version or an equivalent admission mechanism that demonstrably rejects the
file on unsupported readers. Introducing a `parent` key alone is insufficient.
Writing a hierarchy to an existing file requires explicit adoption and a
recoverable original where a format transition is needed. Saving a flat project
does not automatically convert it.

Maintain a supported route for the existing v1-style C++ Behaviour projects.
The current [v2 custom-code admission rules](../handbook/gameplay.md#scene-versions-and-custom-code)
are narrower. A file-format decision must specify how such projects adopt
parenting while preserving components and values. This proposal does not
expand v2 admission implicitly. The standalone player and exporter must admit
the same authored hierarchy that the Editor saved, using compatible paired
SDK artifacts.

## Validation order after promotion

### First qualification with a simple scene

Use one small scene with ordinary visible shapes and no gameplay dependency.
Use the same fixture through runtime, Editor and player checks. Start with this
three-level branch. Use the alternate root for cross-root runtime checks.
Use only Root in the Editor walkthrough and change links within its branch.

```text
Root           world position (100, 100), rotation 0, scale 1
└── Pivot      local position (20, 0), rotation 0, scale 1
    ├── A      local position (0, 10), rotation 0
    └── B      local position (10, 0), rotation 0
```

With the root rotated a positive quarter turn, the expected world positions
are Pivot `(100, 120)`, A `(90, 120)` and B `(100, 130)`. Use exact identity
checks and a stated numeric tolerance for poses. Use distinct colors or shapes
so a viewer can identify both leaves independently.

| Order | Check | Acceptance evidence |
| --- | --- | --- |
| 1 | Runtime and authored persistence | Verify the expected poses, root movement, local edits, keep-world/keep-local reparent, detach and interpolation. Permute entity input order. Round trip IDs, edges, poses and scope. Exercise restore and copy remapping, including an external parent and a missing target. Check flat v1/v2 regression scenes and unsupported-format refusal. |
| 2 | Rejected operations and native boundary | Exercise self-parent, descendant-as-parent, dead/absent target, cross-World or cross-scope edge, invalid scale and unsupported movement. Compare snapshots and selected state before and after rejection. Verify the public SDK route through the actual C API and its bindings, with a matched SDK and an unsupported capability case. |
| 3 | Editor and exported player | Use the actual Editor UI to create the branch, inspect and edit a child, reparent and undo/redo, duplicate, delete and undo, save and reopen. Check Play/Stop, export through the menu and run the player. Add a minimal SDK Behaviour to the fixture for the code-bearing path and rebuild with an unsaved hierarchy edit. Check the retained edit after reopen and the matching code-bearing export. Record which surface produced each result. |

The simple scene qualifies the feature before any Weapons-Lab migration. A
code-bearing path is required before accepting a code-bearing game. A temporary
diagnostic build may help isolate a failure, and its results stay separate from
the installed SDK and Editor/player checks. Reuse compatible build caches and
keep one final fixture export with its receipt, as described in the
[development guide](../development.md#build-and-rehearsal-storage).

### Later acceptance with Weapons-Lab

Start only after the simple fixture passes the applicable checks and the owner
authorizes the game change. Pin the chosen game source, scene, Editor and SDK
identities and a known-good player export. Keep that baseline available for
comparison. Use the existing game project as the consumer, with a recoverable
working revision for the adoption.

- Represent the fixed root, rotating assembly and repeated attachments as
  native parented entities. Verify that each attachment retains its own
  authored identity and state and follows the intended parent pose.
- Verify the scene opens after save, builds and reopens through the Editor,
  and plays and stops with the expected authored tree. Check that reopening
  and copying the chosen branch do not leave duplicate or dangling attachments.
- Exercise existing gameplay behavior affected by the pose changes. If the
  approved game scope includes availability or fault signals, observe one
  scenario per direction and record its project-defined outcome separately.
- Export to a new folder with the selected SDK and exercise the affected
  interactions in the player. Record paths, hashes, UI observations, any
  untested cases and friction in `CONSUMER-REPORT.md`.

## Decisions to settle by slice

1. Confirm the initial transform limits and the admitted movement owners.
2. Choose how the runtime exposes authored local and evaluated world poses,
   including write admission and update/interpolation timing.
3. Choose the scene/fragment representation and the compatibility path for
   existing C++ games, then review the SDK and Editor capability boundary.
4. Confirm the proposed authoring defaults for reparent, detach, delete and
   duplication. Establish the required command surface before adding gestures.

The runtime slice can proceed with the conservative visual attachment limits.
The authored format and SDK capability decision belong to the persistence and
native integration slices. Feature qualification and game acceptance have
separate evidence and completion conditions.
