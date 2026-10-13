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
| PIXIE-ARCHIVE-008 | 2026-10-11 | completion | PIXIE-TODO-001 | claude | Documentation policy reviewed: archive as history with origin and commits, module and sample docs, commit conventions | `f081b81`, `46ee6aa` | [`todo-001-documentation-foundation-2026-10-11.md`](todo-001-documentation-foundation-2026-10-11.md) |
| PIXIE-ARCHIVE-009 | 2026-10-12 | decision | owner decision | claude | macOS is not a target platform for the engine, the player, or the editor | `83aad39` | [`macos-not-a-target-2026-10-12.md`](macos-not-a-target-2026-10-12.md) |
| PIXIE-ARCHIVE-010 | 2026-10-12 | dependency | owner decision | claude | The engine depends on no GUI binding; editor-side dependencies belong to PixieInspector | `83aad39` | [`editor-binding-independence-2026-10-12.md`](editor-binding-independence-2026-10-12.md) |
| PIXIE-ARCHIVE-011 | 2026-10-12 | decision | owner decision | claude | Documentation aligned with Void Agent Workplace's references and the family's evolutions; PixieInspector rename propagated | `83aad39` | [`documentation-aligned-with-vaw-2026-10-12.md`](documentation-aligned-with-vaw-2026-10-12.md) |
| PIXIE-ARCHIVE-012 | 2026-10-13 | dependency | owner decision | claude | PixieInspector 0.1.0 on nuget.org, recorded as a package dependency of the C# editor; the editor's binding comes with its view package | the commit that adds it | [`pixieinspector-nuget-2026-10-13.md`](pixieinspector-nuget-2026-10-13.md) |
| PIXIE-ARCHIVE-013 | 2026-10-13 | decision | owner decision | claude | Object model (EC-style API over ECS storage), three anchors, ImGui editor and viewport, EnTT, PixieLib vendored; serialization postponed | the commit that adds it | [`object-model-and-editor-decisions-2026-10-13.md`](object-model-and-editor-decisions-2026-10-13.md) |
| PIXIE-ARCHIVE-014 | 2026-10-13 | promotion | PIXIE-PROPOSAL-004 | claude | Nine blocks toward the anchors promoted as PIXIE-TODO-003 to PIXIE-TODO-011, with PIXIE-TASK-011 to PIXIE-TASK-057 | the commit that adds it | [`anchors-promotion-2026-10-13.md`](anchors-promotion-2026-10-13.md) |
| PIXIE-ARCHIVE-015 | 2026-10-13 | completion | PIXIE-TODO-003 | claude | PixieLib 0.2.0, GLM 1.0.3, and EnTT 4.0.0 vendored and built offline; the engine links them | `89b5e2f`, `c38d257` | [`todo-003-vendor-pixielib-glm-entt-2026-10-13.md`](todo-003-vendor-pixielib-glm-entt-2026-10-13.md) |
| PIXIE-ARCHIVE-016 | 2026-10-13 | completion | PIXIE-TODO-004 | claude | `scripts/check_docs.py` checks links, metadata, schedulers, and the archive router; each break seen failing | `512ca45` | [`todo-004-documentation-verifier-2026-10-13.md`](todo-004-documentation-verifier-2026-10-13.md) |
| PIXIE-ARCHIVE-017 | 2026-10-13 | completion | PIXIE-TODO-005 | claude | Input, `Camera2D`, GL wrappers, the instanced `ShapeRenderer`, the shapes sample, and CTest tests; measured rates | `d2c5205` | [`todo-005-input-camera-shapes-2026-10-13.md`](todo-005-input-camera-shapes-2026-10-13.md) |
