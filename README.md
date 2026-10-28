# PixieEngine

[![CI](https://github.com/Sakamoto0110/PixieEngine/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/Sakamoto0110/PixieEngine/actions/workflows/ci.yml)

A 2D-first game engine: a C++ runtime on OpenGL 4.5 core, a standalone player
executable, and a C# editor that inspects engine state through
[PixieInspector](https://github.com/Sakamoto0110/PixieInspector). Primitives come
from [PixieLib](https://github.com/Sakamoto0110/PixieLib).

Early stage: Core, Platform, Render and App native boundaries, with Runtime
and C API being packaged into a shared engine, the samples under [`samples/`](samples/) (three of them the anchors:
boids, flocks, and particles; `sprites` a project without code), and the C# ImGui editor under [`editor/`](editor/README.md),
which runs any of the anchors. Shared-engine/export/MCP work is in progress;
the [qualification matrix](docs/shared-engine-export-validation.md) distinguishes
completed checks from pending distribution and development-channel delivery.
From the Visual Studio 2026 developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-debug
bin/Debug/boids
```

Then the editor, with the .NET 10 SDK:

```bash
dotnet build editor/PixieEditor.slnx -c Release
editor/PixieEditor/bin/Release/net10.0/PixieEditor --game boids
```

The configure step also writes `PixieEngine.slnx` at the root. Building,
checking, and committing are in the [development guide](docs/development.md).

In the native player, F3 shows FPS/UPS and F3+C opens a command bar at the top of the window.
Try `-vsync false`, `-fps-cap true 60`, or `-fps-cap false`; Enter runs the command and Escape
closes the bar. See the [player debug controls](docs/player-debug.md) for details.

F3 also displays registered CPU metrics. `--telemetry on|off` or `-telemetry true|false` controls
collection independently of visibility. Each World owns its collector; see the
[telemetry guide](docs/telemetry.md) and [neutral sample](samples/telemetry/README.md).

The [movement room](samples/movement/README.md) exercises the private Box2D collision backend
and engine-owned capsule controller. See [ownership, units and current limits](docs/collision-movement.md).
The [no-code movement laboratory](samples/movement-lab/README.md) opens in the editor and generic
player; [scene authoring](docs/editor-movement.md) shares the native services between both hosts.
The [body dynamics laboratory](samples/physics-lab/README.md) adds falling/rotating boxes,
world gravity and a kinematic platform; [current dynamics limits](docs/body-dynamics.md)
keep character coupling and remaining authoring gates explicit.
The [compound laboratory](samples/compound-lab/README.md) adds an authored L-shaped
body and editor controls for multiple shapes on one body, with save/history support.

- [`ROADMAP.md`](ROADMAP.md): what the engine is and is not, and where it is going.
- [`TODO.md`](TODO.md): promoted work. [`TASKLIST.md`](TASKLIST.md): its task breakdowns.
- [`DEPENDENCIES.md`](DEPENDENCIES.md): every dependency, its status, and candidates.
- [`docs/`](docs/README.md): documentation index, policy, development guide, proposals, and archive.

## License

PixieEngine is released under the [MIT License](LICENSE). Vendored dependencies keep their own
licenses, listed in [`DEPENDENCIES.md`](DEPENDENCIES.md).
