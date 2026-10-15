# Sprites, Next

**Document ID:** ARCHIVE-SPRITES-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** the owner's choice of sprites as the phase after the continuous integration, and the design Claude proposed for it

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-046

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** with this record (parent `dd306d01ef73395abd5f7c09612d4bdc11aee851`)

**Current state:** [PIXIE-PROPOSAL-007](../proposals/sprites.md) and [`TODO.md`](../../TODO.md)

## How the decision arrived

With the continuous integration green, the owner said to go on to sprites ("bora pros sprites"),
the next direction recorded in `ROADMAP.md`. Claude presented a decided design, which the owner
accepted ("pode fazer").

## Decisions taken by Claude, accepted with the design

- PNG only, through `stb_image` at the `nothings/stb` commit already pinned; premultiplied alpha.
- One image pixel is one world unit, times the transform's scale; the `Sprite` has no size field.
- `Sprite` fields: `image`, `region`, `pivot`, `color`, `layer`, `filter` (`Smooth` with mipmaps,
  `Pixel`).
- Drawing order: sprites, particles, shapes, the overlay; a shared layer with shapes later.
- The session's image cache resolves paths against the project folder and reloads changed files.
- A missing or broken image is a magenta checkerboard with one logged reason, never an error that
  stops a scene from opening.
- Text fields enter the reflection, with an asset kind, and the C API goes to version 7.
- In the editor: dropping files onto the viewport, copying outside files into `images/`, "New
  sprite..." in the viewport's menu, and asset fields with a picker and a preview.
- A sample, `samples/sprites`, with the neko mark from the shared resources.
- Next after this phase: exporting a game.
