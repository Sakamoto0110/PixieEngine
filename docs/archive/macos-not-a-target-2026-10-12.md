# macOS Is Not a Target

**Document ID:** ARCHIVE-MACOS-20261012

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** macOS dropped as a target platform for the engine, the player, and the editor

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-009

**Type:** decision

**Origin:** owner decision

**Author:** claude

**Reference date:** 2026-10-12

**Commits:** with this record (parent `46ee6aa4d893fc14c6ca26d8b13fce051474e179`)

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#active-guardrails)

The roadmap said only that macOS was not a target of the OpenGL renderer, as
a consequence of OpenGL 4.5. While discussing the editor host, whose sample in
PixieInspector creates an OpenGL 3.3 context with macOS in mind, the owner said
macOS does not matter. It is now a guardrail: macOS is not a target for the
engine, the player, or the editor, and no choice is weighed against it.

The editor host can therefore create an OpenGL 4.5 core context, which the
engine needs when it renders into the editor's context.
