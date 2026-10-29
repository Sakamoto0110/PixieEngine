# PixieEngine Task List

**Document ID:** GLOBAL-TASKLIST

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** task breakdowns of promoted TODO.md entries

**Surface:** task-scheduler

**Boundary:** global

**Authority role:** scheduler

**Mutation:** authored

**Indexing:** include

## Purpose

This file is a child scheduler of [`TODO.md`](TODO.md). Each section breaks
one promoted `TODO.md` entry into tasks, so that large entries, or entries made
of many small tasks, do not pollute `TODO.md`.

- Every section names its parent in `Parent`, and the parent links back here
  in `Breakdown`.
- Tasks decompose the parent's accepted scope and never extend it.
- When the parent completes, its section moves to
  [`docs/archive/`](docs/archive/README.md).

The full rules are in the
[documentation policy](docs/governance/documentation-policy.md#scheduling-todomd-and-tasklistmd),
and the section shape is in the
[template](docs/templates/TASKLIST-SECTION.md).

## Open breakdowns

## `PIXIE-TODO-043` — Rigid body dynamics

**Parent:** PIXIE-TODO-043

The pre-DLL allocation observations below qualify their recorded statically
linked baselines. After shared-engine migration, the executable-local new/new[]
counter does not observe Engine/backend allocation sites in pixie.dll; current
host-only counts do not requalify those earlier claims. Native CPU/population
telemetry and pending physics/owner acceptance remain separate.

- `PIXIE-TASK-180` — **Status:** done — D0: pin ownership, units, compatibility bridge and initial contracts in the body dynamics guide; full editor migration remains tracked in D3.
- `PIXIE-TASK-181` — **Status:** done — D1: native/scene body baseline, world settings, mass/rotation/commands and lifecycle; 137 runtime tests per configuration and 13 Release integration scenarios passed. Compound scene-list authoring remains D3.
- `PIXIE-TASK-182` — **Status:** done — D2: copied contact/sensor lifetimes, scene identities, sensor-inclusive queries and bounded material/stack/fast-body qualification; 157 runtime tests per configuration, omitted-sensor sensitivity and 13 Release integration scenarios passed. Transient moving-wall overlap is documented.
- `PIXIE-TASK-183` — **Status:** in-progress — D3: presets, admission/minimum guards, full collider dimensions, compound authoring and named collision layers delivered. Native record lists, fixed-slot name configuration, root/compound/mixed-selection checkboxes and history/save/Play/Stop validated with 176 tests per configuration, 15 Release integration scenarios and editor captures. Gravity migration delivered: CharacterMotor2D version 2 scales world gravity, and version 1 saves convert through a descriptor upgrade (no settings, zero and differing legacy values, unknown fields, a frozen movement-lab v1 fixture); 181 tests per configuration and 15 integration scenarios in Debug and Release, with a disabled-upgrade sensitivity failure. Native read-only effective mass/solid area/local center/inertia inspection is delivered through C API 13; single/compound bodies, density/size/override edits, snapshot restoration, invalid-body recovery and Play/Stop have bounded regression coverage. Native collider resize/rotation tools are implemented through C API 14 for boxes/circles/polygons and individual compound items, with 186 runtime tests, 14 Debug and 15 Release CTest suites, and scripted editor history/lifecycle checks. Upright capsule radius/full-height handles are implemented through C API 15 using the shared world-scale-aware native capsule validator; 189 runtime tests, 14 Debug and 15 Release CTest suites, and scripted capsule history/lifecycle checks passed. Native physics debug views are implemented through C API 16: collider/sensor outlines, backend origins, dynamic centers, linear velocities, awake/asleep state and current manifold normals; 193 runtime tests, 14 Debug and 15 Release CTest suites, session isolation, native editor/player captures and sensor sensitivity passed. Scene sync/rebuild, motor, event/pose publication telemetry and retained sync buffers are delivered, with 195 runtime tests, 14 Debug and 15 Release CTest suites. Seeded 0/100/500-body awake/sleeping headless qualification retained 43,200 warm ticks across collection on/off and a disabled-probe reference, plus allocation sensitivity; this is sparse-world CPU evidence, not a universal budget. Extended sparse/compound/stack/sensor fixtures now retain 172,800 warm ticks across 144 runs at 0/100/500 dynamic bodies, including actual populations, event churn, ordinary allocator counts and sampled machine activity; 197 runtime tests and all 15 CTest suites passed in Debug and Release, with a detected sensor-classification regression and exact restoration. Physical-pointer acceptance, debug-view owner acceptance, mixed kinematic/character workloads and coupled workload qualification remain pending.
- `PIXIE-TASK-184` — **Status:** in-progress — D4 native prepare/solve/finish coupling, bounded pushing, platform carry, blocked/crush reporting and regression fixtures are implemented and included in the 235-test Debug/Release matrix. Independent owner feel acceptance and coupled workload qualification remain pending.
- `PIXIE-TASK-185` — **Status:** open — D5: regressions, migration, stress report, remaining limits and owner feel review.

## `PIXIE-TODO-051` — Shared engine export and local MCP development

**Parent:** PIXIE-TODO-051

- `PIXIE-TASK-201` — **Status:** done — Isolated shared-boundary gate passed Debug and Release on MSVC 14.51 with /MD(d): module/DLL typed World add/Each/remove and DLL World destruction, plus module-private descriptor registration and C API field read of 123. Matching type hash 2344671351 survives intentionally different sequential indices 1/0; vendored EnTT type_info equality uses the identifier hash. Deliberately changed module identity failed before GL (412652988 versus 2344671351); disabling the mutation and rebuilding restored Debug PASS. No identity workaround or EnTT setting change was required. Production migration remains unstarted at this safe checkpoint; the proof does not qualify final production ABI, unload/reload or cross-toolchain behavior.
- `PIXIE-TASK-202` — **Status:** in-progress — Resumed after gate 201 on 2026-10-28: adopt /MD and /MDd consistently across native engine, games, modules and third parties; obtain PixieLib build/test evidence and establish the app-local Visual C++ runtime dependency list. PixieLib's earlier standalone Debug/Release PASS used /MD in both. The resumed standalone Debug /MDd validation passed at baseline 21634516bf00014102b056ced02a0f44ad36d614: effective MultiThreadedDebugDLL in all three compiled targets, zero warnings/errors and CTest 1/1 including cross-module events. This closes the standalone /MDd prerequisite; production/vendor integration and app-local runtime closure remain pending.
- `PIXIE-TASK-203` — **Status:** in-progress — Resumed on 2026-10-28: build the existing native boundaries into one pixie shared library with boundary-specific export macros, retaining one-way Core/Platform/Render/App dependencies and keeping header-only templates available to game code. Implementation ownership acknowledged; final build/ABI qualification remains pending.
- `PIXIE-TASK-204` — **Status:** in-progress — Direct player links, development-module generation/loading and C API 17 module-load addition are being implemented, with per-session module leases and adapted editor/project-code tests. Editor Release build passed with zero warnings; native rebuild and module lifecycle acceptance remain pending. Preserve existing C API semantics and verify unload only after module-owned callbacks, descriptors and objects are released, without requiring a game-specific editor rebuild.
- `PIXIE-TASK-205` — **Status:** in-progress — The 2026-10-29 partial checkpoint implements separate shipping build/output, native manifest-only export and the editor command using the same CLI. Boids and no-code sprites export/run passed; the real editor command exported sprites successfully. Preserve the 23-case native harness and three local publish checks. Registered CTest, current Debug/Release regression matrix and final portability/asset coverage remain pending; do not infer completion from this bounded Windows result.
- `PIXIE-TASK-206` — **Status:** in-progress — The 2026-10-29 native harness passed all four export-barrier sensitivities and clean restorations: development Engine/configuration, module-loader player, unlisted content and missing dependency closure. Forwarder/path/junction cases also passed. Local editor publish guards passed three checks. Future MCP operations/adapters/dependencies still need exclusion qualification, along with actual remote-release outputs; optional headless Mesa execution and remote CI were not run. Keep this broader task open.
- `PIXIE-TASK-207` — **Status:** in-progress — Reconcile the owner's accepted proposals 013/014 with the Inspector integration contract: local single-client IPC, namespaced World versus Inspector/editor paths, scene-ID/field-name addressing, native metadata, hello protocol/API versions, session play/stop/pause/resume/step/advance/time_scale/reset, input/view, C API snapshot/restore, and fixed-folder capture returned as image content. Preserve once-per-frame dispatch between Advance and Render, errors with PixieResult/last_error and existing CLI behavior. Reuse --set/--do plus EditRecorder/history for Inspector writes and qualify tree/get/report/history/selection readback. Player exposes only world.*; channel save, including editor.do save, remains disabled until owner question 2 is answered.
- `PIXIE-TASK-208` — **Status:** open — After 207, implement and qualify the agreed Engine/Inspector MCP development path; prove opt-in local operation and absence of development-operation code/dependencies from publish/remote-release artifacts. Keep engine ownership of state and the C API/editor dependency direction.
- `PIXIE-TASK-209` — **Status:** open — After the accepted delivery tasks, including 211/212 capture work, execute the full qualification matrix: vs2026 and ci Debug/Release, project-code, static sample equality and dynamic sample review, editor opening boids/particles/sparks/a project with code, boids/no-code sprites isolated exports, before/after sizes and all barrier sensitivity checks. Separate executed evidence from planned checks and collaborator reports.
- `PIXIE-TASK-210` — **Status:** in-progress — Reconcile current build/API/dependency documentation with source and qualified milestones; Core/Platform/Render/App surfaces and shared-DLL/API-17 module routes are being updated. Record final full code hashes, cross-project acceptance, LogicSandbox breaks and unresolved limits, then archive only if every delivery/qualification gate is satisfied. Keep physics D3/D4/D5 acceptance pending independently. Code/documentation commits remain separate; no push or publication is authorized.
- `PIXIE-TASK-211` — **Status:** open — Implement proposal 014's repeatable player/sample capture options --quit-after-steps, --seed and --size, and editor step-count termination. Render the final fixed-step capture with alpha 1; preserve --quit-after/--screenshot semantics and verify repeated same-renderer runs produce the same frame. Reuse readback for channel capture without admitting development tooling to exported/published artifacts.
- `PIXIE-TASK-212` — **Status:** open — Qualify approved per-sample snapshot scenarios with step/seed/size/renderer/tolerance inputs, CTest comparison and actual/diff outputs in build storage. Provide explicit reference-update tooling; do not replace or declare approval of reference images without the owner's decision. Record the unresolved initial sample selection and tolerance-policy questions of proposal 014 before treating either recommendation as a chosen default.
