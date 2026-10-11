# Static MSVC Runtime

**Document ID:** ARCHIVE-STATIC-RUNTIME-20261011

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** linking the C++ runtime statically so executables run without the Visual C++ Redistributable

**Surface:** archive-record

**Boundary:** global

**Authority role:** evidence

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-007

**Type:** decision

**Origin:** owner decision, during PIXIE-TODO-002

**Author:** claude

**Reference date:** 2026-10-11

**Commits:** `f99f121a7a9f770bc1db620a8db6116d87d42cf9` (build change), `ccf1db9e7537f20f3d9eb127c492984ee1eefa5e` (guardrail)

**Current state:** [`ROADMAP.md`](../../ROADMAP.md#active-guardrails)

The Release `hello-gl.exe` depended on `MSVCP140.dll`, `VCRUNTIME140.dll`, and
`VCRUNTIME140_1.dll`, which a machine without the Visual C++ Redistributable
lacks. Every target, GLEW and GLFW included, now links the runtime statically
(`/MT`, `/MTd`). The Release executable depends only on Windows system DLLs and
`OPENGL32.dll`; it grew from 695 KB to 905 KB.

Cost accepted: each executable or shared library carries its own copy of the
runtime, so memory must be freed by the module that allocated it, as a C ABI
requires anyway.
