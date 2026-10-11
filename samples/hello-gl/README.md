# hello-gl

Balls bouncing off the window edges, "hello GL" in the middle, and the measured frame and update
rates in the top-left corner. It exercises the engine's window, OpenGL 4.5 core context, and main
loop: `Update` at a fixed rate, `Render` interpolated between updates with `FrameTime::alpha`.

The sample consumes only the engine. Its `Vec2`, `Color`, and `Ball` structs are its own; the
renderers draw with raw OpenGL, and the text uses `stb_easy_font`.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-debug
bin/Debug/hello-gl
```

Or open the root `PixieEngine.slnx` and press F5; `hello-gl` is the startup project.

## Options

| Option | Default | Meaning |
|---|---|---|
| `--ups <hz>` | `60` | Updates per second |
| `--fps <hz>` | `0` | Frame cap; `0` means none |
| `--vsync on\|off` | `on` | Vertical sync |
| `--quit-after <seconds>` | `0` | Closes the window after this long; `0` means never |

An invalid option prints the usage and exits with code 2. The engine also logs the measured rates
once per second to stderr and shows them in the window title.
