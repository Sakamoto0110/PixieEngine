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

## `PIXIE-TODO-051` — Shared engine export and local MCP development

**Parent:** PIXIE-TODO-051

- `PIXIE-TASK-201` — **Status:** done — Isolated shared-boundary gate passed Debug and Release on MSVC 14.51 with /MD(d): module/DLL typed World add/Each/remove and DLL World destruction, plus module-private descriptor registration and C API field read of 123. Matching type hash 2344671351 survives intentionally different sequential indices 1/0; vendored EnTT type_info equality uses the identifier hash. Deliberately changed module identity failed before GL (412652988 versus 2344671351); disabling the mutation and rebuilding restored Debug PASS. No identity workaround or EnTT setting change was required. Production migration remains unstarted at this safe checkpoint; the proof does not qualify final production ABI, unload/reload or cross-toolchain behavior.
- `PIXIE-TASK-202` — **Status:** in-progress — Resumed after gate 201 on 2026-10-28: adopt /MD and /MDd consistently across native engine, games, modules and third parties; obtain PixieLib build/test evidence and establish the app-local Visual C++ runtime dependency list. PixieLib's earlier standalone Debug/Release PASS used /MD in both. The resumed standalone Debug /MDd validation passed at baseline 21634516bf00014102b056ced02a0f44ad36d614: effective MultiThreadedDebugDLL in all three compiled targets, zero warnings/errors and CTest 1/1 including cross-module events. This closes the standalone /MDd prerequisite; production/vendor integration and app-local runtime closure remain pending.
- `PIXIE-TASK-203` — **Status:** in-progress — Resumed on 2026-10-28: build the existing native boundaries into one pixie shared library with boundary-specific export macros, retaining one-way Core/Platform/Render/App dependencies and keeping header-only templates available to game code. Implementation ownership acknowledged; final build/ABI qualification remains pending.
- `PIXIE-TASK-204` — **Status:** in-progress — Direct player links, development-module generation/loading and C API 17 module-load addition are being implemented, with per-session module leases and adapted editor/project-code tests. Editor Release build passed with zero warnings; native rebuild and module lifecycle acceptance remain pending. Preserve existing C API semantics and verify unload only after module-owned callbacks, descriptors and objects are released, without requiring a game-specific editor rebuild.
- `PIXIE-TASK-205` — **Status:** in-progress — The 2026-10-29 partial checkpoint implements separate shipping build/output, native manifest-only export and the editor command using the same CLI. Boids and no-code sprites export/run passed; the real editor command exported sprites successfully. Preserve the 23-case native harness and three local publish checks. Registered CTest, current Debug/Release regression matrix and final portability/asset coverage remain pending; do not infer completion from this bounded Windows result.
- `PIXIE-TASK-206` — **Status:** in-progress — The 2026-10-29 native harness passed all four export-barrier sensitivities and clean restorations: development Engine/configuration, module-loader player, unlisted content and missing dependency closure. Forwarder/path/junction cases also passed. Local editor publish guards passed three checks. On 2026-10-30 the MCP bridge, channel and dispatcher were asserted absent from the boids/sprites shipping engines and exported folders and from the published editor, in vs2026 and ci Debug/Release (code 066f097d705d6c11150b844338bf0677de1b3705). Actual remote-release outputs remain unqualified; optional headless Mesa execution and remote CI were not run. Keep this broader task open.
- `PIXIE-TASK-207` — **Status:** done — The accepted Engine/Inspector contract is implemented and bounded Windows integration passed at e64dbb765430093e88f1d66b09e2d8f9340efcd2: player world.* versus editor Inspector/history routes, scene IDs/generation, owner-thread after-Advance dispatch, native errors, snapshots/input/view and fixed-folder image content. Debug/Release each passed 39 editor MCP cases, 21 direct fault/pipe cases and 26 player cases. Existing Set/RunCommand/EditRecorder paths are reused; pending/refused/partial/no-op/live-read diagnostics remain distinct. Capture preserves fractional time. Channel save and indirect filesystem aliases stay disabled; no widget/golden approval or real Codex registration is implied.
- `PIXIE-TASK-208` — **Status:** done — Local Windows player and editor MCP paths were qualified at e64dbb765430093e88f1d66b09e2d8f9340efcd2, with actual editor shipping-publish exclusions and tool publish/NoBuild refusals checked. On 2026-10-30, code 066f097d705d6c11150b844338bf0677de1b3705 opened cold code projects through MCP (startup wait, job-contained host) and made channel save a build choice after the owner's answer; tests/mcp/qualify_mcp.py passed 135 cases in vs2026 Debug/Release and 128 in ci Debug/Release, and a Claude Code client drove the editor. Codex execution, Unix transport, remote operation and project-wide preset/artifact/remote-release reconciliation remain under 209/210 and are not implied. Preserve opt-in local operation, engine ownership of state and the C API/editor dependency direction.
- `PIXIE-TASK-209` — **Status:** open — After the accepted delivery tasks, including 211/212 capture work, execute the full qualification matrix: vs2026 and ci Debug/Release, project-code, static sample equality and dynamic sample review, editor opening boids/particles/sparks/a project with code, boids/no-code sprites isolated exports, before/after sizes and all barrier sensitivity checks. Separate executed evidence from planned checks and collaborator reports.
- `PIXIE-TASK-210` — **Status:** in-progress — Reconcile current build/API/dependency documentation with source and qualified milestones; the shared DLL now exposes API18 development dispatch and the Windows editor/player MCP routes have bounded acceptance; the editor README, development guide and qualification record describe the 2026-10-30 MCP closure. Record final full code hashes, cross-project acceptance, LogicSandbox breaks and unresolved limits, then archive only if every delivery/qualification gate is satisfied. Physics D3/D4/D5 acceptance was recorded independently under PIXIE-TODO-043 on 2026-10-31. Code/documentation commits remain separate; no push or publication is authorized.
- `PIXIE-TASK-211` — **Status:** done — On 2026-10-29, development player/sample step-count, initial-seed and framebuffer-size options plus editor step termination passed local vs2026 and Ninja Debug/Release matrices (19/19 each). Exact 0/1/17 counts and alpha one, animated repetition across frame caps, step-budget precedence, seed sensitivity without an authored override, editor repetition/collapsed layouts and shipping/publish exclusions were qualified. Authored scene seeds retain precedence and old wall-time options remain. Future channel readback integration belongs to 207/208; approved references remain separately pending under 212.
- `PIXIE-TASK-212` — **Status:** in-progress — Capture/comparison and candidate-update tooling is being qualified with synthetic fixtures; same-renderer repetition and mutation/restoration passed locally, while snapshot registration checks are in progress. Actual approved per-sample scenarios remain pending: no owner-selected sample list, tolerance policy or reference image is implied. Preserve step/seed/size/renderer/tolerance inputs, CTest comparison and actual/diff outputs in build storage. Do not invoke reference approval or replace references without the owner's decision.

## `PIXIE-TODO-052` — 2D lighting

**Parent:** PIXIE-TODO-052

- `PIXIE-TASK-213` — **Status:** done — L0: pin the lighting data model, units, composition order, compatibility rule and shadow technique by measured prototypes on the target GPU and Mesa llvmpipe; owner review of the pinned choices before further lighting code. On 2026-10-31 the [L0 design record](docs/lighting-l0-design.md) pinned them; the `lighting-prototype` tool (code b12bb4ccae0b0365f392584a3ba71bccaa7b4245) compared shadow volumes, polar maps and a distance field against an exact reference on the RX 7600 and on Mesa 26.2.4 llvmpipe, and shadow volumes with analytic penumbra won on accuracy, leaks and flicker, and on cost everywhere except two 1280x720, 64-light GPU loads where the distance field was 0.15–0.28 ms cheaper. The owner waived the review ("Yes, continue to L1"); it was not performed, and the choices stay revisable.
- `PIXIE-TASK-214` — **Status:** done — L1: lighting settings, ambient, point, spot and directional lights, light layers, unlit screen UI, linear accumulation, exposure and tone mapping; pixel-identical captures of scenes without lighting. Code d7147f1bf95df06d9f598acb011be988001e47a4 on 2026-10-31: LightingSettings2D, Light2D and LightReceiver2D; the render module's LightRenderer with receiver passes in SpriteRenderer and ShapeRenderer; the lit path in ProjectSession::Render only with settings. `lighting-tests` (9 tests: byte-identical neutral settings, inert lights without settings, falloff also at zoom 2, cone, directional, layers, unlit receivers, exposure and Aces, unlit screen canvas) passed on the RX 7600 and on Mesa 26.2.4 llvmpipe; test-only mutations dropping the layer test and the falloff exponent failed 2 tests (11 checks) and the exact restoration passed. 11 sample captures (12 steps, seed 42, 320x180) were byte-identical between the L0 and L1 Release builds; hello-gl differs between two runs of the same build, so it is not comparable. vs2026 and ci Debug/Release built with 0 warnings and passed 23/23 tests each; the editor built with 0 warnings. The L0 state (e16732b) also passed ci Debug/Release 22/22. Light-layer names, Inspector tools and paths/particles as receivers are later stages; no visual review was made (by instruction, visual results are assumed acceptable and remain open to the owner's review).
- `PIXIE-TASK-215` — **Status:** open — L2: occluders and soft shadows from colliders, polygons or sprite alpha, including physics-driven occluders and opt-in character shadows; leak, flicker and render-rate cases; rooms 1 and 2.
- `PIXIE-TASK-216` — **Status:** open — L3: emissive sprites, shapes and text with transitions; emissive and light-casting particle pools with caps; room 3.
- `PIXIE-TASK-217` — **Status:** open — L4: Inspector authoring, gizmos, debug views, telemetry with CPU and GPU timing, budgets and quality presets, optional normal maps, ambient occlusion, bloom and fog; room 4.
- `PIXIE-TASK-218` — **Status:** open — L5: consolidation: regression matrix, approved references, stress report, residual limits and the owner's visual review.
