# PixieEngine

A 2D-first game engine: a C++ runtime on OpenGL 4.5 core, a standalone player
executable, and a C# editor that inspects engine state through
[PixieInspector](https://github.com/Sakamoto0110/PixieInspector). Primitives come
from [PixieLib](https://github.com/Sakamoto0110/PixieLib).

Early stage: a provisional engine library (window, OpenGL context, main loop,
input, 2D camera, and instanced shapes) and the samples that consume it, under
[`samples/`](samples/), each with its own README. From the Visual Studio 2026
developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-debug
bin/Debug/shapes
```

The configure step also writes `PixieEngine.slnx` at the root. Building,
checking, and committing are in the [development guide](docs/development.md).

- [`ROADMAP.md`](ROADMAP.md): what the engine is and is not, and where it is going.
- [`TODO.md`](TODO.md): promoted work. [`TASKLIST.md`](TASKLIST.md): its task breakdowns.
- [`DEPENDENCIES.md`](DEPENDENCIES.md): every dependency, its status, and candidates.
- [`docs/`](docs/README.md): documentation index, policy, development guide, proposals, and archive.
