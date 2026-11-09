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

The C API/module ABI is **20** for the ordered event ABI, explicit source
metadata, and changed public C++ Behaviour/descriptor/input layouts. Rebuild
game modules and players against this engine. Incompatible development
modules are refused with a rebuild diagnostic; old binaries must not be run
against the new shared engine. Scene component names and format do not change.

## Create and edit a component

The Editor UI, rebuild handoff and IDE workflow are documented in
PixieEditor's `docs/code-authoring.md`. See the [Editor migration guide](../editor/README.md).

The Engine SDK provides the C++ templates and CMake generation contract.
Names are ASCII C++ identifiers of at most 64 characters, starting with a letter.
The generated registry calls each component's Describe. The owned registry,
header and source list are tracked by hash in `.pixie-code/manifest.json`.
Project CMake uses `pixie_project_components()` to attach those sources to its
game library. Generated changes preserve the user's component source and the
CMake text outside the managed block. Conflicts are refused and replaced owned
files have preimages in `.pixie-code/backups/`.

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

Initialize only the new registry files, explicitly naming the project ID:

```powershell
cmake -DPIXIE_PROJECT_DIR=C:/path/to/game -DPIXIE_PROJECT_ID=MyGame `
  -P C:/path/to/PixieEngine/cmake/PixieInitComponents.cmake
```

That command refuses existing registry files and does not modify the owner's
C++ implementation or CMake. Rebuild and use Add component normally. A custom
multi-game build must own its explicit wiring; this automatic editor path
expects one game library/module for the project.

## Qualification boundaries

`tests/runtime/InputControlTests.cpp` and `tests/engine/InputTests.cpp` qualify
ordering, state, disabled/absent controls, lifecycle, fixed-update retention,
and reentry. PixieEditor's `tests/code-components/` qualifies generation refusals and
byte preservation, a real multi-component CMake consumer, C API metadata and
callbacks, scene save/reopen and handoff, the production Add component popup
through ImGui input, and viewport routing. Normal editor/player smoke results,
configuration-specific builds, and owner visual acceptance are recorded
separately. An isolated helper build alone is not normal editor adoption.
