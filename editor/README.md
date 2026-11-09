# PixieEditor migration

PixieEditor is maintained in its own checkout, `PixieEditor` within the Pixie workspace.
Open `Pixie.Workspace.slnf` at the workspace root in Visual Studio. The
workspace references `PixieEditor/src/PixieEditor.csproj` directly. For a
standalone checkout, follow the Editor's `README.md` and `docs/development.md`.

## Projects

See the corresponding section of `PixieEditor/README.md` in the Editor checkout.

## Build and run

Use the Editor checkout's `eng/build.ps1` with an absolute installed SDK root and the producer's trusted catalog hash. The Editor workflow consumes an explicit public SDK artifact.

## Options

See the corresponding section of `PixieEditor/README.md` in the Editor checkout.

## Local development MCP channel

Build the Editor with the private SDK and development profile. The Engine's [MCP qualification driver](../tests/mcp/qualify_mcp.py) takes explicit SDK, host and test artifacts. See the Editor README for bridge setup.

## World properties

See the corresponding section of `PixieEditor/README.md` in the Editor checkout.

## Scene authoring

See the corresponding section of `PixieEditor/README.md` in the Editor checkout.

## Collision and movement authoring

See the corresponding section of `PixieEditor/README.md` in the Editor checkout.

## Generic entity input and C++ component authoring

See the corresponding section of `PixieEditor/README.md` in the Editor checkout.

## Canonical presentation and authored cameras

See the corresponding section of `PixieEditor/README.md` in the Editor checkout.

## Source provenance

PIXIE-PROPOSAL-022 imported the nested source from Engine commit
`523cfa75312231e827e19051e546de7bfbc55bb4` plus the local GLFW integration.
The corresponding source on the path-normalized branch is at `60bbaf4e4b391800b40d252fb703bb6d5449c7b7`.
Use `git show <commit>:<original-path>` in PixieEngine for a committed
baseline file. Historical references name the original paths. The Editor's
`docs/extraction-origin.md` records destination ownership.
