# Pixie consumer handbook

**Document ID:** GLOBAL-CONSUMER-HANDBOOK
**Schema version:** 1
**Kind:** guide
**Lifecycle:** current
**Subject:** building a first game with the installed SDK and independent Editor
**Surface:** guide
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

This handbook takes you from a new project to a standalone export using the
public SDK and Editor.

Start with a compatible **PixieEditor application** and **public PixieEngine SDK**.
Build your game in its own folder. You do not need an Engine source checkout to
create, build or export a game. A runtime-only download is not an SDK.

## Read in this order

1. [Setup and compatibility](setup.md): what to obtain, which folder to select,
   which tools are needed and what the three native profiles mean.
2. Follow the [PixieEditor walkthrough](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/handbook.md#1-choose-the-sdk-and-create-a-project)
   to create, edit, save, undo/redo, Play/Stop, add code, rebuild and export.
   These numbered steps are available from the published Editor source guide.
   Use the walkthrough supplied with your Editor baseline when available.
3. [Gameplay in C++](gameplay.md): lifecycle, components, input, persistence and
   complete reusable movement examples.
4. [Drawing and interactive controls](drawing-and-controls.md): world graphics,
   a screen-space dashboard, mouse hit testing, rotation and text. This is the
   relevant route for a small 2D prototype with an upper play area and lower
   control panel.
5. [Public API map](api-map.md): discover the appropriate headers by purpose.
6. [Export and troubleshooting](export-and-troubleshooting.md): deliver a game
   folder and diagnose common failures without changing SDK contents.

The [initial validation record](../evidence/consumer-handbook-2026-11-06.md) and
[consumer follow-up](../evidence/consumer-handbook-2026-11-07.md) separate source
review, example execution, consumer reports and direct UI observations. Instructions are not proof
that every feature was exercised. This is a consumption guide for the inspected
baseline, not a guarantee that an arbitrary older or newer SDK is compatible.

## The first success

Your first milestone is a saved project that opens in the Editor, has an entity
you can modify and undo, and runs a small C++ behaviour in Play. Stop must return
to authored state. Then export to a new folder and run that folder's executable.
Finish this small loop before implementing the rest of the game.

An Editor scene is not a complete game by itself. New entities have no movement,
button actions, score system or game rules unless built-in components or your
C++ code supply them. The Editor has no general-purpose visual scripting system
or integrated C++ text editor in this baseline. **Open code** opens an external
editor. Edit the game source there, then return to the Editor to build and reopen.

## Terms you will meet

| Term | Meaning for a consumer |
|---|---|
| Project | Your folder: one `.pixie` manifest, scene files, assets and optional C++ code |
| World | The live entity/component simulation, clock, random state and systems |
| Entity | A handle to an object in one live World |
| Component | Data attached to an entity; reflected fields can be inspected and saved |
| Behaviour | A C++ component whose lifecycle/input/update callbacks run automatically |
| Scene file | Authored content loaded into a World; not a dump of arbitrary live program memory |
| Logical Scene scope | Optional v2 membership and policy grouping, independent of transform parenting and the viewport |
| SDK | Headers, libraries, build helpers, profiles, bindings and paired export tooling |
| Runtime | The native code and dependencies used to execute a game |
| Game module | Your authoring DLL, loaded by the Editor; not shipped as the game's code loader |
| Player | The executable in a standalone game export |

## Important baseline limits

- Windows x64, MSVC/C++20 and the selected SDK's exact toolchain identity are
  the installed native consumer baseline. The graphics baseline is OpenGL 4.5
  core. The experimental 4.3 VM renderer is outside this baseline.
- Write game logic in C++. The Editor's C# implementation supplies the authoring UI.
- Keep the generated Behaviour tutorial in v1. v2 introduces logical Scene scopes
  and restricted candidate authoring. v3 supports authored parenting in the
  unscoped family and requires a separate destination when changing scene family.
  [Scene versions and custom code](gameplay.md#scene-versions-and-custom-code)
- Compose game UI with `ScreenCanvas` and your game input/state. The inspected
  public surfaces provide no prefab workflow, general player widget toolkit or
  general audio API. Check the [public API map](api-map.md) for actual entry points.
- The public SDK excludes the private native automation channel.

## Handoff instruction for an assistant

For an assistant-led Editor walkthrough, use the optional
[assistant handoff](assistant-handoff.md#computer-use-workflow), which includes
the reusable prompt, UI evidence requirements and consumer report route.
