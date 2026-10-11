# GLEW Chosen as the Starting GL Loader

**Document ID:** ARCHIVE-GL-LOADER-20261009

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** choice of GLEW over glad 2 as the OpenGL function loader

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-003

**Type:** decision

**Origin:** owner decision (open question 2)

**Author:** claude

**Reference date:** 2026-10-09

**Commits:** `f081b81ddd4ec3c9301a28aba581bb1a94681dbf`

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#active-guardrails), [`DEPENDENCIES.md`](../../DEPENDENCIES.md)

glad 2 was recommended: generated for exactly OpenGL 4.5 core, so a call
outside it fails to compile, with no library to link and a loader callback
independent of who created the context. The owner chose GLEW, the original
choice, to start with. glad 2 stays a candidate.

GLEW under the core profile needs `glewExperimental = GL_TRUE` and a drained
`glGetError()` after `glewInit()`, whose spurious `GL_INVALID_ENUM` would
otherwise reach the debug output. The engine handles both.
