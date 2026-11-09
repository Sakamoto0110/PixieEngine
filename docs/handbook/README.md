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

This handbook is for a game developer, including an assistant starting without
the project's conversation history. It explains how to use the existing public
API. It does not introduce a new API or require changes to PixieEngine.

Start with a compatible **PixieEditor application** and **public PixieEngine SDK**.
Build your game in its own folder. You do not need an Engine source checkout to
create, build or export a game. A runtime-only download is not an SDK.

## Read in this order

1. [Setup and compatibility](setup.md): what to obtain, which folder to select,
   which tools are needed and what the three native profiles mean.
2. PixieEditor's **`docs/handbook.md`**, supplied alongside this handbook: create
   a project, edit, save, undo/redo, Play/Stop, add code, rebuild and export using
   the UI. The Editor owns this walkthrough; the Engine's
   [Editor entry point](../../editor/README.md) locates its independent repository.
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
editor. An assistant restricted to mouse clicks in PixieEditor cannot author
arbitrary C++ gameplay; it needs access to an allowed code editor or file tools.

## Terms you will meet

| Term | Meaning for a consumer |
|---|---|
| Project | Your folder: one `.pixie` manifest, scene files, assets and optional C++ code |
| World | The live entity/component simulation, clock, random state and systems |
| Entity | A handle to an object in one live World |
| Component | Data attached to an entity; reflected fields can be inspected and saved |
| Behaviour | A C++ component whose lifecycle/input/update callbacks run automatically |
| Scene file | Authored content loaded into a World; not a dump of arbitrary live program memory |
| Logical Scene scope | Optional v2 membership/policy grouping; not a transform hierarchy or another viewport |
| SDK | Headers, libraries, build helpers, profiles, bindings and paired export tooling |
| Runtime | The native code and dependencies used to execute a game |
| Game module | Your authoring DLL, loaded by the Editor; not shipped as the game's code loader |
| Player | The executable in a standalone game export |

## Important baseline limits

- Windows x64, MSVC/C++20 and the selected SDK's exact toolchain identity are
  the installed native consumer baseline. The graphics baseline is OpenGL 4.5
  core. The experimental 4.3 VM renderer is not a consumer fallback guarantee.
- Game logic uses C++. The Editor's implementation language, C#, is not an
  embedded game scripting language.
- Ordinary generated Behaviour projects use the supported **v1 scene** path.
  Do not convert one to v2 just because the Editor offers conversion. v2 custom
  authored components and candidate reconstruction have a restricted contract;
  see [gameplay](gameplay.md#scene-versions-and-custom-code).
- There is no built-in parent/child transform hierarchy, prefab workflow,
  general UI-widget toolkit for the player, or general audio API in the public
  surfaces inspected here. Compose simple game UI using `ScreenCanvas` and
  your own input/state logic. Do not invent `Button`, `AudioSource`, `GameObject`,
  `OnCollisionEnter`, or other APIs based on familiarity with another engine.
- Public authoring does not expose the private native automation channel.
  Computer use can operate the public Editor; it does not require enabling a
  private SDK or changing shipping exclusions.

## Handoff instruction for an assistant

Give the assistant this handbook, the Editor walkthrough, the example sources,
the executable and SDK locations, a writable game folder and your game scope.
Use the following instruction as a starting point:

> Read the supplied Pixie consumer handbook and Editor walkthrough first. Build
> the game described in my separate scope using the installed SDK and public
> Editor. Keep game source/assets in the game folder and export to a separate
> output folder. Computer Use is mandatory for Editor
> interactions, including project creation/opening, Inspector edits, component
> attachment, save/history, Play/Stop, code generation, rebuild and export.
> Do not replace these with CLI/MCP/direct scene-file changes and call that UI
> work. Use your allowed external code editor/file tools for C++ source. If
> Computer Use is unavailable, report the block instead of substituting a route.
> Begin with the first-success loop before adding mechanics. Check the selected SDK headers
> when a symbol or signature is uncertain. Preserve generated registry files;
> create components through the Editor and edit their user-owned source files.
> On a continuation, reopen the existing game and reuse its Editor build cache.
> Use scratch builds for specific diagnostics and export when the standalone
> check or handoff is ready.
> Report missing capabilities instead of inventing APIs or modifying the
> Engine, SDK or Editor. Report compilation, observed gameplay, save/reopen and
> export separately. Save a `CONSUMER-REPORT.md` in the game folder using the
> supplied report template; distinguish your observations, code review, user
> reports and unchecked items. Do not publish anything.

The game scope supplies the actual rules, control bindings, layout dimensions
and success conditions. A sketch is a layout reference, not authority for
unwritten mechanics. A simple rotating-object prototype does not inherit a
larger laboratory's subsystem simulation or engineering requirements.

Examples are optional building blocks, not additional game requirements. A
clickable HUD does not require physics sensors; particles and lighting are only
needed when the supplied scope asks for them. Choose examples that serve the
game and explain any adaptation in the [consumer report](consumer-report-template.md).
