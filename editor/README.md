# PixieEditor

The C# editor of PixieEngine, first as a Dear ImGui application: it runs one game through the
game's [C API library](../docs/modules/capi/README.md), shows the game in a docked viewport, lists
its entities, and inspects and edits their components with
[PixieInspector](https://github.com/Sakamoto0110/PixieInspector).

- **Viewport.** The engine draws the World into a texture of the editor's own OpenGL 4.5 core
  context (`pixie_session_render`); the window shows it with `ImGui.Image`, flipped, since GL's
  first row is the bottom. The keys go to the game while the viewport has focus (click it first).
  The game steps at its fixed rate with the editor's wall time. Two buttons at the top choose what
  the mouse does:
  - **Game**: the mouse over the image goes to the game, in the image's pixels, as in its player.
  - **Select**: a click on a boid, an attractor, or a spawner selects it; Shift + click adds it to
    the selection or takes it out; a drag selects every entity whose center is in the box (Shift
    keeps the rest); a click on empty space clears the selection. Ctrl + click deletes the entity
    under the cursor, or else the particle within 6 pixels, and on empty space asks the game to
    create something there: a boid heading somewhere at random in boids and flocks, a particle at
    rest in particles. Particles are never selected. The middle button and the wheel still go to the
    game, to pan and zoom; otherwise the game sees no pointer, so boids do not chase the cursor
    while you pick them. Selected entities are outlined.
- **Entities.** Every entity, oldest first, by name or number; only the rows in view ask the engine
  for a name. A click selects one, Ctrl + click adds or takes out one, and Shift + click selects the
  range from the last one clicked.
- **Inspector.** For the selected entities, one section per component that all of them have: a PixieInspector built from the
  engine's field descriptors (`Inspection/InspectorFactory.cs`), with one `AddField` per field (two
  for a point or a vector, four for a rectangle) over a handle of the component, and the label,
  tooltip, range, scrub, unit, and editor the descriptor carries. An entity field (a particle
  spawner's system or target) is a choice among the live entities that have the component it
  references, shown as "name #index", or "(none)". With several entities selected, the inspector
  is bound to the first and to the rest with `AddBind`, so an edit changes all of them; a field
  whose values differ shows "--", except a number that scrubs, which shows the first entity's value
  so a scrub moves each from its own. Every mixed row's label is amber, and its tooltip says the
  entities differ. The fields read and write the
  component through the C API on every frame; the native World is the only copy of the state. No
  class per component exists in the editor: a component added to a game's library shows up without
  rebuilding it.
- **Time.** A bar at the top slows time down, pauses or resumes, and speeds it up: 0.1, 0.25, 0.5,
  0.75, 1, 1.25, 1.5, 1.75, and 2 times. The fixed step stays 1/60 s; faster runs more steps per
  frame, so a heavy World (5,000 flocking boids) falls a little behind at 2 times.
- **Scenario.** Each game's starting values (counts, attractors) live in a scenario component on
  the `scenario` entity; its inspector has a "Rebuild the scene" button that builds the World again
  from the edited values.
- **Session.** A single step while paused, reset, the World's time, entity count, and step cost, and buttons that open the other games built next to this one in a new editor
  (P/Invoke binds the library once, so one process hosts one game).

The editor references `PixieInspector.ImGui` 0.2.1 from nuget.org, which brings Hexa.NET.ImGui
2.2.9, with Hexa.NET.GLFW 1.0.5 and ImGui's GLFW and OpenGL3 backends 1.0.18
([`DEPENDENCIES.md`](../DEPENDENCIES.md)). Nothing in it is copied from PixieInspector's repository.

## Build and run

Build the engine first: the editor looks for `<game>-capi.dll` in `bin/Release`, then `bin/Debug`,
above the directory it runs from. Then, with the .NET 10 SDK:

```bash
dotnet build editor/PixieEditor.slnx -c Release
editor/PixieEditor/bin/Release/net10.0/PixieEditor --game flocks
```

The dock layout is saved to `imgui.ini` in the directory the editor runs from.

## Options

| Option | Default | Meaning |
|---|---|---|
| `--game <name\|path>` | `boids` | `boids`, `flocks`, `particles`, or the path to a game's C API library |
| `--seed <n>` | `1` | Seed of the World |
| `--select <name>` | none | Selects the entity with this name at the start |
| `--set <Type.field=value>` | none | After `--edit-after` seconds, sets a field of the selected entity through its inspector, in the displayed unit (a point's axis is the field's name with `X` or `Y`); repeatable |
| `--edit-after <seconds>` | `1` | When the `--set` edits happen |
| `--quit-after <seconds>` | `0` | Closes the editor after this long; `0` means never |
| `--screenshot <file.png>` | none | With `--quit-after`, saves the last frame of the window |
| `--time-scale <x>` | `1` | Starts with simulated time at this speed; with `--quit-after`, the simulated time is logged at the end |
| `--mode <game\|select>` | `game` | What the mouse does in the viewport at the start |
| `--paused <on\|off>` | `off` | Starts with time paused, so a World stays as built |

A `--set` on a button presses it: `--select scenario --set BoidsScenario.count=300 --set
BoidsScenario.rebuild=go` rebuilds the boids with 300. For example, `--game boids --select controls --set BoidControls.mode=Repel --set
BoidControls.radius=300` turns the boids away from a pointer with a 300-unit reach, as if edited by
hand in the inspector. With several entities selected, a `--set` changes all of them. Selection
gestures and Ctrl + click actions are logged.
