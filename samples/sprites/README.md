# sprites

The sprites of [PIXIE-PROPOSAL-007](../../docs/proposals/sprites.md), in a project with no code: a
scene whose entities are images, drawn by the engine's `Sprite` component
([runtime reference](../../docs/modules/runtime/REFERENCE.md#sprites)). The image is the neko mark
from the shared resources, `images/neko-purple-light.png`, brought in with `vdr cp`.

| Entity | What it shows |
|---|---|
| `backdrop` | The image at its pixels times 0.68, nearly transparent (alpha 36), on layer -1, behind the rest |
| `neko` | The image as it is, at a scale of 0.22 |
| `tinted and turned` | A light blue tint multiplied with the image, turned 0.35 radians around its center |
| `pixel filter` | The `Pixel` filter: nearest texels, crisp, where `Smooth` blends them |
| `head, a region` | Only the region `[400, 20, 360, 380]` of the image, the head, at a scale of 0.5 |
| `layer 1, on top` and `layer 0` | Two overlapping sprites: the older one is drawn on top, because its layer is higher |

## The project

This folder is a project ([editor's README](../../editor/README.md#projects)) without code:
`sprites.pixie`, `scenes/main.scene`, and `images/`. Since it has no `CMakeLists.txt`, the editor
opens it with the engine's generic library, and the engine's generic player, `pixie-player`, plays
it. The test `sprites-project` opens it through the generic library and reads every image its
sprites name.

## Build and run

From the repository root, in the Visual Studio developer prompt:

```bash
cmake --preset vs2026
cmake --build --preset vs2026-release
bin/Release/pixie-player --project samples/sprites
```

To edit the scene, open the folder in the editor, and drop more PNG files on its viewport:

```bash
editor/PixieEditor/bin/Release/net10.0/PixieEditor --project samples/sprites
```

The middle button and the wheel pan and zoom, in the player as in the editor.
