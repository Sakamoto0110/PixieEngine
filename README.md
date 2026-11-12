# PixieEngine

Workspace contributors: start with the [Engine filesystem map](FILESYSTEM.md) for
local ownership and the [Pixie workspace entry](../../.governance/workplaces/void-workplace/README.md)
for applicable shared rules and cross-project navigation. These documentation routes
do not make a workspace or sibling checkout a build/runtime dependency.

[![CI](https://github.com/Sakamoto0110/PixieEngine/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/Sakamoto0110/PixieEngine/actions/workflows/ci.yml)

A 2D-first game engine with a C++20 runtime on OpenGL 4.5 core. This repository
owns the native engine, SDK, player and game-export tooling. PixieEditor is an
independent C# application that consumes an installed Engine SDK.

**Making a game? Start with the [consumer handbook](docs/handbook/README.md)**
for [SDK setup](docs/handbook/setup.md#obtain-the-inputs), public API usage and
examples. The [Editor walkthrough](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/handbook.md#1-choose-the-sdk-and-create-a-project)
takes you through save, code build, Play/Stop and export.

| Component | Purpose | Guide |
|---|---|---|
| Engine SDK | Headers, libraries, build support and authoring/shipping profiles for external consumers | [Installed SDK](docs/development.md#installed-sdk-and-local-rehearsal) |
| Game runtime | The player and runtime dependencies used by exported games | [Game export](docs/game-export.md) |
| PixieEditor | Project and scene authoring from its own repository | [Editor migration and entry point](editor/README.md) |

Core, Platform, Render, App, Runtime and C API retain their ownership boundaries
inside the shared engine. [PixieLib](https://github.com/Sakamoto0110/PixieLib)
supplies primitives and math; the Editor uses
[PixieInspector](https://github.com/Sakamoto0110/PixieInspector) for inspection UI.

## Build a sample

From the repository root in the Visual Studio 2026 developer prompt with the
C++ workload installed:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-debug
bin/Debug/boids
```

The configure step writes the native solution at `build/PixieEngine.slnx`.
The [development guide](docs/development.md) covers workspace integration,
SDK consumption and release preparation. Existing qualification and its limits
are indexed under [evidence](docs/evidence/README.md).

## Documentation

- [Guides, native API references and samples](docs/README.md).
- [Engine architecture and ownership](docs/architecture.md).
- [Roadmap](ROADMAP.md): objectives, rationale, acceptance criteria and freezes.
- [TODO](TODO.md) and [task breakdowns](TASKLIST.md): active work and dependencies.
- [Dependencies](DEPENDENCIES.md): versions, ownership and candidates.
- [History](docs/history/README.md): completed outcomes and links to evidence.

## License

PixieEngine is released under the [MIT License](LICENSE). Vendored dependencies keep their own
licenses, listed in [`DEPENDENCIES.md`](DEPENDENCIES.md).
