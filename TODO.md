# PixieEngine Promoted Work

**Document ID:** GLOBAL-TODO

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** formally promoted work, gates, dependencies, and completion criteria

**Surface:** work-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

**Last promotion decision:** 2026-10-14

## Purpose and admission rule

This file is the only scheduler. It holds work that the owner has formally
promoted, with enough scope to execute and close it. It does not hold ideas
([`docs/proposals/`](docs/proposals/README.md)), direction
([`ROADMAP.md`](ROADMAP.md)), or completion narratives
([`docs/archive/`](docs/archive/README.md)).

An entry whose scope is large, or that decomposes into many small tasks, keeps
one entry here and its breakdown in [`TASKLIST.md`](TASKLIST.md). The rules are
in the [documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd).

Unchecked or `promoted` entries are not automatically authorized to start; an
entry may keep an explicit gate after promotion.

## Current promoted work

### `PIXIE-TODO-026` — Continuous integration

**Status:** promoted

**Source:** owner decision on 2026-10-15 ("pode fazer", to Claude's design); earlier work is completed, see below

**Accepted scope:** a GitHub Actions workflow (`.github/workflows/ci.yml`) on every push to `main`, every pull request, and by hand: the documentation verifier on Linux; the engine configured with a `ci` preset (Visual Studio 2022, which the runners have), built in Debug and Release, and tested with `ctest` in both, with Mesa's llvmpipe (a pinned release of mesa-dist-win, checked by SHA-256) placed next to the tests for the OpenGL 4.5 context they open; and the editor built in Release. Warnings stay errors: what a different compiler reports is fixed in the code. If llvmpipe cannot serve the tests that open a window, those tests get a label and the CI skips them, saying so. A status badge in `README.md`. Out: a Linux build of the engine, artifacts, and releases.

**Dependencies and gates:** 1. pushing the workflow to `main`, which the owner's decision covers, since a workflow runs only there

**Completion criteria:** (1) the workflow's run on `main` is green, with every test run or, for those skipped, the reason stated in the log; (2) `docs/development.md` describes the CI, the `ci` preset, and the rule that a red `main` comes first; (3) the documentation policy no longer says no CI exists.

**Breakdown:** [`TASKLIST.md`](TASKLIST.md#pixie-todo-026--continuous-integration)

## Completed

`PIXIE-TODO-001` (documentation foundation) completed on 2026-10-11. Its
entry, the policy review, and its verification are preserved in the
[archive record](docs/archive/todo-001-documentation-foundation-2026-10-11.md).

`PIXIE-TODO-002` (main loop and the hello-gl sample) completed on 2026-10-11.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-002-main-loop-hello-gl-2026-10-11.md).

`PIXIE-TODO-003` (PixieLib, GLM, and EnTT vendored) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-003-vendor-pixielib-glm-entt-2026-10-13.md).

`PIXIE-TODO-004` (documentation verifier) completed on 2026-10-13. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-004-documentation-verifier-2026-10-13.md).

`PIXIE-TODO-005` (input, 2D camera, and instanced shapes) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-005-input-camera-shapes-2026-10-13.md).

`PIXIE-TODO-006` (runtime module: World, entities, Behaviours, and phases)
completed on 2026-10-13. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-006-runtime-module-2026-10-13.md).

`PIXIE-TODO-007` (anchor 1: boids steered by the user) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-007-anchor-1-boids-2026-10-13.md).

`PIXIE-TODO-008` (spatial grid and anchor 2: flocks) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-008-spatial-grid-flocks-2026-10-13.md).

`PIXIE-TODO-009` (particles and anchor 3: attractors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-009-particles-attractors-2026-10-13.md).

`PIXIE-TODO-010` (engine side for editor hosts: host loop, render target,
reflection, and `capi`) completed on 2026-10-13. Its entry, breakdown, and
verification are preserved in the
[archive record](docs/archive/todo-010-host-loop-reflection-capi-2026-10-13.md).

`PIXIE-TODO-011` (C# ImGui editor over the anchors) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-011-imgui-editor-2026-10-13.md).

`PIXIE-TODO-012` (single source of field metadata) completed on 2026-10-13. Its
entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-012-single-source-metadata-2026-10-13.md).

`PIXIE-TODO-013` (scenario values, scene rebuild, and time control) completed on
2026-10-13. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-013-scenario-time-control-2026-10-13.md).

`PIXIE-TODO-014` (attractor size and death on contact) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-014-attractor-size-2026-10-13.md).

`PIXIE-TODO-015` (particle systems and spawners apart) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-015-particle-spawners-2026-10-13.md).

`PIXIE-TODO-017` (spawners placed by hand in the particles game) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-017-spawners-by-hand-2026-10-13.md).

`PIXIE-TODO-016` (selection and editing in the viewport) completed on 2026-10-13.
Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-016-viewport-selection-2026-10-13.md).

`PIXIE-TODO-018` (save and load scenes) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-018-scenes-2026-10-14.md).

`PIXIE-TODO-019` (projects, edit mode, and play mode in the engine) completed
on 2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-019-projects-2026-10-14.md).

`PIXIE-TODO-020` (new and open projects in the editor) completed on
2026-10-14. Its entry, breakdown, and verification are preserved in the
[archive record](docs/archive/todo-020-editor-projects-2026-10-14.md).

`PIXIE-TODO-023` (editor polish, the editor's icon, and a viewport menu) was
promoted and completed on 2026-10-14. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-023-editor-polish-2026-10-14.md).

`PIXIE-TODO-021` (code in a project) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-021-project-code-2026-10-14.md).

`PIXIE-TODO-022` (samples as projects) completed on 2026-10-14. Its entry,
breakdown, and verification are preserved in the
[archive record](docs/archive/todo-022-samples-projects-2026-10-14.md).

`PIXIE-TODO-024` (entity snapshots, scene-ID lookup, and names in the engine)
completed on 2026-10-14. Its entry, breakdown, and verification are preserved
in the [archive record](docs/archive/todo-024-snapshots-2026-10-14.md).

`PIXIE-TODO-025` (moving, undo and redo, duplicating, deleting, and renaming in
the editor) completed on 2026-10-15. Its entry, breakdown, and verification
are preserved in the
[archive record](docs/archive/todo-025-authoring-2026-10-15.md).
