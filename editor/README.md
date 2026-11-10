# PixieEditor migration

**Document ID:** GLOBAL-EDITOR-PORTAL
**Schema version:** 1
**Kind:** reference
**Lifecycle:** current
**Subject:** compatibility routes to the independent Editor owner
**Surface:** portal
**Boundary:** global
**Authority role:** portal
**Mutation:** authored
**Indexing:** pointer-only
**Canonical:** README.md
**Canonical scope:** pixieeditor

PixieEditor is maintained in its own repository and consumes the installed
Engine SDK. This Engine portal preserves existing section links; the
[Editor README](../../PixieEditor/README.md) and
[development guide](../../PixieEditor/docs/development.md) own its current workflow.
This documentation pilot does not change Editor workflows or decide future
Editor planning ownership.

## Projects

See [Editor projects](../../PixieEditor/README.md#projects).

## Build and run

See [Editor build and run](../../PixieEditor/README.md#build-and-run).

## Options

See [Editor options](../../PixieEditor/README.md#options).

## Local development MCP channel

See [the Editor development channel](../../PixieEditor/README.md#local-development-mcp-channel).
The Engine's [MCP qualification driver](../tests/mcp/qualify_mcp.py) remains
Engine-owned; the portal authorizes no qualification run.

## World properties

See [Editor World properties](../../PixieEditor/README.md#world-properties).

## Scene authoring

See [Editor Scene authoring](../../PixieEditor/README.md#scene-authoring).

## Collision and movement authoring

See [Editor collision and movement authoring](../../PixieEditor/README.md#collision-and-movement-authoring).

## Generic entity input and C++ component authoring

See [Editor input and C++ authoring](../../PixieEditor/README.md#generic-entity-input-and-c-component-authoring).

## Canonical presentation and authored cameras

See [Editor presentation and cameras](../../PixieEditor/README.md#canonical-presentation-and-authored-cameras).

## Source provenance

PIXIE-PROPOSAL-022 imported the nested source from Engine commit
`523cfa75312231e827e19051e546de7bfbc55bb4` plus the local GLFW integration.
The corresponding source on the path-normalized branch is at `60bbaf4e4b391800b40d252fb703bb6d5449c7b7`.
Use `git show <commit>:<original-path>` in PixieEngine for a committed
baseline file. Historical references name the original paths. The Editor's
[extraction origin](../../PixieEditor/docs/extraction-origin.md) records destination ownership.
