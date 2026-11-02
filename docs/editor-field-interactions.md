# Editor field chains and manual locks

**Document ID:** PIXIE-EDITOR-FIELD-INTERACTIONS

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** local editor adoption of numeric chains and manual edit locks

**Surface:** guide

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

This is the bounded implementation of `PIXIE-TODO-054` in [TODO.md](../TODO.md).
It does not authorize the wider, gated Inspector overhaul in `PIXIE-TODO-053`.
The implementation requires the explicit local source mode below. The normal
editor still consumes the published `PixieInspector.ImGui` 0.3.0 package.

## Editing behavior

Each supported leaf shows a lock, a chain control or empty chain slot, then
its label. Tooltips describe the current state. The original supplied lock
and chain textures are copied to the editor output and owned by its native
session's image cache. PixieInspector supplies a drawn fallback if textures
are unavailable; it does not own the renderer or load the host's assets.

The first chain click arms one field without changing its value. Clicking
another eligible field joins that group. Right-clicking any active member
clears the entire group. A scrub adds the same total displayed delta to each
member's own starting value, separately for every selected entity. Absolute
entry writes the same displayed value. The core visits each member once;
the host declares eligibility and does not implement a second propagation
path.

Eligible groups are the X/Y axes of the same point or vector, or the position
and dimension pairs of a rectangle separately. Shape size and Collider2D size
can share a group on the same selected entities. With one entity selected,
the size axes of its compound collider records are eligible too. Collider
`halfSize` metadata displays full dimensions with a factor of two; propagation
therefore takes place in displayed units, rather than copying native half
extents into Shape. Position, scale, offsets, and dimensions are separate
groups. Scalar values without compatible peers have no chain control.

A manual lock blocks direct user input, including scrubbing. A locked member
already in a chain can still receive propagation. Actual read-only metadata,
conversion, validation, and setter refusals remain effective. Programmatic
`SetValue`, the development channel, and the existing scripted editor handlers
retain their previous semantics: they neither propagate nor consult a UI-only
lock. Chain and lock toggles alone do not write native state, dirty the scene,
or create history.

Native descriptor-built scalar rows, including compound collider properties,
use the generic Inspector controls. Host-owned text/assets, point lists, and
layer masks reuse `DrawEditControls` with a surrogate Inspector leaf that owns
only the manual lock. Their original writers remain in place. A point list is
locked as a collection, including its add/remove controls; its variable-length
coordinates are not members of numeric chains. Compound record editing retains
its single-selection restriction. Entity names and structural component/list
buttons are not numeric field chains.

## History and lifetime

The view's interaction events bracket the existing recorder. Scalar interactions
keep before/after values for the native fields actually written; unrelated
entity fields are not replaced by scalar undo. Record-backed members capture
an entity snapshot before the whole interaction, including any preceding Shape
writes. Existing field and record steps are composed into one history entry.
Redo applies fields before record replacement; undo uses the reverse order and
resolves final selection handles through stable scene IDs. Scalar history
retains unchanged bindings and their UI state. No-op and successfully restored
cancellation produce no history step. Results with refused members are reported
as partial; actual effects remain undoable. This is not an atomic multi-setter
transaction.

Escape requests restoration through the core gesture. A failed restoration
must retain its reported failure and actual changes; the host does not claim
rollback from a successful event callback alone. The native project's unsaved
flag is sticky: cancellation can restore all values without a history step
while leaving an already modified scene marked unsaved. It does not save or
clear unrelated dirty state automatically.

Selection, shared component changes, record structure changes, and native
binding revision changes rebuild the inspectors and discard UI state. Native
Play, Stop, Reset, Restore, destruction, and component/list structural changes
finish open interactions before replacing bindings. The structural world.*
development-channel branch invokes that hook before native JSON dispatch too. Views released afterward
abandon their gesture without replaying a rollback into a replacement World.
No chain/lock configuration is serialized into scenes.

## Explicit local source mode

For normal Visual Studio Debug/Release builds on a development machine, the editor
also imports the optional, unversioned `.local/PixieEditor.props` file. Setting
`PixieInspectorLocalRoot` there enables the same integration in the usual output
directories, without supplying a command-line property for each build. Reload the
project after changing this configuration. `-p:PixieUseLocalConfiguration=false`
skips this local import when validating the default package build or publishing.
Local-source publishing remains refused by the existing guard.

Use the Inspector checkout that contains the editing API. The root property
is not saved as a machine-wide or repository default:

```powershell
$inspectorRoot = 'C:/path/to/PixieInspector'
dotnet build editor/PixieEditor/PixieEditor.csproj -c Debug `
  -p:PixieInspectorLocalRoot=$inspectorRoot `
  --artifacts-path .local/validation/field-interactions/build
```

This swaps the package reference for a project reference and defines
`PIXIE_INSPECTOR_FIELD_INTERACTIONS`. Omitting the property restores the
published dependency and excludes the adoption code. Do not reuse one artifacts
directory between local-source and default-package builds. Publish explicitly
refuses this local source mode, including with development tools disabled.
There is no new package version, release, or native scene format.

## Qualification

The native-backed fixture uses the real C API, descriptor-created nodes, view
interaction events, recorder, history, project serialization, and GL texture
loading. Core/view gestures are invoked through the view's interaction method;
this is not evidence of typed numeric input through the full editor. The
custom layer mask has paired enabled/locked mouse-input checks. Its fixed
540x900 Inspector fixture warms child-window hover state before pressing and
disables persisted ImGui settings. It also captures a real framebuffer for
owner visual review.

```powershell
dotnet run --project tests/editor/field-interactions -c Debug `
  -p:PixieInspectorLocalRoot=$inspectorRoot `
  --artifacts-path .local/validation/field-interactions/test-build -- `
  bin/Debug/channel-write-probe-module.dll `
  .local/validation/field-interactions/native-debug `
  .local/validation/field-interactions/debug-results.json
```

Repeat with Release and its matching native module. Passing `pixie.dll` instead
skips the test-only throwing peer case; it does not prove native refusal. The
probe module belongs to the existing development-channel tests. Inspector's
own core and ImGui suites qualify its generic propagation and input routing;
their evidence remains separate from Engine integration evidence. Maximum
compound-list selection performance and owner readability acceptance are not
implied by the functional fixtures.
