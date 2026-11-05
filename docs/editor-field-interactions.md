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
Field interactions use the explicit local source or selected package mode
below. The ordinary Editor control still uses published
`PixieInspector.ImGui` 0.3.0 with field interactions disabled.

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

This selects the Inspector project reference and defines
`PIXIE_INSPECTOR_FIELD_INTERACTIONS`. With both Inspector selectors unset,
the Editor uses the published 0.3.0 control. Keep local-source outputs
separate from package outputs. Publish refuses the local source mode,
including with development tools disabled.

## Selected package mode

Select the exact D08 core and ImGui pair with
`-p:PixieInspectorPackageVersion=0.3.0-pixie021.3` and clear
`PixieInspectorLocalRoot`. Use the supplied locked offline feed and dependency
locks. The selected pair enables the same interaction feature and assets
through package references. The field fixture requires one of the two feature
selectors.

## Qualification

The native-backed fixture uses the real C API, descriptor-created nodes, view
interaction events, recorder, history, project serialization, and GL texture
loading. Core/view gestures are invoked through the view's interaction method;
this is not evidence of typed numeric input through the full editor. The
custom layer mask has paired enabled/locked mouse-input checks. Its fixed
540x900 Inspector fixture warms child-window hover state before pressing and
disables persisted ImGui settings. It also captures a real framebuffer for
owner visual review.

Use the Windows x64 `PixieEditor.FieldInteractionTests.exe` from the matching
fixture build in a desktop session with OpenGL 4.5. Restore the fixture and
Editor with the supplied lock files, offline feed and the same Inspector
selector. Use separate build, restore cache and output roots for the local
and package rows. Build with `PixieUseLocalConfiguration=false`,
`PixieDevelopmentTools=true` and `PixieNativeMetadataMode=Installed`.
Set `PixieNativeApiHeader` to the header from the supplied SDK.

Set `$fieldFixture` to the built fixture executable, `$privateSdk` to the
admitted private Debug SDK root, and `$privateCatalogSha256` to its expected
catalog SHA-256. Use an owned project root and a result JSON path with an
existing parent. For the native throwing-peer case, supply the absolute path
and expected SHA-256 of the matching private Debug WriteProbe module.

```powershell
& $fieldFixture --installed-sdk $privateSdk local-development Debug $privateCatalogSha256 `
  $ownedProjectRoot $ownedResultJson `
  --module $absoluteWriteProbePath --module-sha256 $writeProbeSha256
```

Omit the two module options for a runtime-only run. For the public runtime
case, use the Release fixture, the selected public SDK root, `authoring Release`,
and its expected catalog hash. Use the installed SDK form for all three cases
so the runtime and any module are admitted against the explicit selection.

| Fixture input | `privateChannelExecuted` | `writeProbeExecuted` |
| --- | --- | --- |
| Public authoring Release runtime | false | false |
| Private local-development Debug runtime | true | false |
| Private local-development Debug with matching WriteProbe | true | true |

The channel flag records execution of the private channel assertions. The
probe flag records execution of the native throwing-peer and Undo assertions.
The probe belongs to the existing development-channel tests. Inspector's
own core and ImGui suites qualify its generic propagation and input routing.
Package-mode fixture results identify the locked Inspector dependency graph
and selected installed runtime. Source isolation requires a separate
producer-access check. Owner visual review and maximum compound-list
selection performance retain their own evidence.
