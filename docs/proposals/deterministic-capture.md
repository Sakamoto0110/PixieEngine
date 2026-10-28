# Deterministic Capture and Snapshot Tests

**Document ID:** GLOBAL-PROPOSAL-DETERMINISTIC-CAPTURE

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** making a capture run produce the same frame every time, and using it for visual regression tests with approved reference images

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-014

**State:** promoted

**Idea:** A capture run starts a game, runs it, saves a frame and closes, in well under a second.
The player and every sample take `--quit-after <seconds>` with `--screenshot <file.png>`, and the
editor takes the same pair. It is the cheapest way an agent sees the engine's output. It is not yet
repeatable: the run ends after a span of wall time, so the frame saved depends on how many fixed
steps fit in it, and the player has no seed (the editor has `--seed`, and `pixie_session_create`
takes one). A static scene (`shapes`, `sprites`) gives the same image every time; an animated one
(`boids`, `particles`, `sparks`) never does.

- **Repeatable runs.** The player and the samples gain `--quit-after-steps <n>` (close after `n` fixed
  steps, whatever the wall time), `--seed <n>` (as the editor's), and `--size <w>x<h>` (the window's
  framebuffer size, so the image size does not depend on the desktop). The captured frame is rendered
  after the last step with an interpolation alpha of 1, so render timing cannot move it. The editor
  gains `--quit-after-steps` too. Same command line, same image, on the same renderer.
- **Snapshot tests.** A sample may keep approved reference images beside it:

  ```text
  samples/boids/
    snapshots/
      scenarios.json   # per scenario: steps, seed, size, the renderer it was approved on, tolerance
      default.png      # one approved image per scenario
  ```

  CTest gets one test per scenario (`snapshot-boids-default`) that runs the capture and compares it
  with the reference. A failure writes `actual.png` and `diff.png` under
  `build/snapshots/<sample>/<scenario>/`, never into the repository, and prints their paths for
  whoever reads the log, agent or person.
- **Approving an image.** A script, `scripts/update_snapshots.py <sample> [scenario]`, runs the
  capture and replaces the reference. A reference changes only in a commit the owner approves, which
  says why the image changed.
- **Renderers.** CI renders with Mesa's llvmpipe; the owner's machine renders on its GPU, and the two
  may differ in the last bits of a pixel. References are approved on llvmpipe, which CI uses on every
  push, and the comparison takes a per-scenario tolerance (largest channel difference and share of
  differing pixels) so a local GPU run does not fail on rounding.

**Rationale:** An agent that can produce the same frame from the same command can tell a change in
the engine from noise, and a reference image turns "it looks right" into a test that keeps passing.
The capture already exists; what it lacks is a step count instead of a time span, a seed and a fixed
size. The development channel's `capture` command ([PIXIE-PROPOSAL-013](dev-channel.md)) relies on
the same rendering rule.

**Constraints:**

- The existing `--quit-after` and `--screenshot` keep their meaning; the new options sit beside them.
- Reference images are binary files in the repository; they are kept small (a scenario's size is the
  smallest that shows what it checks) and few (the anchors and the laboratories first).
- Snapshot tests open a window, like the C API tests, so they need an OpenGL 4.5 driver: llvmpipe in
  CI, the GPU locally.
- The sample folders may be reorganized later (demos, minimal samples, laboratories); the
  `snapshots/` folder moves with its sample.

**Open questions:**

1. Which samples get references first? The recommendation is the three anchors and `sparks`.
2. Should the tolerance be one value for the repository or set per scenario?

**Promotion target:** [PIXIE-TODO-051](../../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).

**Source:** owner discussion in conversation, 2026-10-28: the fast open-capture-close run as the
agents' main way to look at a program, made repeatable and given its own place in the filesystem.

**Outcome:** supplied and explicitly included by the owner on 2026-10-28 in PIXIE-TODO-051, with
proposals 010 and 013. Repeatable capture and snapshot qualification are accepted work, not
completed behavior. The recommendations in open questions 1 and 2 are not owner-selected defaults.
Reference images still require explicit owner approval; no new approved images are implied by
promotion. Development capture/snapshot tools follow the task's export/publish/remote-release
exclusion rules, while the existing command-line options keep their semantics.
