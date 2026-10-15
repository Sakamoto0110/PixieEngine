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
| PIXIE-ARCHIVE-018 | 2026-10-13 | completion | PIXIE-TODO-006 | claude | The `runtime` module: World over EnTT, Entity, Behaviours, phases, Random, built-in components; module documents and tests | `6c9bd8b` | [`todo-006-runtime-module-2026-10-13.md`](todo-006-runtime-module-2026-10-13.md) |
| PIXIE-ARCHIVE-019 | 2026-10-13 | completion | PIXIE-TODO-007 | claude | Anchor 1: the boids sample, steered toward or away from the mouse; 1,000 boids at 0.11 ms per step | `8162c4d` | [`todo-007-anchor-1-boids-2026-10-13.md`](todo-007-anchor-1-boids-2026-10-13.md) |
| PIXIE-ARCHIVE-020 | 2026-10-13 | completion | PIXIE-TODO-008 | claude | `SpatialGrid`, `AddSpatialIndex`, `Acceleration2D`, order-independence tests, and anchor 2, flocks: 5,000 boids at 7 ms per step | `147ec99` | [`todo-008-spatial-grid-flocks-2026-10-13.md`](todo-008-spatial-grid-flocks-2026-10-13.md) |
| PIXIE-ARCHIVE-021 | 2026-10-13 | completion | PIXIE-TODO-009 | claude | Particle pools, affectors, attractors, interaction, `ParticleRenderer`, and anchor 3: 100,000 particles and 16 attractors at 4.5 ms per step | `16112a2` | [`todo-009-particles-attractors-2026-10-13.md`](todo-009-particles-attractors-2026-10-13.md) |
| PIXIE-ARCHIVE-022 | 2026-10-13 | completion | PIXIE-TODO-010 | claude | `FixedStepper`, `RenderTarget`, field descriptors, `Project` and `ProjectSession`, the `capi` module with one library per anchor, and its C test | `1edd152` | [`todo-010-host-loop-reflection-capi-2026-10-13.md`](todo-010-host-loop-reflection-capi-2026-10-13.md) |
| PIXIE-ARCHIVE-023 | 2026-10-13 | completion | PIXIE-TODO-011 | claude | The C# ImGui editor: viewport, entities, PixieInspector over C API proxies, session controls; the three anchors run in it; an exit fault outside the engine noted | `b4a5f08` | [`todo-011-imgui-editor-2026-10-13.md`](todo-011-imgui-editor-2026-10-13.md) |
| PIXIE-ARCHIVE-024 | 2026-10-13 | fix | PIXIE-ARCHIVE-023 | claude | The editor exit fault: the ImGui backends kept a stale context; clearing it before `DestroyContext` (cause found in PixieInspector) gives 0 faults in 15 runs | `74771b8` | [`editor-exit-fault-fix-2026-10-13.md`](editor-exit-fault-fix-2026-10-13.md) |
| PIXIE-ARCHIVE-025 | 2026-10-13 | decision | owner decision | claude | The owner's review: single source of metadata, scenario values, time control, attractor size and death on contact, spawners, selection gestures | the commit that adds it | [`editor-review-decisions-2026-10-13.md`](editor-review-decisions-2026-10-13.md) |
| PIXIE-ARCHIVE-026 | 2026-10-13 | promotion | owner decision | claude | Five review blocks promoted as PIXIE-TODO-012 to PIXIE-TODO-016, with PIXIE-TASK-058 to PIXIE-TASK-077 | the commit that adds it | [`editor-review-promotion-2026-10-13.md`](editor-review-promotion-2026-10-13.md) |
| PIXIE-ARCHIVE-027 | 2026-10-13 | completion | PIXIE-TODO-012 | claude | Field metadata in the native registry, C API version 2, and the editor's inspectors built from the descriptors with PixieInspector 0.2.0; the proxies removed | `057f92d` | [`todo-012-single-source-metadata-2026-10-13.md`](todo-012-single-source-metadata-2026-10-13.md) |
| PIXIE-ARCHIVE-028 | 2026-10-13 | completion | PIXIE-TODO-013 | claude | Scenario components with a scene rebuild, `Project::Capture`, a session time scale, and the editor's time toolbar; measured time scales | `9beb8de` | [`todo-013-scenario-time-control-2026-10-13.md`](todo-013-scenario-time-control-2026-10-13.md) |
| PIXIE-ARCHIVE-029 | 2026-10-13 | completion | PIXIE-TODO-014 | claude | Attractors get a body size apart from their strength, `reach` replaces `radius`, and die on contact removes particles in the body | `bef16f9` | [`todo-014-attractor-size-2026-10-13.md`](todo-014-attractor-size-2026-10-13.md) |
| PIXIE-ARCHIVE-030 | 2026-10-13 | completion | PIXIE-TODO-015 | claude | `ParticleSystem` keeps the pool and `ParticleSpawner` entities feed it, with emission shapes, direction modes, and an `Entity` field kind the editor offers as a choice | `d9538c9` | [`todo-015-particle-spawners-2026-10-13.md`](todo-015-particle-spawners-2026-10-13.md) |
| PIXIE-ARCHIVE-031 | 2026-10-13 | completion | PIXIE-TODO-017 | claude | In the particles game, Alt + click adds a spawner at rate 0, and spawners are moved and removed as attractors are | `71a61ce` | [`todo-017-spawners-by-hand-2026-10-13.md`](todo-017-spawners-by-hand-2026-10-13.md) |
| PIXIE-ARCHIVE-032 | 2026-10-13 | completion | PIXIE-TODO-016 | claude | The editor's Game and Select modes: click, Shift, box, and Ctrl gestures, outlines, and inspectors over the whole selection, with mixed values marked by PixieInspector 0.2.1 | `c7988f3`, `a1606b2` | [`todo-016-viewport-selection-2026-10-13.md`](todo-016-viewport-selection-2026-10-13.md) |
| PIXIE-ARCHIVE-033 | 2026-10-14 | decision | owner decision | claude | Next phase: projects created and opened from the editor, portable, without PATH or git, together with scene serialization; the design decisions behind PIXIE-PROPOSAL-005 | the commit that adds it | [`projects-and-scenes-decisions-2026-10-14.md`](projects-and-scenes-decisions-2026-10-14.md) |
| PIXIE-ARCHIVE-034 | 2026-10-14 | promotion | PIXIE-PROPOSAL-005 | claude | PIXIE-PROPOSAL-005 promoted as PIXIE-TODO-018 to PIXIE-TODO-022, with PIXIE-TASK-080 to PIXIE-TASK-102 | the commit that adds it | [`projects-and-scenes-promotion-2026-10-14.md`](projects-and-scenes-promotion-2026-10-14.md) |
| PIXIE-ARCHIVE-035 | 2026-10-14 | completion | PIXIE-TODO-018 | claude | Scenes saved and loaded as JSON through the field descriptors: `SceneId`, descriptors that add and remove, unknown data kept; nlohmann/json 3.12.0 vendored | `816df46`, `635d70d` | [`todo-018-scenes-2026-10-14.md`](todo-018-scenes-2026-10-14.md) |
| PIXIE-ARCHIVE-036 | 2026-10-14 | completion | PIXIE-TODO-019 | claude | Project folders, sessions built from a scene with edit and play modes, the generic `pixie-scene-capi`, and C API version 4 | `38b397b` | [`todo-019-projects-2026-10-14.md`](todo-019-projects-2026-10-14.md) |
| PIXIE-ARCHIVE-037 | 2026-10-14 | completion | PIXIE-TODO-020 | claude | The editor's start screen, an ImGui folder picker, recent projects, save, play and stop, and creating entities and components; verified with real input | `ac89587` | [`todo-020-editor-projects-2026-10-14.md`](todo-020-editor-projects-2026-10-14.md) |
| PIXIE-ARCHIVE-038 | 2026-10-14 | completion | PIXIE-TODO-023 | claude | Promoted and completed at the owner's request: a centered start screen, the neko icon on the editor, a right-click viewport menu in place of Ctrl + click, and C API version 5 with view-to-world conversion | `5427c49` | [`todo-023-editor-polish-2026-10-14.md`](todo-023-editor-polish-2026-10-14.md) |
| PIXIE-ARCHIVE-039 | 2026-10-14 | completion | PIXIE-TODO-021 | claude | Projects with code: the engine as a subproject, `pixie_game`, the engine's player, code templates and their generator, the project-code test, and the editor's build screen; verified with real input | `6600b61` | [`todo-021-project-code-2026-10-14.md`](todo-021-project-code-2026-10-14.md) |
| PIXIE-ARCHIVE-040 | 2026-10-14 | completion | PIXIE-TODO-022 | claude | Boids, flocks, and particles as project folders with code and scenes saved by the editor, `SceneEntityWith`, `PROJECT_DIR`, and a project test per sample; verified from the command line | `e121214` | [`todo-022-samples-projects-2026-10-14.md`](todo-022-samples-projects-2026-10-14.md) |
| PIXIE-ARCHIVE-041 | 2026-10-14 | decision | owner decision | claude | Next phase: authoring in the viewport (moving, undo and redo, duplicating, deleting, renaming) before sprites and exporting; the engine does not depend on the editor | the commit that adds it | [`authoring-decisions-2026-10-14.md`](authoring-decisions-2026-10-14.md) |
| PIXIE-ARCHIVE-042 | 2026-10-14 | promotion | PIXIE-PROPOSAL-006 | claude | PIXIE-PROPOSAL-006 promoted as PIXIE-TODO-024 and PIXIE-TODO-025, with PIXIE-TASK-107 to PIXIE-TASK-115 | the commit that adds it | [`authoring-promotion-2026-10-14.md`](authoring-promotion-2026-10-14.md) |
