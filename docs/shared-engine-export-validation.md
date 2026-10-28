# Shared Engine, Export and Local Development Qualification

**Document ID:** GLOBAL-SHARED-ENGINE-EXPORT-VALIDATION

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** qualification plan and recovery boundaries for shared engine distribution and the isolated local MCP development channel

**Surface:** guide

**Boundary:** global

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

## Scope and status

[PIXIE-TODO-051](../TODO.md#pixie-todo-051--shared-engine-game-export-and-local-mcp-development-channel)
owns accepted scope and gates. [TASKLIST.md](../TASKLIST.md#pixie-todo-051--shared-engine-export-and-local-mcp-development)
owns progress. This guide maps that scope to evidence; it does not authorize
additional work or claim that any row has passed.

The implementation baseline is the local native split at
`f634eeeef474eac34b780fc2e43f11c239e2cd6d`. The owner promoted distribution and
the MCP extension on 2026-10-28. Proposals
[013](proposals/dev-channel.md) and [014](proposals/deterministic-capture.md)
were unavailable in the initial inspection and subsequently supplied and
explicitly included by the owner. Use those records and the accepted
clarifications; do not reconstruct an assumed earlier specification.

The first checkpoint is the EnTT cross-DLL proof. The isolated proof passed;
production migration remains unstarted at the safe checkpoint below. The
shared engine/export work does not close physics D3/D4/D5,
owner feel acceptance or coupled workload qualification under PIXIE-TODO-043.

The resumed DLL/module foundation is preserved in local code commit
`70e3524455386e648ff8bb1ec62cc1c0e1b89014`, parent
`ce85c8442970f8c49e8858cd190f01f9ae573e7d`. It is a partial delivery checkpoint,
not TODO-051 completion. Follow-up source commit
`9abd26f540d1db4ffcf72ee831bc25ad7a83a89f` sets position-independent code
before creating the embedded native dependencies, covering static GLEW,
PixieLib math and Box2D as well as the engine objects. This corrects the
shared-library build wiring; it is not a Linux build qualification.
Documentation is committed separately. The executed
matrix below distinguishes earlier full runs, final targeted reruns and work
not performed; the code commit alone is not proof of the remaining gates.

## Qualification matrix

| Claim | Required evidence and failure signal | Dependency |
|---|---|---|
| Module component identity/storage crosses the DLL boundary | A component defined in a module is added, iterated and removed by World code in the DLL; descriptor registration and a C API field read agree. Record teardown while module code remains loaded. Type-ID disagreement stops migration and requires options/costs. | First gate; before production migration |
| Shared native allocation/runtime assumptions hold | Debug /MDd and Release /MD throughout native and third-party targets; PixieLib validation identifies commands/configurations and any static-runtime assumptions. Record app-local runtime filenames and provenance. | EnTT gate, compatible compiler/runtime settings |
| Engine code is shared once | Final target/artifact graph, boundary-specific symbol exports, directly linked player game code, module containing game code and dependency closure. Do not equate one DLL with collapsed layer dependencies. | Runtime migration and exports |
| Editor retains its C boundary | Existing C functions/semantics remain; additions raise API version. Open boids, particles, sparks and a project with code, including module/session teardown without calling unloaded code. Editor must not need recompilation for each game. | Shared library, module contract, editor consumer |
| Export is an allowlisted portable game | Native tool and editor command use one implementation. Record admitted player/engine/runtime/dependency files and project/scenes/assets; use boids and no-code sprites in initially empty destinations, run with --quit-after 2 --screenshot, inspect output and exit code. No additional export format or installer is implied. | Export manifest, app-local dependencies |
| Barrier 1: export configuration omits development code | Inspect effective configuration and artifacts; deliberately re-enable development-only code in an isolated copy and prove the qualification rejects it. Ordinary Release alone is not proof of a development-free configuration. | Production/development target separation |
| Barrier 2: exported player cannot load a game module | Verify the direct game link and absence of the development module-loading path; introduce that path in an isolated copy and prove rejection. | Player/module separation |
| Barrier 3: only manifest entries are copied | Compare output against the manifest; deliberately copy an unlisted development artifact and prove rejection. Cover MCP adapters/tools/configuration as well as game modules. | Manifest implementation |
| Barrier 4: clean-folder export qualification catches contamination and broken closure | Reject development files and native imports not satisfied by shipped files or Windows; run the game from the isolated destination. Demonstrate rejection of contamination and a missing required dependency. Record the scanner/test commands and observed failures. | All previous export mechanisms |
| MCP operates only through the agreed local development path | Qualify each accepted command group in memory, plus a real single-client pipe/socket integration. Verify namespaced routing, scene IDs/field names, once-per-frame dispatch after Advance/before Render even while paused, and PixieResult/last_error diagnostics. Player accepts only world.*; preserve native state/descriptor ownership. | Inspector contract checkpoint, existing native/editor paths |
| Session commands execute the requested simulation change | hello reports protocol/API versions; play/stop/pause/resume/step/advance/time_scale/reset produce the expected session state. step/advance responses follow execution, not mere enqueueing. Verify types/type metadata, C API snapshot/restore and input/view command groups. | Native dispatcher and safe dispatch point |
| Inspector channel tests the actual inspection/history path | tree/get/report/history/selection read back the inspection state; inspector.set/editor.do reuse existing handlers and EditRecorder. Compare an edit against world.get, undo and compare again; exercise mixed values and refused writes. Disable save through every channel entry point, including editor.do, until owner question 2 is resolved. World writes do not claim Inspector/history verification. | Existing editor handlers, Inspector contract |
| Capture produces an image and cannot escape its destination | Fix the capture directory at channel open, reject escaping names, render/read the requested frame and return PNG image content through MCP. Verify scene state and frame timing, not merely a path-shaped response. | Shared readback, channel host, MCP adapter |
| Fixed-step captures and snapshot comparisons are repeatable | Qualify --quit-after-steps/--seed/--size and editor step termination with final alpha 1, preserving old options. Record repeated same-renderer comparisons, step/seed/size, approved reference hashes and scenario tolerances. Mismatch produces actual/diff paths in build storage. Do not approve reference changes automatically or present proposed sample/tolerance choices as owner decisions. | Proposal 014 implementation and owner-approved references |
| MCP is absent from publish and remote-release | Identify actual publish/remote-release artifact producers and inspect their outputs/dependency closures. No development-operation implementation, endpoint startup, adapter, tool or configuration may ship there. Break exclusion in an isolated copy and demonstrate detection. Game-export checks alone do not prove package/release exclusion. | Agreed channel and separate development artifacts |
| Native build/test compatibility survives | vs2026 and ci presets configure and build Debug/Release with zero warnings; CTest passes in both, including project-code building a player and development module against the shared engine. Record renderer/toolchain and any environment limitation. | Final production sources and target graph |
| Rendering remains equivalent | Shapes, sprites, hello-gl and particles are pixel-identical to the qualified split baseline; boids/flocks start, retain appearance and exit zero. Record deterministic capture inputs, comparison method and who performed visual review. Restore any temporary instrumentation before final build claims. | Same driver, captures, final rendering code |
| Distribution reduces duplication | Before/after sizes of players, old per-game C API libraries, new game modules and shared engine, with configuration and exact binaries/revisions. Historical proposal estimates are not measurements. | Baseline preservation, final artifacts |
| Consumers and documentation can resume accurately | PixieLib/Inspector responses distinguish recommendations from executed acceptance. Record final full code hashes, package/binary provenance, LogicSandbox header/build/loading breaks without migrating it, documentation checks and unmet rows. | Final implementation and consumer feedback |

## Evidence and recovery

### Resumed production DLL/module foundation, 2026-10-28

The implementation working tree built vs2026 Debug and Release with zero
warnings/errors. Debug CTest passed 17/17 in 94.05 seconds; Release passed
17/17 in 91.29 seconds. These runs include the external project-code build,
shared-engine game modules, engine/runtime tests and module-lifetime fixture.
The lifetime fixture rejects an invalid factory image, clears the default
module selection while a live boids session still steps/resets and reads its
descriptors, then destroys the session. It does not by itself qualify arbitrary
hot reload or mixed compiler/configuration modules.

An earlier Debug run passed 14/15 but failed player-input tests because static
GLFW was duplicated between the host and DLL. Sharing glfw3.dll corrected that
state/TLS split; the complete rerun above passed. GLEW state is embedded/exported
once by the engine. No vendored source edit or suppression of class-export
warnings was used for those fixes.

The editor Release build passed with zero warnings. Launches for boids,
particles and sparks exited zero and produced viewport captures, inspected
by the principal coordinator. The bare --game particles/sparks logs reported
missing font assets, so those captures prove geometry/viewport opening but
do not qualify complete HUD/text display. Asset-root/project launches and the
project-with-code editor opening remain to be closed. The initial code-project
fixture lacked a start scene; it is being extended and requalified before an
editor opening is counted.

The code-project fixture was then extended with a Spinner/triangle scene and
player execution. Its targeted Release CTest passed 1/1 in 75.12 seconds.
The editor opened that project through CodeTest-module.dll and exited zero;
its log records a Spinner.speed edit through the existing Inspector path,
undo/redo and Play/Stop with retained history labels. The captured frame shows
the project/Inspector. This is module/editor lifecycle evidence, not an MCP
execution or an independent proof of every post-gesture native value.

Before the next checkpoint, the coordinator added an ABI-valid module whose
factory returns null. Session creation rejects it with an internal error and
null output rather than silently falling back to the scene-only project.
The affected module-lifetime/generic/boids cases passed 3/3 in Debug (5.33
seconds) and Release (4.33 seconds) after that change. These targeted reruns
do not relabel the earlier complete matrices as executions of later source.

The active solution's generated native project settings were inspected for
34 targets in both configurations: 68 entries, zero runtime mismatches
against MultiThreadedDebugDLL in Debug and MultiThreadedDLL in Release. This
checks selected build properties, not the final exported folder's runtime
dependency closure. The gate fixture was also rebuilt against the current
sources and passed Debug 1/1 in 2.83 seconds; its current-source Release gate
was not rerun. The earlier pre-migration Release gate remains historical.

At the owner's subsequent checkpoint request, stop after this DLL/module
foundation. CI-preset qualification, development-free output/configuration,
manifest export and four barrier sensitivities, MCP, deterministic capture,
snapshot approvals and full sample-project asset-root visual checks remain
pending. Bare --game font warnings above are not treated as fixed. No push
or publication accompanies the checkpoint.

These are staged milestone results, not the final CI/export/MCP or
deterministic snapshot matrix. Later edits require the corresponding checks
again. The foundation code hash is recorded above; its full-matrix versus
targeted-rerun limits remain part of the evidence. No TODO-051 completion is
inferred here.

The physics benchmark's executable-local replacement of global new/new[] no
longer observes Runtime allocation sites moved into pixie.dll. Its new runs
are host-allocation observations only, not a repetition of the pre-DLL Engine
allocation qualification. CPU/population collectors retain their native
scope. See [physics instrumentation limits](physics-telemetry.md) before
interpreting allocation CSV. No new cross-DLL allocator instrument or physics
acceptance is implied by passing the build/test matrix.

### Resumed Inspector contract review, 2026-10-28

The Inspector collaborator rechecked public API 0.3.0 at
`d9716b130d7ee4d426e1e71c8dd90cf3772075fe`, with all-route VAW compatibility,
and supplied a proposed host integration contract and 26 acceptance cases.
That review ran no executable tests and changed no API, code, package or
published artifact. The cases are a preparation/acceptance plan; they are not
26 passed tests or a new mandatory Inspector feature set.

The plan separates held/view values from explicit live/storage reads, creation
failures in Inspector.Report from current node/command failures, and handler
execution from binding success. Host responses must not erase command-local
refusals through a later successful read, imply multi-edit rollback, apply
pending edits while reading or reuse nodes across selection/session generations.
Existing --set/--do handlers and EditRecorder remain the write/history path.
Those observations qualify the already accepted readback/history semantics;
they do not require exact widget-formatted text or a new public Inspector API.
Any generic fixture result still needs separate real editor/MCP qualification.

### Inspector managed fixture qualification, 2026-10-28

The next Inspector execution used the public 0.3.0 package in an isolated cache
and passed five public-API fixture cases (I03, I08, I10, I14 and I15), totaling
26 assertions: view/storage units, pending manual binding without implicit
apply, rule refusal despite a handler returning true, partial multi-target
effects and preservation of a failed-command receipt through later reads and
recovery. A copied harness using only handler true as the success oracle
failed I10 as intended. Restoring its source and explicitly rebuilding passed
again; the evidence distinguishes stale incremental output from the rebuilt
restoration.

This is managed fixture evidence from the Inspector collaborator, not the
real Engine handler, native state, EditRecorder/history, IPC/MCP or widget
acceptance. Those host checks remain pending. No source/API/package/version
change or publication accompanied the fixture run.

### Observed EnTT prerequisite and safe stop, 2026-10-28

The standalone `tests/shared-boundary` experiment built a shared engine,
game module and host with MSVC 14.51 and /MD(d), leaving the production target
configuration unchanged. Its single CTest case passed Debug (2.46 seconds
total) and Release (3.59 seconds total) on the owner's AMD Radeon RX 7600,
OpenGL 4.5 core. These are test durations, not performance measurements.

A module adds a typed component to a DLL-owned World; DLL code iterates it,
adds/removes another instance, and the module iterates the remaining value
before DLL-side World destruction. Separately, a component private to the
module is described through ComponentTypes and read through the C API as
`ModulePrivate.value == 123`; the session is destroyed before unloading the
module. The fixture's shared component hash is 2344671351 on both sides,
although deliberately primed sequential indices differ (module 1, DLL 0).
The vendored EnTT `type_info` equality compares the identifier hash, so these
different sequence indices do not mean incompatible identity.

With `BOUNDARY_BREAK_IDENTITY=ON`, the module's intentionally different type
hash 412652988 fails the gate against DLL hash 2344671351 before GL starts
(0.85 seconds total). Reconfigure OFF, rebuild and rerun restored Debug PASS
(1.02 seconds total). No custom type-ID override or EnTT setting change was
needed. The fixture and logs were read for this documentation checkpoint; this
writer did not run the native experiment. The local gate code commit is
`d4be5a4a244712b2031a0762169b27136d6d09fe`. The coordinator also checked the
generated gate vcxproj runtime selections: /MDd in Debug, /MD in Release.

At the owner's request for a safe checkpoint, stop before production migration.
This result qualifies the matching-toolchain fixture, not arbitrary compiler
or standard-library ABIs, hot reload, the final production export macro set,
or the full module lifecycle matrix. The production shared-runtime/DLL switch,
game modules, export barriers, MCP channel and deterministic capture/snapshot
delivery are not implemented or qualified by this experiment. Keep their
delivery and verification tasks open. No remote action is authorized.

### PixieLib collaborator result, 2026-10-28

The PixieLib thread reported validation at code baseline
`2f2c9444cfdac8e9fb7ab36c4c51885c5c11327a`, with its documentation record in
`21634516bf00014102b056ced02a0f44ad36d614`. Its standalone configuration used
`MultiThreadedDLL` (/MD) in **both Debug and Release**, with one CTest case
passing in each and zero warnings reported. It reported no CMake or API change
and five successful CI jobs. This is collaborator-reported evidence, not a
native rerun by this documentation writer.

Do not describe that PixieLib Debug result as /MDd. The isolated Engine gate
qualified /MDd Debug separately; these are different input configurations.
At that checkpoint TASK-202 remained open for the production migration,
app-local runtime closure and explicit standalone /MDd Debug qualification.
The resumed result below closes the last of those prerequisites only.
The collaborator reported publishing its documentation record;
that does not broaden this task's remote authority, and the coordinator
requested no further pushes.

### Resumed PixieLib /MDd prerequisite, 2026-10-28

After resumption, the PixieLib thread performed a separate standalone Debug
build at `21634516bf00014102b056ced02a0f44ad36d614` with
`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDebugDLL`. Its effective generated
properties and compiler command records confirm /MDd for `pixie_math`,
`pixie_tests_module` and `pixie_tests`. Configure/build/test exited zero,
with zero warnings/errors and CTest 1/1 passing (1.26 seconds total), including
the existing events-across-modules checks. Directive/import inspection reports
MSVCRTD and Debug runtime dependencies. The toolchain was MSVC 19.51.36260.0,
toolset 14.51.36231, Visual Studio 18 2026 x64 and CMake 4.3.1-msvc1.

The coordinator read the result, effective target properties and CTest log;
the execution belongs to the PixieLib thread. No API/CMake/versioned-source
change, commit, push or publication occurred in this resumed validation.
Release was not repeated. This new /MDd result remains separate from the
earlier /MD-in-both-configurations run. It qualifies the standalone checkout,
not the Engine vendor snapshot, production target migration, final DLL ABI,
runtime deployment or MCP/export exclusion. TASK-202 remains in progress.

Keep in-progress commands, logs, snapshots and measurements in unversioned
working evidence. No versioned link points into that local evidence. A recovered
session first checks HEAD, dirty files, the EnTT gate and the last recorded
successful operation; it never assumes an interrupted build, export or test
completed. Shared-runtime experiments do not establish the final production
target configuration.

At delivery, reconcile current technical/build/dependency documentation with
source and preserve the scoped matrix results, failures/limits, full code hashes
and the child task snapshot in the completion archive. Leave unqualified tasks
open. Documentation structure validation is separate from native/runtime,
visual, export and package-publication evidence. This task authorizes no push
or remote publication.
