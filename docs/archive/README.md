# Archive Router

**Document ID:** GLOBAL-ARCHIVE-INDEX

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** router to every archived event: what changed, what was completed, and why

**Surface:** index

**Boundary:** global

**Authority role:** non-normative

**Mutation:** append-only

**Indexing:** include

One row per archive record, in `Archive ID` order. A row says enough to decide
whether the record is the one you need; the record holds the full context. The
rules are in the [documentation policy](../governance/documentation-policy.md#archive).

| ID | Date | Type | Origin | Author | Summary | Commits | Record |
|---|---|---|---|---|---|---|---|
| PIXIE-ARCHIVE-001 | 2026-10-09 | promotion | PIXIE-TODO-001 | claude | Documentation foundation promoted and delivered, derived from NekoLib's model | `f081b81` | [`documentation-foundation-2026-10-09.md`](documentation-foundation-2026-10-09.md) |
| PIXIE-ARCHIVE-002 | 2026-10-09 | decision | owner decision | claude | Scope discussion: 2D first, OpenGL 4.5 core, PixieLib's role, editor portability; rationale for open recommendations | `f081b81` | [`inception-2026-10-09.md`](inception-2026-10-09.md) |
| PIXIE-ARCHIVE-003 | 2026-10-09 | decision | owner decision | claude | GLEW as the starting GL loader; glad 2 kept as a candidate | `f081b81` | [`gl-loader-glew-2026-10-09.md`](gl-loader-glew-2026-10-09.md) |
| PIXIE-ARCHIVE-004 | 2026-10-10 | roadmap | PIXIE-PROPOSAL-001 | claude | Guardrail: the engine never depends on the editor | `ccf1db9` | [`guardrail-engine-independent-of-editor-2026-10-10.md`](guardrail-engine-independent-of-editor-2026-10-10.md) |
| PIXIE-ARCHIVE-005 | 2026-10-10 | decision | owner decision | claude | C++20 without modules, CMake, and `FrameTime` instead of a global timestep | `ccf1db9` | [`cpp20-cmake-frametime-2026-10-10.md`](cpp20-cmake-frametime-2026-10-10.md) |
| PIXIE-ARCHIVE-006 | 2026-10-11 | completion | PIXIE-TODO-002 | claude | Main loop, hello-gl sample, vendored dependencies, `bin/`, root solution; with measured rates | `1b67d02`, `f99f121`, `ccf1db9` | [`todo-002-main-loop-hello-gl-2026-10-11.md`](todo-002-main-loop-hello-gl-2026-10-11.md) |
| PIXIE-ARCHIVE-007 | 2026-10-11 | decision | owner decision | claude | Static MSVC runtime: executables no longer need the Visual C++ Redistributable | `f99f121`, `ccf1db9` | [`static-msvc-runtime-2026-10-11.md`](static-msvc-runtime-2026-10-11.md) |
| PIXIE-ARCHIVE-008 | 2026-10-11 | completion | PIXIE-TODO-001 | claude | Documentation policy reviewed: archive as history with origin and commits, module and sample docs, commit conventions | `f081b81`, and the commit that adds it | [`todo-001-documentation-foundation-2026-10-11.md`](todo-001-documentation-foundation-2026-10-11.md) |
