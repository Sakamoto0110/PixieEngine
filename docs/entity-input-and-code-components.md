# Entity input and C++ components

**Document ID:** PIXIE-ENTITY-INPUT-CODE-COMPONENTS

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** generic entity device input and preserving C++ component authoring

**Surface:** guide

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

This implements the bounded [PIXIE-TODO-055](../TODO.md) scope. Input components
do not define movement, physics, dimensions, or game actions. The game decides
what a device event means. Code and registration remain C++, using the existing
`Behaviour`, `Entity::AddBehaviour`, reflected components, and `pixie_game` path.

## Device contract

`KeyboardControl` and `MouseControl`, in
[`InputControl.hpp`](../runtime/include/pixie/runtime/InputControl.hpp), are
ordinary per-entity components with an `enabled` field. The keyboard offers
`IsDown(input, key)`, `WasPressed`, `WasReleased`, and `Modifiers`. The mouse
offers the equivalent button queries plus `Position`, `Movement`, `Scroll`,
and `Modifiers`. Pass the `const Input&` supplied to the current callback or
Update. The component retains no input pointer or per-entity copy of the key
arrays. An absent component cannot be queried through that entity; a disabled
component reports neutral state: false buttons/keys, zero vectors/position,
and no modifiers. The existing low-level global Input remains unfiltered for
compatibility and must not be described as a component-filtered query.

Enabled components opt the entity's Behaviours into these optional callbacks:

```cpp
void OnKey(const pixie::KeyEvent&, const pixie::Input&) override;
void OnMouseButton(const pixie::MouseButtonEvent&, const pixie::Input&) override;
void OnMouseMove(const pixie::MouseMoveEvent&, const pixie::Input&) override;
void OnMouseScroll(const pixie::MouseScrollEvent&, const pixie::Input&) override;
```

For each Behaviour instance, Start runs once before callbacks, then events are
delivered in host order before Update. The matching component and its enabled
flag are checked for each event; disabling keyboard does not disable mouse or
the Behaviour's Update. There is no global cross-entity event ordering promise.
Removing/destroying through `World::Defer` retains the existing end-of-step
lifecycle. Until deferred destruction completes the entity remains readable;
Stop runs once if Start ran. Never retain component/event references across
steps or capture event references in deferred work: copy the needed values.
`World::Step` rejects reentry, including from its deferred actions.

Key args carry `key`, `action`, `modifiers`, and `scanCode`, with `Pressed()`,
`Released()`, and `Repeat()`. Mouse button args carry `button`, `action`,
`modifiers`, and event-time `position`. Move args contain position/delta;
scroll args contain offset/position. Mouse positions are host viewport pixels,
not inferred world coordinates. Modifier bits follow GLFW; cursor/scroll use
the host's last reported modifier state.

Input stores a single ordered event sequence, including press-release-press
and repeat between updates. Polling describes the final accumulated tick
state; event args describe each intermediate transition. EndUpdate consumes
the events, edge bits, movement, and scroll once, retaining held state. Hosts
must consume updates; a pause retains pending events until an update consumes
them. Focus/modal cancellation discards pending actions, releases previously
held controls once, and retains that cancellation until EndUpdate. Old-World
transitions are discarded when ProjectSession rebuilds a World; held state
can still be polled. Editor start/build screens never accumulate game input.
The viewport admits events under its focus/hover/capture policy and transforms
event-time pointer positions into its own pixel coordinates.

## Legacy compatibility

Serialized `KeyboardControl2D` retains its name, enabled value, and old
WASD/arrows/Space behavior through the explicitly named
[`LegacyKeyboardControl2D.cpp`](../runtime/src/LegacyKeyboardControl2D.cpp)
adapter. It is marked legacy in the editor; it is not renamed to, or treated
as, generic KeyboardControl. SceneMovement2D consumes its compatibility
command without owning its key mapping. New character authoring adds generic
KeyboardControl, so game C++ must supply meaningful commands. New generic
controls never imply a platformer or an action map. Games may remove the
legacy component after implementing their own consumer code.

ABI 20 originally introduced ordered device events, explicit type-source
metadata and changes to the C++ Behaviour, descriptor and input layouts.
That delivery preserved scene component names and format. For current builds,
read `PIXIE_CAPI_VERSION` in the selected SDK's `include/pixie_capi.h` and
the [SDK identity and module admission contract](modules/capi/REFERENCE.md#sdk-identity-and-profiles).
Rebuild game modules and players for the selected SDK/profile/toolchain.
Incompatible development modules are refused with a rebuild diagnostic.

## Create and edit a component

See the standalone [PixieEditor code-authoring guide](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/code-authoring.md)
for the Editor UI, rebuild handoff and VS Code workflow. The
[Editor walkthrough on creating and editing a component](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/handbook.md#4-create-and-edit-your-own-component)
covers what survives a rebuild and reopen. The reopened Editor starts a
fresh undo history. The [Editor migration guide](../editor/README.md)
records the extraction.

The selected Engine SDK supplies the C++ templates and CMake generation
helpers. Installed `PixieAddCode.cmake` creates the initial project files and
registry after checking the destinations. The Editor owns the later Add
component workflow and reuses the SDK templates. Names are ASCII C++
identifiers of at most 64 characters, starting with a letter. The generated
registry calls each component's `Describe`. In the standard registered
layout, `src/PixieComponents.generated.hpp` declares the registry and
`src/PixieComponents.generated.cpp` implements it. The source list is
`.pixie-code/Components.cmake`. The generator tracks all three files by hash
in `.pixie-code/manifest.json`. Project CMake uses
`pixie_project_components()` to attach the listed sources to its game library.
Preserving generation retains user component sources and CMake text outside
the managed block. It refuses conflicts and retains
preimages of replaced owned files under `.pixie-code/backups/`.

The separate `.pixie-code/sdk.json` records the portable SDK requirement
(SDK ID, authoring profile, platform/architecture, compiler version and CPU
features). Keep machine-specific paths and build-cache pins outside this file.
Package-mode generators validate an existing requirement. Initial adoption
requires explicit `PIXIE_PROJECT_ADOPT_SDK=ON`. The helper creates a missing
requirement only with its owned generated outputs. Existing incompatible
requirements require a separate explicit adoption workflow.

## Adopt an existing custom C++ project

Untouched old Add code templates are recognized as a complete three-file
baseline and migrated explicitly. A customized, unfamiliar registration body
is not rewritten by filename. To adopt it, the owner adds this include and
call in their actual Project::Describe implementation:

```cpp
#include "PixieComponents.generated.hpp"
// Inside the existing Describe(ComponentTypes& types):
pixie::generated::DescribeComponents(types);
```

Keep the existing Describe logic. After the project's `pixie_game(...)` call,
add exactly one managed block, preserving the rest of CMakeLists.txt:

```cmake
# BEGIN PIXIE COMPONENT SOURCES (managed)
pixie_project_components()
# END PIXIE COMPONENT SOURCES (managed)
```

Initialize the registry from the selected authoring SDK, explicitly
naming the project ID. The example also authorizes writing a missing SDK
requirement.

```powershell
$sdk = 'C:/path/to/selected-sdk'
cmake -DPIXIE_PROJECT_DIR=C:/path/to/game -DPIXIE_PROJECT_ID=MyGame `
  "-DPIXIE_SDK_ROOT=$sdk" -DPIXIE_SDK_PROFILE=authoring -DPIXIE_PROJECT_ADOPT_SDK=ON `
  -P "$sdk/cmake/PixieInitComponents.cmake"
```

The helper refuses existing registry files and leaves the owner's C++ and
CMake implementation intact. Source-checkout contributors can run the same
script from the Engine tree with explicit `-DPIXIE_ENGINE_MODE=SOURCE`
instead of the package-selection/adoption arguments. Rebuild and use Add
component normally. A custom multi-game build must wire its registry
explicitly. The automatic Editor path expects one game library/module for
the project.

## Qualification boundaries

`tests/runtime/InputControlTests.cpp` and `tests/engine/InputTests.cpp` qualify
ordering, state, disabled/absent controls, lifecycle, fixed-update retention,
and reentry. PixieEditor's `tests/code-components/` qualifies generation refusals and
byte preservation, a real multi-component CMake consumer, C API metadata and
callbacks, scene save/reopen and handoff, the production Add component popup
through ImGui input, and viewport routing. Normal editor/player smoke results,
configuration-specific builds, and owner visual acceptance are recorded
separately. An isolated helper build alone is not normal editor adoption.
