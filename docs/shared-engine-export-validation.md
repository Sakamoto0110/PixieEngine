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

The first checkpoint was the EnTT cross-DLL proof. The isolated proof passed;
the later DLL/module foundation and its limits are recorded separately below. The
shared engine/export work does not close physics D3/D4/D5,
owner feel acceptance or coupled workload qualification under PIXIE-TODO-043.

The resumed DLL/module foundation is preserved in local code commit
`70e3524455386e648ff8bb1ec62cc1c0e1b89014`, parent
`ce85c8442970f8c49e8858cd190f01f9ae573e7d`. It is a partial delivery checkpoint,
not TODO-051 completion. Follow-up source commit
`9abd26f540d1db4ffcf72ee831bc25ad7a83a89f` sets position-independent code
before creating the embedded native dependencies, covering static GLEW,
PixieLib math and Box2D as well as the engine objects. This corrects the
shared-library build wiring. The coordinator reported successful Windows
configure and pixie_shared builds in Debug and Release with zero warnings
after that configuration-only fix; this was not another complete CTest run
and is not a Linux build qualification.
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
| Inspector channel tests the actual inspection/history path | tree/get/report/history/selection read back the inspection state; inspector.set/editor.do reuse existing handlers and EditRecorder. Compare an edit against world.get, undo and compare again; exercise mixed values and refused writes. Channel save follows the owner's 2026-10-30 answer to question 2: editor.do save reuses the save handler in development builds and `-p:PixieChannelSave=false` removes it; qualify both builds, and world.* never saves. World writes do not claim Inspector/history verification. | Existing editor handlers, Inspector contract |
| Capture produces an image and cannot escape its destination | Fix the capture directory at channel open, reject escaping names, render/read the requested frame and return PNG image content through MCP. Verify scene state and frame timing, not merely a path-shaped response. | Shared readback, channel host, MCP adapter |
| Fixed-step captures and snapshot comparisons are repeatable | Qualify --quit-after-steps/--seed/--size and editor step termination with final alpha 1, preserving old options. Record repeated same-renderer comparisons, step/seed/size, approved reference hashes and scenario tolerances. Mismatch produces actual/diff paths in build storage. Do not approve reference changes automatically or present proposed sample/tolerance choices as owner decisions. | Proposal 014 implementation and owner-approved references |
| MCP is absent from publish and remote-release | Identify actual publish/remote-release artifact producers and inspect their outputs/dependency closures. No development-operation implementation, endpoint startup, adapter, tool or configuration may ship there. Break exclusion in an isolated copy and demonstrate detection. Game-export checks alone do not prove package/release exclusion. | Agreed channel and separate development artifacts |
| Native build/test compatibility survives | vs2026 and ci presets configure and build Debug/Release with zero warnings; CTest passes in both, including project-code building a player and development module against the shared engine. Record renderer/toolchain and any environment limitation. | Final production sources and target graph |
| Rendering remains equivalent | Shapes, sprites, hello-gl and particles are pixel-identical to the qualified split baseline; boids/flocks start, retain appearance and exit zero. Record deterministic capture inputs, comparison method and who performed visual review. Restore any temporary instrumentation before final build claims. | Same driver, captures, final rendering code |
| Distribution reduces duplication | Before/after sizes of players, old per-game C API libraries, new game modules and shared engine, with configuration and exact binaries/revisions. Historical proposal estimates are not measurements. | Baseline preservation, final artifacts |
| Consumers and documentation can resume accurately | PixieLib/Inspector responses distinguish recommendations from executed acceptance. Record final full code hashes, package/binary provenance, LogicSandbox header/build/loading breaks without migrating it, documentation checks and unmet rows. | Final implementation and consumer feedback |

## Evidence and recovery

### Export-stage resumption, 2026-10-29

The owner resumed from clean main at
`836ba0b5873f1822efa0d914c19f96e789ab4a39`. Engine all-route VAW compatibility
returned bound with all seven routes checked. The existing implementation
writer acknowledged exclusive code/build ownership; the principal writer
retains documentation and read-only verification. The next work is separate
shipping output, manifest export, dependency/runtime closure and the four
barriers with sensitivity tests. This is a resumption record, not execution
evidence for those still-pending gates. Prior Lib/Inspector results keep their
recorded baseline and scope; no repeated qualification is inferred.

The first native Windows boids bundle was planned/copied/verified with eleven
binary entries (player, engine, GLFW and the explicitly listed release CRT
bundle), project document and start scene. The development flags were false
in its product/manifest. The player exited zero from a different working
directory and wrote a 1280x720 PNG; the principal coordinator inspected the
frame and observed the boids scene. This is a first positive working-tree
case, not the complete barrier/CRT/visual regression matrix. No golden image
approval or Windows-clean-machine qualification follows from it.

Read-only review found original-output junction handling, OS-library
classification and missing export-forwarder closure conditions. The writer
applied fixes and the independent reviewer rechecked them statically. The fresh
native Windows qualification then passed 23 recorded cases on the uncommitted
export-stage working tree. Boids and no-code sprites were exported from paths
containing spaces and Unicode, with unknown project data preserved byte for
byte. Both players exited zero from a different working directory, captured
PNGs and were observed loading pixie.dll, glfw3.dll, vcruntime140.dll and
msvcp140.dll from their export folders.

Independent broken copies rejected a development Engine/configuration, a player
with a game-module loader, unlisted files/directories, missing GLFW and CRT,
a missing export-forwarder dependency, invalid import basenames, manifest path
traversal, a nonempty destination and an original output junction before
canonicalization. Adding the forwarder provider and restoring clean boids and
sprites bundles passed verification. The first harness attempt stopped at an
LNK2001 in its forwarder fixture; correcting that fixture and using a fresh
scratch run produced the result above. It was not an export-product failure.

The sprites PNG hash matched the retained pre-migration capture
(`e6ea9c41fc8c99efadcacc291c3591f540f553a6c59bafff376e9d5077a4b2ec`).
This is one retained-baseline comparison, not a fresh baseline build, a full
render regression matrix or owner approval of a golden reference. OS-library
classification remains statically reviewed without a System32 mutation test.
Three local managed publish checks also passed: the default development build
was rejected before payload production, a fresh build with
`PixieDevelopmentTools=false` published locally, and `NoBuild` publication was
rejected. The implementation writer inspected the shipping assembly and found
ProjectExportJob, ExportPanel and the Development namespace absent; the
coordinator read the three recorded outcomes and the matching project guards.
This is local artifact qualification, not a NuGet upload or remote release.
Future MCP code/dependencies must independently satisfy these exclusions.
The real editor `--project` sprites / `--do export` route then invoked the
native tool, verified the output manifest/closure and exited zero after its
asynchronous export job. Its timed quit waited for completion (4.302 seconds
wall time with a two-second quit request), and it wrote a 1600x900 capture.
Static source inspection connects the File menu and command to the same
ExportPanel/ProjectExportJob; this is not a recorded interactive menu gesture.
The editor existing-output rejection was rerun without a build and its receipt
confirms exit 3, as expected. Final native regressions, optional headless Mesa
provider execution, remote CI and other-platform export remain pending here.
The [export guide](game-export.md) describes the current command and scope.

At the owner's quota checkpoint, stop without starting another long matrix.
Local code checkpoint `fbbcf230847b3d7fddc0799395dddaf3f6a3427e` preserves
this partial export implementation on main. The 23-case suite and three publish
checks preceded small final adjustments; they do not constitute a complete
rerun of that final commit. The final editor Release build reported zero
warnings, and the coordinator read the editor rejection receipt confirming
exit 3. The registered CTest export entry, current Debug/Release regression matrix, software-renderer
fixture branch and full visual/asset matrix remain pending. MCP and
deterministic capture are subsequent increments, not delivered by this export
checkpoint. Preserve scratch evidence and the earlier foundation checkpoint;
resume by checking branch/HEAD and the final local checkpoint record. No push,
publication or reference-image approval is included.

### Resume and LogicSandbox inventory, 2026-10-29

After renewed owner authorization, the coordinator verified clean main at
`f0f634b35486d0754bb27923b5a3038fb2de4e75` and VAW all-route compatibility
(seven checked, none unchecked). The implementation writer acknowledged sole
code/build ownership again; documentation remains with the coordinator.
Current-source export/native/editor regressions were resumed, not assumed
from the previous checkpoint's earlier executions. With native sources still
at `fbbcf230847b3d7fddc0799395dddaf3f6a3427e`, the vs2026 Debug build passed
and full CTest passed 18/18 in 238.00 seconds. Its registered game-export case
passed in 146.12 seconds and recorded all 23 export cases. The coordinator read
the complete CTest summary and receipt count. The editor Release rebuild
reported zero warnings; its three local publish checks passed again on this
revision. Release native build and full CTest also passed: 18/18 in 243.07
seconds, including registered game-export in 153.39 seconds. These vs2026
executions do not qualify the separate Ninja preset, Mesa provider or remote CI.

An independent read-only review of the final export commit found no new
actionable defect in its scoped CMake/CI/export-fixture inspection. It confirmed
limits: the optional graphics-provider path verifies the raw export but launches
a second explicit-manifest bundle; the fixture observes four app-local native
DLLs, without asserting the OpenGL provider path or llvmpipe renderer. PNG
signature/dimension checks are not visual comparisons. The editor CI job builds
only; the local publish suite is not automatically run by that job. Static
review is separate from the executed local matrix above.

Follow-up editor-only guards remove the Export property/menu and command case
when development tools are disabled. The strengthened publish harness passed
all three local cases after rebuilding; it now checks absence of the Export
getter and UTF-16 menu label as well as the adapter types. The coordinator read
the diff and passing harness log. This refinement is later than the native
baseline above and does not imply another native-source change or matrix run.
The editor-only fix is committed as
`e31ec6d2eda256f4fc280ea15ef8df19e1b96a5a`. A sensitivity receipt confirms
that the strengthened getter/menu check rejects the previously retained
shipping assembly; this is not a source mutation or a new claim about the older
test's narrower scope. The real editor sprites export was rerun and both editor
and native verifier exited zero. The coordinator read both receipts before
recording these outcomes. Deterministic capture implementation starts next;
no result or reference-image approval follows from that start.

Release particles and sparks players also launched with their sample project
folders and exited zero with 1280x720 captures. Their logs have no missing-font
warning. Both implementation writer and coordinator inspected the images:
particles shows its count/interaction HUD, and sparks shows the mass input and
instructions alongside the bodies and ramp scene. This closes the earlier
native player asset-root/HUD gap only; it does not qualify editor project-folder
opening, interaction gestures, a pixel comparison or an approved reference.

A read-only inventory of LogicSandbox at
`927ab988fc8a4b5133aa49d1174ceb7ca4f77abb` found seven old include names absent
from the current Engine header tree: Camera2D, GlLoader, Image, Input, Log,
PathRenderer and ShapeRenderer under `pixie/`. Their current routes are
`pixie/render/2d/`, `pixie/render/gl/`, `pixie/core/` and `pixie/platform/`;
the inspected `pixie/runtime/` includes remain present. Its CMake still uses
the supported `pixie_game` and game-library targets, but its comment names the
old LogicSandbox-capi artifact instead of the development module plus shared
pixie.dll. Its README records an older tested Engine baseline, `dc758e3`.

Panel.cpp opens `assets/inter-regular.ttf` directly, while its game declaration
has no ASSETS entry; that code-owned font needs explicit export declaration.
The gates.json input is compiled into a generated string and is not equivalent
to a runtime asset dependency. A future migration must also verify shared CRT,
module loading and the custom logic-shot host against the new DLL boundaries.
No LogicSandbox file, build or test was changed or executed by this inventory;
these are static migration requirements, not an executed compiler or runtime
failure list. Migrating that consumer remains outside this task.

### Deterministic capture increment, 2026-10-29

On the uncommitted proposal-014 working tree, a seven-case local harness passed
after correcting step-budget precedence over wall-time termination. Boids,
flocks, particles and sparks each repeated a 12-step 320x180 capture on the AMD
Radeon RX 7600 with identical output despite changing FPS cap from zero to 30
and adding a 0.001-second quit request. Logs checked the completed step count
and alpha one. A synthetic scene without an authored seed repeated seed 42
exactly, differed with seed 43 and differed after 13 steps. A one-pixel mutation
failed the strict comparison and wrote a diff; the clean comparison then passed.
Two full editor-window boids captures also matched at 12 steps despite the
short wall-time limit. The coordinator read all seven receipts and the harness.

These are same-renderer synthetic/initial integration results, not a full final
Debug/Release matrix, Mesa result or approved sample/tolerance policy. Earlier
native engine tests reported 35 passing cases including exact counts 0/1/17
and alpha one; the final integrated rerun remains required after source changes.
Candidate generation and snapshot tooling are described in the
[capture guide](deterministic-capture.md). No reference image is approved.
The capture executions used local Python 3.15/Pillow 13.0.0, independently
queried by the coordinator, not the bundled documentation-check interpreter.
Four 120-step 640x360 candidates were generated through an external local
manifest with `approved: false`; these sample/seed/size/tolerance inputs are
review proposals only and no sample reference files were changed.
The local editor publish harness also passed its three cases with the new
capture exclusions: DeterministicCapture, the QuitAfterSteps getter and its
UTF-16 command-line option are absent from the published assembly, in addition
to the export adapter/menu checks. The coordinator read the receipt and marker
checks. This does not yet qualify the frozen native matrix or future MCP code.

The frozen capture working tree then passed full vs2026 Debug CTest 19/19 in
312.17 seconds: deterministic capture took 34.66 seconds and export 172.27
seconds. The export receipt now records 25 cases, including shipping players
rejecting the development step-count option with exit 2; its scanner also
checks the option string is absent from the shipping Engine. The coordinator
read the final summary and the new exclusion receipts. Release build/full CTest
also passed: 19/19 in 276.71 seconds, capture 11.19 seconds and export 165.07
seconds, with 25 recorded export cases. Later snapshot-registration CMake and
editor-only refinements still need their scoped checks; these full runs do not
by themselves qualify every later source revision, Ninja or remote CI.
The coordinator also read the effective runtime audit: 70 active native
target/configuration properties checked, zero /MDd Debug or /MD Release
mismatches. That audit establishes build settings, not allocator coverage or a
new package/runtime distribution claim.

The capture implementation is preserved in local code commit
`1facc7564df9eeb631daea5503badad05fd58909`, after the editor-only export guard
fix `e31ec6d2eda256f4fc280ea15ef8df19e1b96a5a`. Documentation is committed
separately. This is a recoverable increment, not completion of TODO-051 or MCP.
A checker-only follow-up,
`e94cd1aac41b6884b41b1a577a67856e3f11a257`, excludes generated build-ci
Markdown. The first final documentation run had found 18 broken links in four
copied export-source READMEs, outside the versioned documentation scope.
After the correction, 165 documents passed with zero problems. An isolated
fixture confirms generated copies are skipped while an actual documentation
broken link is still rejected. The coordinator read the result and sensitivity
receipt; no retained build evidence was deleted to obtain the passing result.

Scoped follow-up fixtures passed after the snapshot helper/editor refinements.
Four synthetic registration cases cover an approved code-sample target,
unapproved candidates producing no tests, a missing code target failing closed
and a no-code scene using the generic player. Their synthetic approval flags
are test inputs, not approval of any repository image. The editor completed
0, 1 and 12 steps with a collapsed viewport and preserved the original layout
bytes. A final seven-case capture harness including editor repetition passed
again, retaining the same editor image hash. The coordinator read these receipts.
Release editor project-folder builds/opens for particles and sparks then exited
zero with their corresponding development modules, no missing-font warnings
and 1600x900 screenshots in Edit mode at zero steps. The coordinator read the
logs and inspected both images: HUD text and scene geometry are present.
This qualifies opening/asset resolution, not editing gestures or simulation.
The separate ci preset (Ninja Multi-Config, local Windows toolchain) then passed
Debug CTest 19/19 in 236.88 seconds, capture 31.06 seconds and export 128.38
seconds with the expanded 25-case harness. The coordinator read its final
summary. Release also passed 19/19 in 203.02 seconds, capture 11.56 seconds and
export 127.75 seconds, with 25 export receipts. These are local preset executions
using the installed GPU, not remote CI or the optional Mesa-provider branch.
The current isolated shared-boundary gate was also rebuilt and passed in Debug
(1/1, 3.02 seconds total) and Release (1/1, 2.99 seconds total), with deliberate
identity breaking disabled. The coordinator read both final CTest summaries.
This closes the historical missing current-Release gate run; the earlier
broken-identity sensitivity remains evidence at its recorded older baseline.

### Native dispatcher resumption, 2026-10-29

The owner explicitly resumed after the pause. The coordinator verified clean
main at `4bc18a76b5877ba79de64c66a39364dda04f14a9`, the preserved eight-file
013 scaffold manifest and all seven VAW routes. The implementation writer
acknowledged sole code/build ownership; the coordinator retains documentation
and read-only review. The scaffold was not buildable when paused and does not
establish API-18 or dispatcher delivery merely by being restored.

The first bounded increment is native World dispatch in memory, with borrowed
session lifetime, owner-thread rejection, response-buffer preflight, immediate
native error receipts and snapshot failure semantics qualified before IPC.
Runtime must not acquire a C API/editor dependency; owned sessions retain module
leases through destruction of native objects and descriptors. Save and indirect
save/export/build/import aliases remain unavailable over the channel. Inspector
handler/history qualification, once-per-frame host dispatch, local single-client
transport and actual MCP compatibility are later integration gates. None is
claimed by this resumption record. The completed capture checkpoint and all
unapproved candidate/reference boundaries remain intact.

The initial in-memory implementation subsequently built in Debug and its
targeted CTest passed 1/1 (2.43 seconds total), wrapping thirteen native fixtures.
The coordinator read the final test summary, dispatcher and fixtures. Coverage
includes owner-thread/buffer preflight, native named-field and scene-ID access,
actual step/time-scale receipts, text/point/record operations, borrowed teardown,
input/view commands, lifecycle/unsaved state and disabled save aliases.

Review strengthened restore handling before qualification: tokens are tied to
the session's World revision, while ordinary experiment edits remain possible.
Incoming references outside the snapshot are refused before replacement,
including handles of entities already destroyed during an experiment. Internal
and outgoing references use native restoration. Recreating entities may change
runtime handles; it does not change their scene IDs. Failure injection covers
partial native restore, cleanup restricted to the selected scene IDs, successful
recovery of prior values and explicitly failed rollback. This is bounded
recovery over the declared surface, not a transaction for arbitrary game code
or its side effects.

A greater-than-4-MiB post-write readback produces a small error receipt marked
possibly_applied; the fixture confirms the mutation ran once without retry.
Earlier iterations included a mistaken collider-default expectation and a
constructor-injection failure that exposed partial restore; the later targeted
pass does not erase those attempts. Release and broader integration/exclusion
qualification remain pending. There is still no listener, host drain or MCP
wrapper in this increment.

A later response-buffer preflight mutant was detected: short-buffer writes
caused three failed assertions. Restoring source bytes initially preserved an
older timestamp, so the subsequent incremental build reused the mutated C API
object. That intermediate full Debug run is not final-restoration evidence.
The writer identified the stale object and is forcing effective recompilation
before repeating qualification. A correct source hash alone does not establish
that the executed binary has been restored; retain the distinct mutation,
intermediate failure and final rebuild results.
The subsequent restoration build log explicitly compiles Capi.cpp. The targeted
module-lifetime and dispatcher tests then passed 2/2 in 2.13 seconds total;
the coordinator read both the compilation line and test summary. The preceding
full Debug run remains recorded as 19/20 with the known stale-mutant failure,
not relabeled as an all-pass run. The following full Release matrix passed
20/20 in 272.12 seconds (dispatcher 1.56, export 162.29, capture 10.35 seconds).
The coordinator read the final summary. The fresh complete Debug rerun then
passed 20/20 in 278.29 seconds (dispatcher 1.44, export 157.84, capture 26.72
seconds). The coordinator read that summary and the final qualification receipt.
Local code commit `f22942a8e206b820003f9a68256c6b5b1eace3f1` preserves the
bounded in-memory World dispatcher/API-18 increment. Transport, player-host
integration and MCP tool/client work are excluded from that qualified commit;
their uncommitted files do not establish listener or MCP delivery.

The future MCP tool's locked restore also passed. Its local publish refusal
returned exit 1 with zero files in the requested publish destination; the
coordinator read the receipt. This is tool-project isolation evidence, not a
functional MCP/client or final native/editor dependency-closure qualification.

### Initial Windows player-host and MCP integration, 2026-10-29

An initial Debug integration using a real local pipe and the official C# SDK
client passed twenty cases on the player host. The coordinator read the receipt
and test client. Checks include hello-first, single-client refusal, queue drain
while paused, completed three-step responses, capture alpha one, traversal and
reserved-name rejection, existing-file byte preservation, disabled save and
unavailable Inspector namespace. SDK discovery/pause/step and PNG image content
passed; invalid capture paths and overwrite attempts returned tool errors.
The capture root contains spaces and Unicode. This is SDK-client qualification,
not evidence that the tool is registered in or has been called by Codex.

Two earlier attempts at 96x64 failed explicitly because the requested
framebuffer was unavailable, before reaching hello. Hiding the window did not
establish support for that size. The passing fixture uses supported 640x360
native and 1280x720 wrapper captures; no arbitrary-size guarantee follows.

An independent static review found two further host cases: a capture while the
framebuffer is empty could leave the worker waiting for a skipped render, and
Stop/Start of one PlayerChannel could reuse a connection identifier with a null
dispatcher. Corrections now complete unavailable-framebuffer captures with an
error and reset/recheck dispatcher/client state. Dedicated regression fixtures
then passed in the targeted Debug native test (1/1, 2.62 seconds total), now
containing eighteen fixture groups: thirteen World groups, four transport
groups and the player lifecycle/empty-framebuffer group. A fixture-local MSVC
alias/default-argument compilation issue was corrected before that passing run;
it was not an executed transport failure. The direct empty-framebuffer fixture
also verifies no file is written and subsequent stats requests flow. It does
not exercise minimization through the actual UI. The independent reviewer
rechecked both corrections statically, without executing tests. The coordinator
read the native summary and fixture, then a fresh real-pipe/SDK-client receipt
with twenty passing cases on the corrected host.

The transport fixtures inspect the owner-SID protected ACL, reject another
server instance and a second client, defer effects until owner drain, reconnect
with a new client identity, and stop during idle/deferred I/O. This is scoped
local Windows evidence, not a cross-user adversarial process test or Unix
permission qualification. Export detection has been expanded for the new
transport classes/flags/marker; its new contamination cases remain unexecuted
at this point and do not inherit acceptance from the earlier export suite.
Editor-host Inspector/history routing and final artifact exclusions remain
separate pending integration gates.

An expanded SDK-client run subsequently passed twenty-six cases. Its receipt
records monotonic/distinct command frames, negotiated MCP protocol 2026-07-28,
and a client explicitly configured for legacy 2025-11-25 completing discovery,
pause and image-content calls. The coordinator read the receipt; these remain
local SDK-client results, not Codex registration or remote interoperability.
The full host Debug matrix and new transport-contamination export fixtures are
still in progress. Editor-host capture timing remains under design; a temporary
pause cannot be treated as a neutral observation because the native pause setter
drops accumulated fractional time. An editor capture implementation must avoid
that hidden clock change or explicitly refuse unsupported live captures until
a non-mutating rendering path is available.

The first full host Debug matrix passed 19/20 but failed export: after the
Windows entry point converted arguments to UTF-8, automatic project discovery
still interpreted argv[0] as an ANSI path. This broke the exported player's
Unicode-folder launch without an explicit project argument. The writer changed
that construction to a UTF-8 filesystem path and started a fresh full rerun.
The failure does not establish a manifest-verifier defect or acceptance of the
new contamination cases; keep the failed and restored results separate.
The corrected full host Debug rerun then passed 20/20 in 285.70 seconds,
including capture 29.95, dispatcher/transport 1.10 and export 161.33 seconds.
The coordinator read the final summary and all thirty-three export receipts.
New negative cases reject an actual transport-only DLL without C API exports,
a channel-marker-only DLL and all three channel options in both code and
no-code shipping players. Those executed cases qualify the expanded Windows
detector at this revision; they do not cover unbuilt editor-host integration.
Release host qualification then passed 20/20 in 269.70 seconds: capture 10.04,
dispatcher/transport 1.51 and export 161.59 seconds. The SDK-client integration
also passed all twenty-six cases against Release, including current/legacy
protocol discovery, pause and image content. The coordinator read the final
Release summary and receipt. Ordinary and NoBuild tool publication each failed
with zero destination files; locked package restore passed.

The preserved editor pipe scaffold creates its native handle with explicit remote
client rejection, first-instance/overlapped flags and an owner-SID protected
ACL before wrapping it for managed asynchronous I/O. This addresses the gap
between current-user identity restriction and local-only transport by source
inspection. It is not yet a listening editor-host or executed cross-user/network
qualification. Do not infer those claims from the player-host tests. That
scaffold, along with its Inspector surface/native wrappers/history preparation,
was backed up and removed/restored out of the active source before checkpoint;
it is not an active feature of the committed player host.

The local host/tool milestone is committed as
`d319ce312b5ae818ab753f5fac40fc67e0b4a458`, following in-memory dispatcher
commit `f22942a8e206b820003f9a68256c6b5b1eace3f1`. At the owner's checkpoint
request, all new implementation stopped. Six unfinished editor files are
preserved separately with hashes and a patch; the complete earlier working
snapshot is retained as well. The editor scaffold is neither listening nor
functionally qualified. Its one-shot non-mutating alpha override is still a
plan. The current editor was rebuilt against the committed source, and future
work must recheck the checkpoint/backup manifests rather than assuming the WIP
is active. No push, remote publication, reference approval, real Codex tool
registration, Linux/Unix transport or new Ninja host matrix is established by
this milestone. TODO-051 remains unfinished.

### Editor-channel round resumption, 2026-10-29

After renewed owner authorization, the coordinator verified clean main at
`e9ad8be4e6cb0d8043d520c164988a4b247b479e` and all seven VAW routes. The
same single code/build writer retains implementation ownership; documentation
and read-only review remain with the principal coordinator. The six-file editor
backup is a dormant scaffold, not accepted editor integration. Restoration must
check its manifest/base and preserve the later documentation commit.

The next gate is a one-shot alpha-one render that preserves fractional elapsed
time, paused state and time scale, followed by owner-thread editor handlers,
Inspector readback/history and coherent UI capture. The initial fixture must
prove two half-step advances still produce one step across the capture and
that the next normal render uses its ordinary alpha. No editor listener or
capture is qualified merely by resuming this round. Save/export/build/import
through the channel remain disabled, no references are approved and no push or
publication is authorized.

The native one-shot alpha override was subsequently implemented through the
existing development dispatcher and a RenderToTarget(double) overload retaining
the original signature. The targeted Debug test passed 1/1 in 3.11 seconds
total, containing nineteen fixture groups. The coordinator read the diff and
fixture: at time scale 0.5, a half-step advance, prepared alpha-one render,
ordinary render and second half-step produce exactly one completed step without
changing paused state, time scale or the preserved fraction. The owned C API
render bridge is exercised separately in that fixture. No additional C export
or temporary pause is introduced.

This is initial native Debug evidence, not completed editor integration or a
new full matrix. The one-shot flag is consumed at the render attempt; a failed
target allocation must be treated as capture failure rather than proof that an
image was rendered. The editor's planned central Advance/drain before panels
and deferred coherent UI capture still need executable acceptance.

The alpha fixture then passed targeted Release CTest 1/1 in 2.28 seconds total.
An initial real editor-host SDK-client run passed thirty cases. The coordinator
read the receipt and test client and inspected its 1600x900 full-window capture:
the Inspector shows selected entities and authored values while the session
reports three completed steps and paused state. The tests compare held degrees
against live/native radians, actual native edits and undo/redo values, one
history gesture for multi-selection writes, mixed per-target values, refreshed
Inspector values after direct World edits without history, stale-generation
refusal, scene-ID selection/rebind after restore, and disabled filesystem aliases.
Deferred PNG capture, overwrite/path rejection and no pause/scale/step change
across capture passed in that fixture.

This is initial integration evidence on a disposable Unicode-named no-code
project, not complete consumer acceptance, widget interaction or an approved
reference image. Custom rows, current-command failure/pending/read-only cases,
broader lifecycle and final publish exclusions remain under qualification.
The Inspector collaborator has been asked for a separate read-only contract
review; no Inspector repository/package/API change is implied.

The writer then added a direct owner-thread native/Inspector fixture with
eleven passing cases, using the real Set/recorder path. It covers read-only
refusal despite handler true, manual pending state without a native write,
new command failure taking precedence over existing pending state, native
NOT_FOUND captured before readback, partial first-target effects/history and
preservation of an immutable failure receipt. This deliberately bypasses the
channel's generation preflight to inject a stale bound target; it is not an
executed MCP race. The initial run printed passing assertions but exited with
a graphics-context cleanup fault. The corrected cleanup run exited zero;
retain that distinction instead of treating the first assertion output as
successful process completion.

The Inspector collaborator's independent static review found four P2 cases.
Pending-state precedence and native error propagation were already corrected
in the later source and fixture. The other findings concerned a recorder staging
requested After values before native setter success and live getters generating
new failures after diagnostic state was copied. The writer subsequently changed
the recorder to observe actual storage after the attempt and added command-local
live-read failure capture, separate from cached/write diagnostics. The coordinator
read those changes; the native rejecting-setter and live-read failure fixtures
are still being extended. Static correction alone does not close acceptance.

The real editor SDK-client suite expanded to thirty-six passing cases, including
custom text and point-list handlers, native-storage domain labeling, registered
record Inspector readback and a record gesture's failure receipt. Its first
custom run assumed an empty text default instead of the native "Text" value;
the corrected fixture captures the prior value and checks exact undo restoration.
The native Debug full matrix passed 20/20 in 329.09 seconds (capture 32.73,
dispatcher 1.87 and export 186.28 seconds) before the new rejecting-setter module
fixture was configured. The coordinator read both the thirty-six-case receipt
and native summary. The native Release matrix also passed 20/20 in 304.93
seconds (capture 10.11, dispatcher 1.64 and export 184.21 seconds). These runs
cover the native alpha/dispatcher source; later managed fault fixtures retain
their own executed scope.

The actual editor publish harness passed its three cases after channel wiring:
development publish rejected, fresh shipping publish accepted with channel/pipe,
Inspector adapter, development dispatch/friend markers, SDK artifacts and new
flags excluded, and NoBuild publish rejected. The coordinator read the receipt.
The old recorder implementation was then used as a sensitivity mutant; the
rejecting-setter ghost-history check failed with exit 1, as intended. Source was
restored with a verified hash and forced rebuild before the passing restored
fixture. This is separate from the earlier graphics-cleanup failure and does
not approve arbitrary partial-write rollback.

The independent Inspector reviewer rechecked all four original P2 fixes and
found them corrected by source inspection, without edits/builds/tests or a
package change. Final direct fault runs and expanded stable-reference MCP
qualification are being closed before final editor acceptance.

The final editor code milestone is local commit
`e64dbb765430093e88f1d66b09e2d8f9340efcd2`. For both Debug and Release,
the final receipts contain thirty-nine passing editor SDK-client cases,
twenty-one passing direct fault/real-editor-pipe cases (fifteen fault and six
transport cases), and twenty-six passing player cases. The coordinator read
all six per-configuration integration receipts. The expanded editor cases
include scene-ID entity references, rejection of runtime-index labels and
incompatible reference component types. Existing Set/SetShape handlers call a
development-only attempt callback after parsing and before writing, so a
conversion refusal is not misreported as an accepted pending edit.

Final ordinary-editor regressions also passed 0/1/12-step collapsed-viewport
runs in both configurations with the user's layout bytes preserved. The final
editor publish suite passed its three cases. Both ordinary and NoBuild MCP tool
publication were rejected with zero payload files. The coordinator read these
receipts separately from native full-matrix results; no extra broad matrix was
inferred from a targeted managed test. Source and documentation commits are
separate and no remote action occurred.

This completes the bounded local Windows editor/Inspector channel milestone,
not TODO-051. The full project-wide qualification/reconciliation, any new Ninja
host matrix, actual remote-release producers and owner-approved snapshot
selection/tolerances/references remain separate gates. No real Codex tool
registration, Linux/Unix transport, widget interaction acceptance or automatic
reference approval is established. The stable local checkpoint must record the
final code/docs hashes and these limits before later work starts.

### MCP and development-channel closure, 2026-10-30

At the owner's request, a Claude session closed the remaining local MCP and
development-channel gates on the local branch `mcp-dev-channel-closure`, from
`c2248eb6efdc8b7a804f1c4a4462ec0db94cce18`, with VAW `bound_scoped` for the
build, capi, documentation, editor and engine routes (profile `pixieengine`,
revision 1). Physics and the shared-engine/export tasks other than channel
exclusion were out of scope. The code commit is
`066f097d705d6c11150b844338bf0677de1b3705`; nothing was pushed.

A review of the channel code found one defect: `pixie-mcp` gave its host 15
seconds to open the pipe, while the editor opens its channel only after
building a project's code, whose first build compiles the engine. The server
now answers its client at once, opens the host in the background and makes
the first call wait up to `--startup-timeout` (600 seconds by default). The
host joins a Windows job of the server, so a killed server takes it along.
The owner answered question 2 of proposal 013: `editor.do save` reuses the
save handler in development builds, `-p:PixieChannelSave=false` removes it,
and `world.hello` reports `capabilities.save`.

`tests/mcp/qualify_mcp.py` replaces the hand-run suites. Final runs on Windows
x64, AMD Radeon RX 7600, OpenGL 4.5 core:

| Preset and configuration | Cases | Suites |
|---|---|---|
| vs2026 Debug and Release, each | 135 | player 26, editor with save 40, editor without save 40, pipe and faults 21, killed server 1, cold code project 7 |
| ci (Ninja Multi-Config) Debug and Release, each | 128 | the same without the cold code project |

The cold code project compiled the engine's 128 translation units in its own
build folder. Its tools were listed after 0.39 seconds (Debug) and 0.40
seconds (Release); the channel opened after 47.70 and 68.39 seconds, beyond the
former 15-second limit. A host that exits first, and a three-second deadline,
were tool errors with `possibly_applied: false`. Sensitivity: with the job
removed from a copy of `HostJob.cs`, the killed-server case failed with the
host still running; the restored file was byte-identical, rebuilt and passed.
The save cases ran against both editor builds: the scene file changed and
`unsaved` cleared with save, and stayed byte-identical with refusal without it.

The first Ninja Debug run failed because the editor loads the engine from the
nearest `bin/Release` before `bin/Debug`, and `build-ci/bin/Release` still held
an older API 17 engine. Earlier Debug editor evidence therefore used the
Release engine whenever both existed. The script now builds each editor beside
a `bin/` holding only the tested configuration's engine; three repeats and the
final runs passed.

The native matrices passed 20/20 in each configuration: vs2026 Debug 262.69
seconds, Release 236.29; ci Debug 219.85, Release 188.08. They ran with the
final native sources and export checks, which now also assert that the shipping
`pixie.dll` of the boids and sprites exports lacks the channel, dispatcher and
`world.hello` markers and that no MCP, probe or module file is exported. They
preceded only the test clients' stderr relay and the script's engine staging.
The editor publish suite passed its three cases, now also rejecting any
`PixieMcp` or `ModelContextProtocol` file in the published editor.

A real client drove the editor: Claude Code 2.1.310 in print mode, with a
temporary MCP configuration and its Haiku model, called hello, entities,
selection, `inspector.set`, `world.get`, undo and capture, and described the
captured image correctly. Claude Code exposes the dotted tool names as
`mcp__<server>__world_hello`. Codex was not installed on this machine, so its
registration is documented but unexecuted.

Remaining limits: no Codex execution, Linux/Unix transport, Mesa or remote CI
run of the MCP suites, concurrent physical UI gestures or golden reference
approval. PIXIE-TASK-209's project-wide matrix and TODO-051 remain open.

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
