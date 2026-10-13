# shapes

Ten thousand triangles, circles, rings, and squares drifting and spinning inside a square world,
seen through a 2D camera. It exercises the engine's input, its `Camera2D`, and its instanced
`ShapeRenderer`: every shape of one kind is drawn by a single draw call, interpolated between
fixed updates with `FrameTime::alpha`.

- Drag with the left or right mouse button to pan; the world moves with the cursor.
- Turn the mouse wheel to zoom around the cursor; the point under it stays there.
- A white ring marks the world point under the cursor and keeps its size on screen at every zoom.
- Space pauses and resumes the motion; Home brings the camera back to the whole world.

The sample consumes only the engine and the samples' shared option parser.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/shapes
```

## Options

| Option | Default | Meaning |
|---|---|---|
| `--count <n>` | `10000` | Number of shapes, 0 to 1,000,000 |
| `--seed <n>` | `1` | Random seed for positions, motion, kinds, and colors |
| `--ups <hz>` | `60` | Updates per second |
| `--fps <hz>` | `0` | Frame cap; `0` means none |
| `--vsync on\|off` | `on` | Vertical sync |
| `--quit-after <seconds>` | `0` | Closes the window after this long; `0` means never |
| `--screenshot <file.png>` | none | With `--quit-after`, saves the last frame as a PNG |

An invalid option prints the usage and exits with code 2. The engine logs the measured rates once
per second to stderr and shows them in the window title.
