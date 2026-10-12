# The Engine Depends on No GUI Binding

**Document ID:** ARCHIVE-EDITOR-BINDING-20261012

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** removal of the editor UI dependencies from the engine's dependency list

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-010

**Type:** dependency

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-12

**Commits:** with this record (parent `46ee6aa4d893fc14c6ca26d8b13fce051474e179`)

**Current state:** [`DEPENDENCIES.md`](../../DEPENDENCIES.md#editor-side)

`DEPENDENCIES.md` had listed Dear ImGui, ImGui.NET, Hexa.NET.ImGui, and Silk.NET
as engine dependencies, with a recommendation to move to Hexa.NET.ImGui. By
2026-10-12, PixieInspector had already moved its Dear ImGui view to
Hexa.NET.ImGui 2.2.9 and its host to Hexa.NET.GLFW.

In answering PixieInspector's questions about which view to integrate, the
owner said he does not want to be tied to Hexa.NET.ImGui. The engine's contract
with an editor host is an active OpenGL 4.5 core context and a texture to show;
it depends on no toolkit or binding. The rule that the host and the inspector's
view use the same binding and version is PixieInspector's. The editor UI rows
and the "ImGui version" section left `DEPENDENCIES.md`, which now points to
PixieInspector's own list.
