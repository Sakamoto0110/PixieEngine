# PixieEngine

A 2D-first game engine: a C++ runtime on OpenGL 4.5 core, a standalone player
executable, and a C# editor built on InteractiveEditor.

Early stage: a provisional engine library (window, OpenGL context, main loop) and
the `hello-gl` sample that consumes it.

```bash
cmake --preset vs2026
cmake --build --preset vs2026-debug
bin/Debug/hello-gl --help
```

CMake and the Visual Studio 2026 generator ship with Visual Studio; run these from its developer
prompt, where `cmake` is on `PATH`.

- **Solution:** the configure step writes `PixieEngine.slnx` at the repository root. It shows the
  engine, the samples (`hello-gl` starts on F5), and the dependencies built from source. It is
  generated, not versioned: to add a project, add it to CMake and configure again.
- **Executables:** `bin/Debug/` and `bin/Release/`.
- **Engine only:** `cmake --preset vs2026 -DPIXIE_BUILD_SAMPLES=OFF` leaves the samples out.
- **Generated files:** everything else lives in `build/`, which can be deleted at any time.

- [`ROADMAP.md`](ROADMAP.md): what the engine is and is not, and where it is going.
- [`TODO.md`](TODO.md): promoted work. [`TASKLIST.md`](TASKLIST.md): its task breakdowns.
- [`DEPENDENCIES.md`](DEPENDENCIES.md): every dependency, its status, and candidates.
- [`docs/`](docs/README.md): documentation index, policy, proposals, and archive.
