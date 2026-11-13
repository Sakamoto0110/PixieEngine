# Project assets and spritesheets

**Document ID:** PIXIE-PROPOSAL-025

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** Shared project asset workflow for the Editor and Engine runtime

**Surface:** proposal

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## PIXIE-PROPOSAL-025

**State:** promoted on 2026-11-12 to workspace `PXW-ROADMAP-002` and its three
TODO entries. The atlas contract, browsing, import, and frame assignment were
verified locally. Builder implementation is deferred.

The first delivery makes it possible to take existing game artwork from
import to a playable exported scene. It picks up the deferred asset and
resource work in [Proposal 017](editor-authoring-consolidation.md#8-documentation-and-later-managers)
and uses the integrated Spritesheet Builder as its first consumer. Pixie
already draws a [PNG or a pixel region](../modules/runtime/REFERENCE.md#sprites),
and the Editor already imports PNG and TTF files into project-relative paths.

## Responsibilities

| Area | Owner | First delivery |
| --- | --- | --- |
| Asset Manager | PixieEditor | Import and organize project assets, browse existing files, search, preview, assign assets to components, and report missing references. Reuse the existing project asset import path for PNG and TTF. |
| Resource Manager | PixieEngine | Resolve project-relative assets and atlas frame references. Extend the existing image and font caches with explicit ownership, sharing, reload, release, and diagnostics. Keep GPU texture lifetime tied to the graphics context and expose the runtime behavior to SDK consumers. |
| Spritesheet Builder | PixieEditor | Select imported PNGs to pack into an atlas or slice an existing sheet by grid or manual selection. Preview regions, edit names and pivots, and drag a frame from the Asset Manager into a scene. |

## Asset references and output

- Existing scenes retain support for direct PNG paths, pixel regions and
  pivots. New atlas-backed sprites reference a project-relative atlas
  descriptor and a stable frame ID. Display names and packed coordinates can
  change while the frame ID survives. Removing a referenced frame produces
  a diagnostic and preserves the reference for repair.
- The builder produces a PNG and a versioned descriptor containing the image
  path, frame IDs, names, pixel rectangles and normalized pivots. Save a
  rebuild so a failed write leaves the last usable atlas intact. Provide
  padding and edge extrusion for packed frames and check sampling under the
  existing sprite filters. Keep the packing logic callable separately from
  the Editor panel so preview and rebuild share one implementation.
- Atlas resolution uses the existing rendering path after the runtime finds
  the image, region and pivot. The normal export includes the referenced
  descriptor and PNG. The first delivery covers PNG, TTF, and atlas frame
  authoring. Animation and additional media formats remain outside this scope.
- Use the owner's licensed artwork only in final game products or disposable
  test projects. The SDK ships the contract and code. Repository tests use
  generated fixtures. This restriction also applies to derived atlas images.

## Promotion

The Resource Manager (`PXW-TODO-002`) owns the reference contract. The Asset
Manager (`PXW-TODO-003`) owns the project-facing workflow. The Spritesheet
Builder (`PXW-TODO-004`) integrates both and delivers the end-to-end acceptance
fixture. The workspace roadmap owns the shared acceptance criteria.
