# PixieEditor

The C# editor of PixieEngine, first as a Dear ImGui application: it makes and opens projects, or
runs a game built in code, through a [C API library](../docs/modules/capi/README.md); it shows the
World in a docked viewport, lists its entities, and inspects and edits their components with
[PixieInspector](https://github.com/Sakamoto0110/PixieInspector).

## Projects

A project is a folder with one `<Name>.pixie` file and its scenes
([runtime reference](../docs/modules/runtime/REFERENCE.md#project-folders)). It holds no path to
the engine or to anything outside it, so it can be copied, versioned, or opened on another machine
as it is. The editor creates no version-control files.

- **Start screen.** Without `--game` or `--project`, the editor opens on New project, Open project,
  and the recent projects (a click opens one; a missing one is marked, with Forget).
  - **New project** takes a location, chosen in a folder picker drawn in ImGui (Up, the drives,
    the folders inside, and a path to type and confirm with Enter), and a name; it makes the folder
    `<location>/<name>`, which must not exist or be empty, and says why before Create can be
    pressed. The project takes the folder's name, and its scene starts with one shape.
  - **Open project** takes a folder that holds a `.pixie` file; project folders are marked in the
    picker, and Open waits until the current folder is one.
- A project runs on the shared engine's generic scene project, so it opens without a
  compiler. The recent projects and the place of the last one are kept in the user's settings
  (`PixieEditor/settings.json` under the application data folder), never in a project.
- **Edit and play.** A project opens in edit mode: time stands still, the time bar and Game wait,
  and the mouse selects. Edits change the World; **Save** (Ctrl + S, or File) writes the scene, and
  the window's title marks unsaved changes with `*`. **Play** builds the World again from the scene
  and runs it, with the mouse on Game; while playing, the title says so, the toolbar shows
  "PLAYING - changes are not kept", and the viewport is framed in orange. **Stop** goes back to the
  scene as it was at Play. Closing the window, or File, New or open project (a new editor on the
  start screen), with unsaved changes asks first: Save, Don't save, or Cancel.
- **Code.** A project can have C++ code of its own. **Code, Add code** writes `CMakeLists.txt` and
  `src/<Id>Project.{hpp,cpp}` into the project from the engine's templates, with an example
  Behaviour, `Spinner`, then restarts the editor on the project. Opening a project that has code
  builds it first, on a build screen that shows CMake's output as it comes: CMake configures
  `<project>/build` with the engine the editor runs from (`PIXIE_ENGINE_DIR`) and builds the
  editor's own configuration, leaving the game's player and C API library in `<project>/bin/<config>/`.
  The first build compiles the engine too, which takes a few minutes. CMake is taken from `PATH`,
  or else from Visual Studio, found through `vswhere`; a missing CMake or compiler is named, with
  what to install. When the build fails, the errors are shown in red, with **Try again** and **Open
  without code**, which opens the scene with the engine's own components; what the code adds stays
  in the file, untouched. The editor then runs on the game's library, so the game's components
  show in Add component and play in Play; the Session panel says which library is loaded.
  **Code, Build and reopen** builds again after changes to the code: P/Invoke binds one library per
  process, so the editor restarts, and the new one waits for the old to exit before building over
  its library. The game's player, `<project>/bin/<config>/<Id>.exe`, plays the project's start
  scene on its own.
- **Authoring.** Create entity, in the Entities panel, adds an entity named `entity <n>` to the
  scene, with a transform and a 24-unit circle, and selects it. In the inspector, Add component
  lists the types the selected entities do not all have, and a component's header has a close
  button that removes it from all of them. The inspector's first line is the selected entity's
  name, which can be typed over; Enter, or leaving the field, renames it. **Duplicate** (Ctrl + D,
  the Edit menu, or the viewport's menu) copies the selected scene entities 16 units to the side
  and selects the copies; references between the copies follow them, and the rest keep their
  targets. **Delete** (the key, or the menus) deletes the selection. The keys act unless a text
  field has the keyboard.
- **Sprites.** PNG files dropped on the editor's window become sprites, one per file, where the
  mouse is over the viewport (in the middle of the view when it is elsewhere), each 16 units to the
  side of the last, named after its file, selected, and as one step; other files are left out, and
  the status line says so. **New sprite here...**, in the viewport's menu on empty space, picks a
  PNG and places it at the click. A sprite is drawn at one world unit per image pixel; one that
  would be larger than half the view is given a smaller scale. An image from outside the project
  is copied into the project's `images/` folder first (an identical one already there is used
  instead of a second copy, and another file of the same name gets a number), and the status line
  names what was copied, so the project stays whole on another machine. The image of an open
  project's sprite that is saved again by a painting program shows at once.
- **Undo and redo.** Ctrl + Z undoes, Ctrl + Y or Ctrl + Shift + Z redo, and the Edit menu names
  what they would undo or redo. Every change to the scene in edit mode is one step: an inspector
  edit (a scrub, a drag on a slider, or a typed value, from press to release, over every selected
  entity), a move, creating, deleting, duplicating, adding or removing a component, and renaming.
  Steps name entities by scene ID, so they hold after the World is built again. The history lasts
  the editor session, through Play, Stop, and Save; changes made while playing are not steps, since
  Stop discards them, and undo and redo wait for Stop. Entities the game's code generates are not
  in the scene, and not in the history. Each step is logged.

- **Viewport.** The engine draws the World into a texture of the editor's own OpenGL 4.5 core
  context (`pixie_session_render`); the window shows it with `ImGui.Image`, flipped, since GL's
  first row is the bottom. The keys go to the game while the viewport has focus (click it first).
  The game steps at its fixed rate with the editor's wall time. Two buttons at the top choose what
  the mouse does:
  - **Game**: the mouse over the image goes to the game, in the image's pixels, as in its player.
  - **Select**: a click on a boid, an attractor, or a spawner selects it; Shift + click adds it to
    the selection or takes it out; a click on empty space clears the selection. A drag that starts
    on an entity moves it, and the whole selection with it when it was selected (otherwise it is
    selected first); Shift held during the drag keeps it to the axis it moved most along; with
    **Snap** on the toolbar, the dragged entity lands on a grid of 8, 16, 32, or 64 world units and
    the others keep their places around it. Each drag is one step of the history. A drag that
    starts on empty space selects every entity whose center is in the box (Shift keeps the rest).
    A right click opens a menu of what can be done at that point. On an entity, it joins the
    selection if it was not in it, and the menu duplicates or deletes the selection. On empty space, the menu offers Create entity here (a scene
    entity with a circle, at that point in world units); for a game, Ask the game to create one
    here (a boid heading somewhere at random in boids and flocks, a particle at rest in particles);
    and, where a particle system exists, Delete the particle here (the nearest within 6 pixels);
    and, with a Sprite component, New sprite here... (see Sprites).
    What the menu does is logged. Particles are never selected. The middle button and the wheel still go to the
    game, to pan and zoom; otherwise the game sees no pointer, so boids do not chase the cursor
    while you pick them. Selected entities are outlined: a sprite by its rectangle, turned as it is
    drawn, a text by its block of lines, a path along its line with its ends circled, anything else
    by a circle. What is drawn on
    top is picked first: shapes, then sprites, then paths, then sprites of negative layers.
- **Entities.** Every entity, oldest first, by name or number; only the rows in view ask the engine
  for a name. A click selects one, Ctrl + click adds or takes out one, and Shift + click selects the
  range from the last one clicked.
- **Inspector.** For the selected entities, one section per component that all of them have: a PixieInspector built from the
  engine's field descriptors (`Inspection/InspectorFactory.cs`), with one `AddField` per field (two
  for a point or a vector, four for a rectangle) over a handle of the component, and the label,
  tooltip, range, scrub, unit, and editor the descriptor carries. A color field edits with
  PixieInspector's color editor: its four channels, and a picker on its swatch. An entity field (a particle
  spawner's system or target) is a choice among the live entities that have the component it
  references, shown as "name #index", or "(none)". With several entities selected, the inspector
  is bound to the first and to the rest with `AddBind`, so an edit changes all of them; a field
  whose values differ shows "--", except a number that scrubs, which shows the first entity's value
  so a scrub moves each from its own. Every mixed row's label is amber, and its tooltip says the
  entities differ. The fields read and write the
  component through the C API on every frame; the native World is the only copy of the state. No
  class per component exists in the editor: a component added to a game's library shows up without
  rebuilding it. A text field, such as a sprite's `image`, is drawn above the rest of its section: a
  text box whose edit is one step when it lets go of the keyboard, "(mixed)" in its label when the
  selected entities differ, and, for a field that names a file (an asset), a **...** button that picks
  one of its kind: a PNG for an image, starting in the project's `images/` folder, a `.ttf` for a
  font, starting in `fonts/`, each copied into that folder from outside the project (an identical
  copy reused, a taken name numbered). Under an image, a preview with its size, or, in red, why it
  cannot be read (a missing file, not a PNG); under a font, in red, a file that does not exist. Adding
  a `Text` component gives it the project's first `.ttf` (in `fonts/` first), so it shows at once. A list of points, such as a path's `points`, is drawn there too: its count, a row of X and Y
  per point, typed over and written as one step when the row lets go of the keyboard, a **-** button
  per row that removes the point, and **Add point**, which adds one 32 units right of the last; each
  is one step. When the selected entities' lists differ, the field asks to select one.
- **Time.** A bar at the top slows time down, pauses or resumes, and speeds it up: 0.1, 0.25, 0.5,
  0.75, 1, 1.25, 1.5, 1.75, and 2 times. The fixed step stays 1/60 s; faster runs more steps per
  frame, so a heavy World (5,000 flocking boids) falls a little behind at 2 times.
- **Scenario.** Each game's starting values (counts, attractors) live in a scenario component on
  the `scenario` entity; its inspector has a "Rebuild the scene" button that builds the World again
  from the edited values.
- **Session.** The project and its folder, or the game; a single step while paused, Rebuild (for a
  project, from the scene's entities as they are) or Reset; the World's time, entity count, and step
  cost; and, for a game, buttons that open the other games built next to this one in a new editor
  (P/Invoke binds the library once, so one process hosts one game or project).

The editor's icon, on its executable and its window, is the neko mark of the shared asset store
(`vdr` alias `neko-purple-light`), copied into `PixieEditor/assets/` with `vdr cp` under its curated
name. The window takes its 32-bit frames as pixels through GLFW, so it shows on every host.

The editor references `PixieInspector.ImGui` 0.3.0 from nuget.org, which brings Hexa.NET.ImGui
2.2.9, with Hexa.NET.GLFW 1.0.5 and ImGui's GLFW and OpenGL3 backends 1.0.18
([`DEPENDENCIES.md`](../DEPENDENCIES.md)). Nothing in it is copied from PixieInspector's repository.

## Build and run

Build the engine first: the editor looks for `<game>-module.dll`, with
`pixie.dll` beside it; scene-only projects load the shared engine directly.
It searches `bin/Release`, then `bin/Debug`, above the directory it runs from. Then,
with the .NET 10 SDK:

```bash
dotnet build editor/PixieEditor.slnx -c Release
editor/PixieEditor/bin/Release/net10.0/PixieEditor
```

That opens the start screen; `--project <folder>` opens a project and `--game flocks` a game.

The dock layout is saved to `imgui.ini` in the directory the editor runs from.

## Options

| Option | Default | Meaning |
|---|---|---|
| `--project <folder>` | none | Opens the project in this folder, in edit mode; its code, if it has any, is built first |
| `--game <name\|path>` | none | `boids`, `flocks`, `particles`, or the path to a development game module; with neither option, the start screen |
| `--seed <n>` | `1` | Seed of a game's World; a project's comes from its scene |
| `--select <name>` | none | Selects the entity with this name at the start |
| `--set <Type.field=value>` | none | After `--edit-after` seconds, sets a field of the selected entity through its inspector, in the displayed unit (a point's axis is the field's name with `X` or `Y`; a list of points is `x,y;x,y;...`); repeatable |
| `--edit-after <seconds>` | `1` | When the `--set` edits happen |
| `--quit-after <seconds>` | `0` | Closes the editor after this long; `0` means never |
| `--screenshot <file.png>` | none | With `--quit-after`, saves the last frame of the window |
| `--time-scale <x>` | `1` | Starts with simulated time at this speed; with `--quit-after`, the simulated time is logged at the end |
| `--mode <game\|select>` | `game`; `select` for a project | What the mouse does in the viewport at the start |
| `--paused <on\|off>` | `off` | Starts with time paused, so a World stays as built |
| `--after <process id>` | none | Waits, up to 15 seconds, for that editor to exit before opening; the editor passes it when it restarts itself |
| `--save-after <seconds>` | none | Saves the project after this long, after the `--set` edits |
| `--do <command>` | none | After the `--set` edits, one per frame, as the editor's gestures would: `create`, `duplicate`, `delete`, `undo`, `redo`, `select=<name>`, `select+=<name>` (adds to the selection), `rename=<name>`, `move=<dx>,<dy>` (world units), `add=<Type>`, `remove=<Type>`, `save`, `play`, `stop`, `drop=<file>[;<file>...]@<x>,<y>` (as files dropped at that point of the view, in pixels), `image=<file>` (as chosen with the selected sprites' **...**), `font=<file>` (as chosen with the selected texts' **...**), `addpoint=<Type.field>` and `removepoint=<Type.field>@<index>` (as **Add point** and **-** on a list of points); the entities, the selection with each sprite's image and each path's points, the history, and whether the scene is saved are logged after each; repeatable |
| `--recent <on\|off>` | `on` | Whether the project joins the recent projects; `off` for scripted runs, so they leave the user's list alone |

The times of `--edit-after`, `--save-after`, and `--quit-after` count from when the game or the
project opens, after any build of its code; the build screen is never closed by them.

A `--set` on a button presses it: `--select scenario --set BoidsScenario.count=300 --set
BoidsScenario.rebuild=go` rebuilds the boids with 300. For example, `--game boids --select controls --set BoidControls.mode=Repel --set
BoidControls.radius=300` turns the boids away from a pointer with a 300-unit reach, as if edited by
hand in the inspector. With several entities selected, a `--set` changes all of them. Selection
gestures and the viewport menu's actions are logged.

## Local development MCP channel

Development builds accept `--dev-channel <name>` and
`--dev-capture-dir <folder>`. The former opts into one local Windows client;
the latter fixes its capture destination. The native pipe handle rejects remote
clients and uses a protected owner-SID ACL. Without the opt-in flag, no listener
starts. These channel options/types are excluded from a shipping editor publish.

`tools/pixie-mcp/` is the separate .NET 10 stdio server/client bridge. After
building its project in Release, an MCP client can launch it in this form from
the repository root:

```powershell
dotnet tools/pixie-mcp/bin/Release/net10.0/PixieMcp.dll --kind editor --host editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project samples/sprites --captures build/captures/editor
```

The server answers its client at once and opens the host in the background; the
first tool call waits for the host's channel. A project with code is built before
the editor opens its channel, and its first build compiles the engine too, so the
wait lasts up to `--startup-timeout <seconds>` (600 by default). A host that exits
first, or a deadline that passes, makes that call a tool error with
`possibly_applied: false`. The host runs in a Windows job of the server: it closes
with the server, also when the server is killed.

To register it with a client, use absolute paths, since the client chooses the
working directory. Claude Code, for the current user:

```powershell
claude mcp add pixie-editor -- dotnet C:/path/to/PixieEngine/tools/pixie-mcp/bin/Release/net10.0/PixieMcp.dll --kind editor --host C:/path/to/PixieEngine/editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe --project C:/path/to/project --captures C:/path/to/captures
```

Codex, in `~/.codex/config.toml`:

```toml
[mcp_servers.pixie-editor]
command = "dotnet"
args = ["C:/path/to/PixieEngine/tools/pixie-mcp/bin/Release/net10.0/PixieMcp.dll", "--kind", "editor",
        "--host", "C:/path/to/PixieEngine/editor/PixieEditor/bin/Release/net10.0/PixieEditor.exe",
        "--project", "C:/path/to/project", "--captures", "C:/path/to/captures"]
tool_timeout_sec = 900
```

A client's per-call time limit has to cover the first call's wait for a project
with code: `tool_timeout_sec` in Codex, `MCP_TOOL_TIMEOUT` (milliseconds) in
Claude Code. Each registration opens one project; `--kind player` with a game's
player, or `pixie-player`, serves only `world.*`.

Local qualification uses the official C# SDK client; a Claude Code client also
drove the editor, and shows the tools as `mcp__<server>__world_hello` (dots become
underscores). The player offers `world.*`;
the editor also offers `inspector.*` and `editor.*`. World writes use native
operations without Inspector history. Inspector writes use the same Set and
editing-command handlers as scripted runs, with actual recorder/history effects.
Use scene IDs and a current generation from `inspector.tree` for channel writes,
not names or runtime-index labels. Entity-reference inputs must also satisfy
their native References metadata.

Inspector observations distinguish held/view values from optional live/storage
reads. Custom text/point/record/mask rows identify their native-storage domain;
they are not advertised as generic held Inspector values. Receipts distinguish
pending, refusal, no-op, stale target and possible partial effects, preserving
command-local native failures before later reads. Handler true alone is not
write acceptance. Read-side failure diagnostics are separate from cached or
previous write failures.

Capture returns full-window PNG image content after a coherent subsequent UI
frame. Its alpha-one render does not pause or discard fractional time. Names
cannot escape the fixed folder and existing files are not overwritten. Export,
build, import/drop and font/image filesystem gestures are unavailable through the
channel, even though ordinary CLI/UI editing retains its own behavior. No
screenshot is an approved snapshot reference merely because a capture succeeded.

`editor.do` with `save` writes the scene through the same handler as Ctrl + S: in
Play it writes the scene as it was at Play, and its receipt reports `unsaved`.
`world.hello` reports it in `capabilities.save`. An editor built with
`-p:PixieChannelSave=false` leaves the gesture out, and the channel refuses it;
the editor's own Save is unchanged. World commands never save, in the player or
the editor.

`py tests/mcp/qualify_mcp.py --config Release` (or `Debug`) builds the bridge, its
test clients and both save variants of the editor, and runs, in disposable
projects under `build/tests/mcp/`: the player's real-pipe and MCP cases, the
editor's Inspector/history cases with and without save, the direct pipe and
fault fixtures, a killed server closing its host, the no-code physics authoring
workflow (presets, handles, layers, compounds, Play/Stop, save/reopen and a legacy
migration; see the [physics qualification](../docs/physics-qualification.md#no-code-editor-workflow)),
and a project with code opened
cold, which builds the engine again (`--skip-cold` leaves it out). For the `ci`
preset, pass `--bin build-ci/bin --work build-ci/tests/mcp`. See the
[qualification record](../docs/shared-engine-export-validation.md) for Windows
scope, executed cases and remaining project-wide gates.

## Collision and movement authoring

Open [movement-lab](../samples/movement-lab/README.md) as a no-code project.
Collider/Character creation buttons and matching empty-space viewport menu
entries use native reflected components and one undo step per creation.
Edit-mode gizmos show actual static geometry/capsules; invalid configuration
appears in Session. Play uses the same native services as the generic player;
Stop restores authored data. The editor requires the C API version it was built
for (18 now), so game libraries must be rebuilt with the engine. See [workflow and runtime laboratory reuse](../docs/editor-movement.md).
**Session > Physics debug** toggles collider/sensor outlines, origins, centers of
mass, velocities, awake/asleep markers and contact normals in Edit and Play.
These options belong to the session and do not change scene data. Use
`--do physicsdebug=127` for all options or `physicsdebug=0` for none; native
players accept `--physics-debug 127`. Velocity segments show 0.25 seconds of
travel; contact normals may include speculative rigid-body manifold points.
Enable **Collider handles** in Edit/Select mode with one entity selected. The
square resizes its collider and the circle rotates it; Shift snaps rotation to
15 degrees, Escape restores the pre-drag scene data, and release records one undo
step. For compounds, choose **Shape N** in the toolbar. Visual size and entity
transform remain unchanged. Polygons resize uniformly; circles have only radius
resize. Upright character capsules expose a lateral square for radius and a top
diamond for full height. Radius changes retain the straight segment; height
changes retain radius and center, with a minimum full height of twice the radius.
The radius respects the native world-scale minimum. Capsules cannot rotate.
Selecting one rigid body shows its native effective mass, solid area, local center
of mass and inertia as read-only values below its authored properties. The solid
area excludes sensors and adds overlaps. Disabled/invalid bodies show unavailable
data and direct the user to Session diagnostics. Multi-selection asks for one body.
The **Physics...** menu adds static obstacles, dynamic boxes, kinematic platforms,
compound boxes, sensor regions, characters and world settings. Conflicting motion components are
disabled with a native explanation; collider size shows full width/height without
changing saved half extents. Scripted commands accept `--do collider`, `character`,
`dynamic`, `compoundbox`, `kinematic`, `sensor` and `world`. `compound` converts the
selection, `addshape` appends a shape, `removeshape=<index>` removes one, and
`shape=<index>:<field>:<value>` uses the item's descriptor-built control (including
display units, for example `shape=1:rotation:30`). `wait=<seconds>` delays subsequent
script commands while the session keeps running. See [compound-lab](../samples/compound-lab/README.md).


Collision filters now use named **belongs to** and **collides with** checkboxes,
including compound shapes and mixed selections. **Edit layer names...** creates
or edits scene-native CollisionLayers2D configuration. Names use fixed slots
1..31; clearing a name never clears filter bits or changes world gravity.
See [named layer authoring](../docs/editor-movement.md#named-collision-layers).
Scripted verification uses `layername=slot:name`,
`layer=category|mask:slot:true|false` and
`shapelayer=zeroBasedShape:category|mask:slot:true|false` through the same authoring
and checkbox paths. These commands retain raw numeric save compatibility.
