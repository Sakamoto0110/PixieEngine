# PIXIE-TODO-028 Completed: Sprites in the Editor, and a Sprites Sample

**Document ID:** ARCHIVE-TODO-028-SPRITES-EDITOR-20261015

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** completion of PIXIE-TODO-028, and with it PIXIE-PROPOSAL-007: placing sprites in the editor by dropping or choosing PNG files, image fields in the inspector, and the sprites sample with a generic player

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-049

**Type:** completion

**Origin:** PIXIE-TODO-028 (PIXIE-TASK-126 to PIXIE-TASK-129)

**Author:** claude

**Reference date:** 2026-10-15

**Commits:** `0a55f75ccd554a92f17df6dd00e229fec15256f6`

**Original path:** `TODO.md` (the entry) and `TASKLIST.md` (the breakdown)

**Current state:** the [editor's README](../../editor/README.md) and the [sample's README](../../samples/sprites/README.md)

## Entry as completed

**Status:** completed on 2026-10-15

**Source:** owner decision on 2026-10-15; [PIXIE-PROPOSAL-007](../proposals/sprites.md), part 2

**Accepted scope:** dropping PNG files onto the viewport makes one sprite per file where they fall, named after the file, selected, as one step; a file outside the project is copied into the project's `images/` first, said in the status line. "New sprite..." in the viewport's menu picks a PNG and places it at the click. An asset field in the inspector shows its path, a `...` button picking a file of its kind from the project folder, and a preview of the image; editing it is a step. `samples/sprites`, a project with only a scene, shows the neko mark from the shared resources tinted, layered, rotated, with both filters, and with a region. Out: rotating and scaling by hand, animation.

**Dependencies and gates:** 1. PIXIE-TODO-027

**Completion criteria:** (1) in the editor, dropping, the menu, the asset field, and its undo work, verified through options for scripted runs, since the owner asked that computer use not be used, with what could not be run stated; (2) the sample opens in the editor and runs in the player; (3) the editor's README and the samples' documents describe them; (4) zero warnings.

**Commits:** `0a55f75ccd554a92f17df6dd00e229fec15256f6` (dropping, the image chooser, "New sprite here...", text and image fields in the inspector, `--do drop=` and `image=`, the sample, the generic player, and the `sprites-project` test); the editor's README, the sample's README, the root README, and `docs/development.md` are updated with this record.

## Breakdown as completed

- `PIXIE-TASK-126` — **Status:** done — Asset fields in the inspector: path, `...` picker from the project folder, preview, a step per edit.
- `PIXIE-TASK-127` — **Status:** done — Dropping PNG files onto the viewport, copying outside files into `images/`, and "New sprite..." in the viewport's menu, each one step.
- `PIXIE-TASK-128` — **Status:** done — `samples/sprites`, with the neko mark from `vdr`.
- `PIXIE-TASK-129` — **Status:** done — Scripted verification, the editor's README, and the samples' documents.

## Choices made while building it

- A text edit is a `ReplaceStep` (snapshots before and after) rather than a step of its own, as
  renames are; nothing new was needed in the history.
- An image dropped or chosen in a project is always used by a path inside it: one outside is copied
  into `images/`; an identical file already there is reused, and a different one with the same name
  gets a number (`neko 2.png`).
- A dropped image larger than half the view is given a smaller scale, so it does not cover the
  view; one image pixel stays one world unit, times that scale. This refines the proposal, which
  said only "where they fall".
- Files dropped anywhere on the window are taken, at the mouse when it is over the viewport and at
  the middle of the view otherwise; files that are not PNG are left out, and the status line names
  them.
- The inspector draws text fields itself, above the component's PixieInspector section, so a path is
  written once, when the box lets go of the keyboard, and the session never reads half-typed paths.
- A project without code had no player: the engine now builds `pixie-player`, `PlayerMain` linked
  with the generic game, which plays the folder `--project` names. The sample is checked by
  `capi-test --open <folder> --scene-only` (no scenario; every sprite's image readable), registered
  from `samples/CMakeLists.txt`, since a `CMakeLists.txt` in the sample would make the editor treat
  it as code.

## Verification

Machine: Windows 11, AMD Radeon RX 7600, .NET SDK 10.0.401, at `0a55f75`; the engine in Debug and
Release with zero warnings and `ctest` 10 of 10 in both (with `sprites-project`); the editor in Debug
and Release with zero warnings. At the owner's request, no computer use: the editor was driven with
`--do` on a copy of the sample.

- `drop=` of `neko-blue.png` and `neko-gray.png` from the shared resources and of `sprites.pixie`, at
  (500, 200): the `.pixie` was left out with its reason; two sprites named after the files, 16 units
  apart, selected, one step ("creation of 2 sprites"); both images copied into `images/`, as the
  status line said. Undone to 7 entities, redone to 9.
- `image=` of `neko-gray.png` on `neko-blue`: the existing copy reused, one step ("edit of
  Sprite.image"), undone back to `images/neko-blue.png`; `image=` of a file inside the project: its
  relative path, nothing copied.
- A second drop of `neko-blue.png`: named `neko-blue 2`, the copy in `images/` reused. `save`: the
  scene file holds the sprites, their paths, and, for an 838 x 855 image dropped into an 893-unit
  view, a scale of 0.504.
- The screenshot showed the inspector's image field with its `...` button and a preview of
  838 x 855 px, and the selected sprite outlined by its rectangle.
- `pixie-player --project samples/sprites` drew the sample as its README describes: the tint, the
  turn, both filters, the region, and the layers.
- Not run: dropping from the system's file manager and the image chooser's clicks, which need real
  input; they call the code the commands above ran (`ViewportPanel.Drop`, `Authoring.ImportImage`
  and `SetText`). Reloading an image changed on disk was checked by the runtime test, not by hand in
  the editor. Linux.
- The continuous integration runs on the push that carries these commits; its run is checked then.
