# Engine filesystem and authority map

**Document ID:** GLOBAL-FILESYSTEM
**Schema version:** 1
**Kind:** reference
**Lifecycle:** current
**Subject:** Engine ownership, parent workspace and focused documentation routes
**Surface:** index
**Boundary:** global
**Authority role:** non-normative
**Mutation:** authored
**Indexing:** include

This map describes Engine's scope. The [workspace entry](../../.governance/workplaces/void-workplace/README.md)
routes the four projects; its [compact policy](../../.governance/workplaces/void-workplace/docs/governance/documentation-policy.md)
defines applicable shared constraints. Engine's [policy](docs/governance/documentation-policy.md)
owns local specialization. Runtime copies and catalogs do not create new authorship.

## Where to work

| Area | Owner and direct reading route |
|---|---|
| `engine/core`, `engine/platform`, `engine/render`, `engine/app` | Engine; [architecture](docs/architecture.md#host-and-player) and [module map](docs/README.md#native-modules-and-api) route contracts |
| `runtime/` | Engine World/entity/lifecycle/persistence semantics; [Runtime reference](docs/modules/runtime/REFERENCE.md) |
| `capi/` | Engine native ABI and binding producer; [C API reference](docs/modules/capi/REFERENCE.md) |
| `cmake/`, `templates/`, SDK support | Engine generation and installed consumer contracts; [development](docs/development.md) |
| `tools/`, `devchannel/` | Engine-owned native export and development tooling; [export](docs/game-export.md), [capture](docs/deterministic-capture.md) |
| `samples/`, `tests/` | [Feature samples](docs/README.md#features-and-samples), [World tests](tests/runtime/WorldTests.cpp), [input tests](tests/runtime/InputControlTests.cpp), [scene movement tests](tests/runtime/SceneMovementTests.cpp) and [export qualification driver](tests/export/qualify_export.py) |
| `external/` | Versioned dependency snapshots; [dependencies](DEPENDENCIES.md), never a second owner of current Lib contracts |
| `editor/README.md`, `docs/editor-*.md` | Compatibility routes to independently owned [Editor documentation](https://github.com/Sakamoto0110/PixieEditor/blob/d1760e14549da826fecddec3550244123f9ea715/docs/README.md) |
| Root planners | [ROADMAP](ROADMAP.md) owns outcomes/criteria, [TODO](TODO.md) owns approved execution, [TASKLIST](TASKLIST.md) decomposes it |
| `docs/history/`, `docs/archive/`, `docs/evidence/` | Existing canonical records, original IDs and baseline-specific limitations |
| `.local/`, build outputs | Machine-local state, retained evidence or generated artifacts; not a deletion permission |

## Coordination and preserved records

Outcomes 053-056, 060 and 061 now have their canonical coordination owner in the
[workspace roadmap](../../.governance/workplaces/void-workplace/ROADMAP.md).
The local planners route to shared parents and retain Engine-owned task states.
[Pinned pre-transfer excerpts](docs/workspace-reference.md) support standalone
reading but cannot replace current parent or block requirements.
The [058/059 closure](docs/history/consumer-integration-2026-11-06.md)
is the preservation pilot. It records legacy direct-owner work without an original
roadmap block and does not gain new acceptance from being indexed.

The [D09 freeze](ROADMAP.md#d09-glfw-distribution) remains governed by its original
Engine record. Neither workspace membership nor this map authorizes its resumption.

## Standalone reading and checks

A standalone checkout can use the local contract routes without the parent workspace.
Unavailable parent/peer routes are reported as unavailable, not as waived requirements.
`./checkdocs.ps1` permits explicitly declared absent optional scopes and reports them.
`./checkdocs.ps1 -Workspace` requires every declared scope and validates catalog sources.
Both check structure only. No product build or runtime qualification follows from this map.
