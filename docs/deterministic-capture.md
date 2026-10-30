# Deterministic Capture

**Document ID:** GLOBAL-DETERMINISTIC-CAPTURE

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** development fixed-step capture and snapshot candidate workflow

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Status and scope

Proposal 014 is delivered in increments under
[PIXIE-TODO-051](../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel).
Local Visual Studio and Ninja Debug/Release matrices passed, including fixed-step
capture and export exclusions. Snapshot registration/comparison fixtures and
editor repetition passed separately. No reference image, initial sample selection or
tolerance policy has been approved. The
[qualification record](shared-engine-export-validation.md) distinguishes
executed checks from remaining work.

## Capture inputs

Development player/sample builds accept `--quit-after-steps <n>`, `--seed <n>`
and `--size <width>x<height>`. For example:

```powershell
bin/Release/boids.exe --project samples/boids --quit-after-steps 12 --seed 42 --size 320x180 --vsync off --screenshot build/boids-capture.png
```

The step budget includes zero for the initial state and takes precedence over
a simultaneous wall-time quit request. The capture draws the completed state
with interpolation alpha one. Size describes framebuffer pixels. Existing
wall-time `--quit-after` and `--screenshot` behavior remains available separately.
The new common development options are excluded from shipping builds; existing
sample-specific options keep their contracts.

Seed is an initial World seed. An authored scene seed retains precedence;
changing the command-line seed does not override that authored value. A test
of seed sensitivity must use a scene without an authored seed. Same inputs
promise repeatability on the same renderer, not pixel identity across drivers.

The editor adds development-only step-count termination through its existing
paused-session Step/Render path. It suppresses volatile timing labels for that
capture. Its screenshot is the editor window; the native player's framebuffer
size option does not imply an identically sized editor viewport capture.

## Candidates and comparisons

Python tooling requires the development dependency pinned in
`scripts/requirements-snapshots.txt`. `scripts/snapshot_capture.py` captures and
compares a scenario described by steps, seed, size, renderer, reference and
tolerance. Tolerance limits both maximum channel difference and the fraction
of differing pixels. Qualification fixtures may use strict synthetic comparisons
without establishing an owner-selected tolerance policy.

Approved per-sample scenario manifests live under `samples/<sample>/snapshots/`;
CTest registers only scenarios explicitly marked `approved: true`. Comparison
output goes under build storage and includes actual/diff images on mismatch.
Tests do not overwrite references. A renderer mismatch is reported separately
from the pixel comparison.

`scripts/update_snapshots.py <sample> [scenario]` generates review candidates
under `build/snapshot-candidates/`. An optional `--manifest <file>` accepts an
external candidate manifest without creating repository references. Its
explicit `--approve` mode replaces
references and records the renderer only after the owner has approved that
change and its reason. The presence of that switch grants no approval to use
it. No approved manifests or golden images are implied by the initial tooling.

Capture/comparison tools remain outside game exports and published editor
payloads. Local synthetic tests do not establish Mesa, remote CI, MCP capture
or the full visual regression matrix.
