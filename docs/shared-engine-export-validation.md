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
TASK-202 remains open for the production migration, app-local runtime closure
and, if standalone PixieLib evidence is required, explicit /MDd Debug
qualification. The collaborator reported publishing its documentation record;
that does not broaden this task's remote authority, and the coordinator
requested no further pushes.

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
