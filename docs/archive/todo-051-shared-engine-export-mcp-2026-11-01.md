# Shared Engine, Export and Local MCP Completed: PIXIE-TODO-051

**Document ID:** ARCHIVE-TODO-051-SHARED-ENGINE-EXPORT-MCP-20261101

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** Windows shared-engine distribution, manifest export, local MCP and the approved lighting snapshot

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-090

**Type:** completion

**Origin:** PIXIE-TODO-051 (PIXIE-TASK-201 through PIXIE-TASK-212)

**Author:** codex

**Reference date:** 2026-11-01

**Commits:** closure fixture/reference/revision `92c6a642eedf7fedd1f19c7752f2f4a170be36d0`; documentation with this record (parent `92c6a642eedf7fedd1f19c7752f2f4a170be36d0`); earlier implementation hashes are retained below

**Reference commit:** `a6fc4180435f725b81ef4d30c6bccfef219cc224`

**Current state:** [qualification](../shared-engine-export-validation.md), [game export](../game-export.md), [deterministic capture](../deterministic-capture.md), [editor](../../editor/README.md)

## Accepted work snapshot

### `PIXIE-TODO-051` — Shared engine, game export and local MCP development channel

**Status:** done
**Source:** owner promotion of [PIXIE-PROPOSAL-010](../proposals/shared-engine-and-export.md) on 2026-10-28, explicitly extended to [PIXIE-PROPOSAL-013](../proposals/dev-channel.md) and [PIXIE-PROPOSAL-014](../proposals/deterministic-capture.md) as the development-channel and deterministic-capture scope. The owner's extension supersedes the supplied implementation prompt's exclusion of that channel.
**Accepted scope:** one shared native engine containing Core/Platform/Render/App, Runtime and C API; directly linked game players and development-only game modules; shared MSVC runtime across all native targets with app-local runtime deployment; one native manifest-allowlisted export tool reused by the editor; a separate local MCP development channel for Engine/Inspector operations; repeatable seeded, sized, fixed-step captures and approved snapshot scenarios. Nothing related to development operations enters game exports, publish or remote-release artifacts. The channel uses separate world.* and inspector.*/editor.* paths, descriptor-based discovery, session stepping, input/view operations and C API-backed snapshot/restore. Inspector writes reuse existing --set/--do handlers and EditRecorder/history, never a parallel writer. The player serves only world.*. Capture returns image content through MCP and writes only beneath the folder fixed when the channel opens. Entities use scene IDs and components/fields use names, never field indices. One client is served; queued commands run once per frame between Advance and Render, including while paused, and errors carry PixieResult plus pixie_last_error text. Channel save follows the owner's answer to question 2 of proposal 013 (2026-10-30): the editor's `editor.do save` reuses the existing save handler and is present in development builds, and the build flag `-p:PixieChannelSave=false` leaves it out; world.* and the player never save. Existing command-line behavior remains compatible.
**Dependencies and gates:** first prove module-defined EnTT components can be added, iterated and removed across the DLL boundary, and registered through ComponentTypes/read through the C API. Stop on type-ID disagreement and report mechanisms/costs instead of bypassing the gate. Only after it passes migrate production targets to /MD (/MDd for Debug), including third parties. Preserve the four-library dependency directions from the split baseline. Keep existing C API functions and semantics; additions raise the API version, and removals or semantic changes require a new owner decision. Obtain PixieLib shared-runtime evidence and agree the Inspector channel contract before dependent changes. Native engine code never depends on Inspector or the editor.
**Completion criteria:** all child tasks finish with the [qualification matrix](../shared-engine-export-validation.md), including project-code, both native presets/configurations, visual comparison, editor/module lifecycle, isolated exports, dependency closure, size measurements, and independently broken copies of all four development-artifact barriers. Qualify each accepted channel command group in memory and one real local IPC session, Inspector write/read/undo and refusal behavior, deterministic capture inputs and snapshot mismatch detection. Reference image updates need owner approval and a stated reason. The local MCP channel works through the agreed Engine/Inspector paths and is absent from publish/remote-release and exported runtime dependency closures. Record remaining LogicSandbox migration breaks without migrating it. No installer, additional export format, push or publication is authorized by this entry. PIXIE-TODO-043 was completed independently on 2026-10-31; this work did not complete physics or its owner acceptance.
**Breakdown:** [TASKLIST.md](#task-snapshot)

## Closure and delivered boundary

The owner authorized closing TODO-051 after selecting the existing lighting room
and explicitly approving its presented reference image and tolerance on 2026-10-31.
The accepted local Windows delivery is complete: one native shared engine, directly
linked players, development game modules, manifest-only shipping export reused by
the editor, local player/editor MCP, deterministic capture and the initial approved
lighting regression reference. Physics and lighting remain independently archived
deliveries; this record does not take ownership of their implementation or acceptance.

The closure increment adds qualification fixtures/reference data and documentation,
not a new runtime capability. Per the product version policy it raises REVISION from
0.2.0.0 to **0.2.0.1**, public version 0.2.0; the C API remains **19**. This version
does not authorize a tag, upload or release. Code and documentation are separate
local commits. No push or external publication was performed for this closure.

## Task snapshot

**Parent:** PIXIE-TODO-051

- `PIXIE-TASK-201` — **Status:** done — Matching-toolchain cross-DLL World/component/descriptor gate passed Debug/Release, with identity-mismatch sensitivity and clean restoration before migration.
- `PIXIE-TASK-202` — **Status:** done — Shared /MDd Debug and /MD Release apply throughout the active native graph. The final generated-solution audit checked 100 target/configuration entries with zero mismatches; the separate PixieLib /MDd prerequisite and isolated app-local CRT closure remain distinct evidence.
- `PIXIE-TASK-203` — **Status:** done — Core/Platform/Render/App, Runtime and C API are implemented once in pixie.dll with boundary export macros and one-way dependencies. Final target graph/builds, native regressions and artifact sizes qualify this matching-toolchain Windows distribution.
- `PIXIE-TASK-204` — **Status:** done — Players link game code directly; development modules retain per-session leases. Invalid images/null factories, retained sessions after clearing default module selection, external project compilation/loading and editor code-project operation are covered. Arbitrary hot reload and cross-toolchain modules are not promised.
- `PIXIE-TASK-205` — **Status:** done — Native exporter and editor share one implementation; isolated boids/sprites exports, asset closure, output preservation and verifier cases passed. The selected lighting code project was also exported, verified and run from its output directory.
- `PIXIE-TASK-206` — **Status:** done — All four shipping barriers have retained negative/restoration evidence. Fresh managed publish checks refuse development and NoBuild publication and inspect the clean shipping assembly/payload. MCP server/client projects refuse publication. Producer inventory found no remote-release artifact producer or existing release; no nonexistent remote payload is claimed tested.
- `PIXIE-TASK-207` — **Status:** done — Native world.* and editor Inspector/history paths retain descriptor/state ownership, scene identities, generation checks, owner-thread execution, error/readback distinctions and existing handlers. Native dispatcher, real pipe and official MCP client evidence is recorded in the qualification guide.
- `PIXIE-TASK-208` — **Status:** done — Local Windows MCP player/editor integration, save-enabled/disabled builds, cold code-project startup, killed-server containment and actual Claude Code client use are qualified. Current lighting-room checks additionally prove visible scene effect, native readback and undo/redo through the real editor.
- `PIXIE-TASK-209` — **Status:** done — Reconciled the staged full vs2026/ci Debug/Release matrices, baseline remote Mesa CI, final closure builds/targeted regressions, module/editor launches with fonts, exports, barrier sensitivities and sizes. This is explicitly staged evidence, not a claim of one new universal full run.
- `PIXIE-TASK-210` — **Status:** done — Current build/export/capture/dependency documentation and final provenance are reconciled. The unchanged LogicSandbox consumer inventory is preserved without migration. This record retains accepted scope, task outcomes, source identities and remaining limits.
- `PIXIE-TASK-211` — **Status:** done — Fixed total-step termination, seed precedence, framebuffer size, alpha-one rendering and existing CLI compatibility passed native/editor fixtures in both presets/configurations, with development-artifact exclusions.
- `PIXIE-TASK-212` — **Status:** done — The owner-selected light-lab/rooms-running image and tolerance are approved and registered. Same-renderer repetition, cross-renderer comparison, four targeted CTest preset/configuration runs, direct llvmpipe execution and wrong-reference rejection passed. Other candidate images are not approved.

## Evidence reconciliation

| Requirement | Mechanism and executed evidence | Qualification boundary |
|---|---|---|
| Component identity and shared ABI | Initial gate `d4be5a4a244712b2031a0762169b27136d6d09fe`; production foundation `70e3524455386e648ff8bb1ec62cc1c0e1b89014` and PIC correction `9abd26f540d1db4ffcf72ee831bc25ad7a83a89f`; module-lifetime/project-code fixtures | Matching Windows toolchain/configuration, not cross-compiler ABI or general hot reload |
| Runtime/vendor consistency | Separate PixieLib /MDd result at `21634516bf00014102b056ced02a0f44ad36d614`; active Engine 100-entry runtime-property audit, final builds and PE dependency checks | Standalone Lib acceptance is not substituted for the Engine vendor build |
| Native compatibility | Lighting consolidation records 27/27 full CTest suites in each vs2026/ci Debug/Release configuration at `c4f9a58dc1cd397ea64b73fd8276f45748b2e654`; later enum-refusal fix `c37f177` is covered by successful remote CI of main `a6fc4180435f725b81ef4d30c6bccfef219cc224` | Full historical matrices and final targeted reruns are separate; adding the approved snapshot registers a 28th suite |
| Final closure source | `92c6a642eedf7fedd1f19c7752f2f4a170be36d0`: fixture/reference/version-only change; final four native builds and targeted engine/runtime/lighting/module/snapshot runs | No runtime behavior rewrite; exact fresh outcomes are in the final qualification appendix |
| Real editor/game consumption | External CodeTest descriptor edit/history/lifecycle evidence; boids, particles and sparks module opens with isolated asset roots and inspected 1600x900 captures; real light-lab code project opened/built via MCP | Game-module launches are distinguished from the independently tested authored code-project workflow |
| Export portability and barriers | Retained 33-case boids/sprites receipts, observed app-local DLL loading, explicit missing-dependency/development/loader/unlisted-content mutations and restorations; current light-lab shipping manifest/isolated launch | Windows PE64 only; asset discovery follows descriptors plus explicit code-owned ASSETS |
| Publish exclusion | Fresh three-case editor publish harness; native shipping marker/CLI exclusions; MCP bridge and qualification-client RefusePublish targets | Local payload inspection; no package upload or new release pipeline |
| Local development channel | `e64dbb765430093e88f1d66b09e2d8f9340efcd2`, closure `066f097d705d6c11150b844338bf0677de1b3705`; later physics/lighting MCP qualification records 178 cases per Debug/Release with --skip-cold | Windows named pipes; native state/readback/history/error semantics, not arbitrary widget acceptance |
| Concrete lighting workflow | 23 final official-client receipt records; light intensity changes native storage and scene pixels, undo/redo restores it, Play/Stop preserves authoring; disabled-light fixture fails the visual-effect assertion | Default 1600x900 editor layout, bounded scene rectangle; an Inspector text change alone cannot pass |
| Approved visual regression | `samples/light-lab/snapshots/rooms-running.png`, SHA-256 `d64e5bfb536dac2ab821e9f4c31772352c7255f3b39caef3edb4277e9ba1e39d`; owner approval, exact same-renderer repetition and isolated wrong-reference failure | 120 steps, authored seed 1, 960x540, llvmpipe baseline; max delta 1 and differing-pixel share 0.15 |
| Rendering preservation | Lighting L5 records 11 deterministic unlit sample captures byte-identical to the L0 baseline; initial split/shared-engine captures and current module captures are retained | hello-gl uses random_device and volatile rate text, so preexisting nondeterminism prevents a pixel-identity claim; its primitive/render path is reviewed instead |
| Consumers | Public Inspector 0.3.0 adoption/real Engine consumer record; separate Lib/Inspector collaborator evidence retains its own scope; unchanged LogicSandbox checkout `927ab988fc8a4b5133aa49d1174ceb7ca4f77abb` has a static migration inventory | No LogicSandbox file/build/test was changed or migration claimed |

The baseline remote [CI run](https://github.com/Sakamoto0110/PixieEngine/actions/runs/41261714340)
passed Documentation, Engine Debug/Release with pinned Mesa 26.2.4 llvmpipe, and Editor.
It belongs to baseline `a6fc418`, not the unpushed closure commit or its approved snapshot.
There is no assertion of remote CI for this local documentation/reference closure.

Raw source/binary hashes, full logs, captures, retained export receipts and size JSONs
are kept in the machine-local validation directories for the lighting scenario and
TODO-051 closure. They are local evidence, not a clean clone's shared proof. Persistent
fixtures, approved image/manifest, commands and this scoped record are versioned.

After closure code `92c6a64`, all four native builds passed without compiler warnings.
Each preset/configuration passed its five targeted engine/runtime/lighting/module/
snapshot suites (vs Debug/Release 24.54/8.64 s; ci Debug/Release 24.74/8.49 s).
The final ci Release game-export invocation passed all 33 expected outcomes in
144.66 s, with source/artifact markers, actual isolated loading, four barrier
sensitivities and clean restoration. Fresh editor publish again passed 3/3; server
and qualification-client publication were refused with zero payload files. Editor
Release built with zero warnings/errors. These are new closure runs; the older
full matrices, concrete lighting workflow and baseline remote CI retain their own
code/configuration provenance. No universal fresh 28/28 or unpushed remote run is claimed.

## Distribution inventory and sizes

The native exporter is the sole game-export implementation and its verifier enforces
manifest entries, basename/path/reparse guards, nondevelopment configuration and native
dependency closure. The app-local Release CRT bundle includes `vcruntime140.dll`,
`vcruntime140_1.dll`, `msvcp140.dll`, `msvcp140_1.dll`, `msvcp140_2.dll`,
`msvcp140_atomic_wait.dll`, `msvcp140_codecvt_ids.dll` and `concrt140.dll`, alongside
`pixie.dll` and `glfw3.dll`. OS DLL policy remains explicit; machine-installed third-party
DLLs are not accepted merely because they appear in System32.

The producer inventory contains native game export and .NET editor local publish.
The only GitHub workflow is CI: it does not upload a runtime release payload; there
are no GitHub releases and no remote-release packaging workflow/target to execute.
Therefore no remote runtime artifact exists to inspect or claim qualified. This is
an inventory outcome, not a substitution of game-export proof for an existing remote
release or permission to add one. Any future producer must preserve these exclusions.

The retained pre-DLL inventory at split baseline
`f634eeeef474eac34b780fc2e43f11c239e2cd6d` measures four players at 8,943,104 bytes
and their four per-game C API DLLs at 8,405,504 bytes, total **17,348,608** bytes.
The closure inventory selects only actual current products: four players **1,546,240**,
four development modules **1,373,696**, and one engine **2,535,424** bytes, total
**5,455,360** bytes before shared GLFW, CRT and assets. Old ignored per-game capi DLLs
left in bin/ are not counted as current products or copied by export.
This is a historical retained-file comparison, not a controlled attribution of every
byte to the split: current binaries also contain later physics and lighting. Individual
SHA-256 values and sizes are retained; no estimate is presented as measurement.

## Preserved consumer migration inventory

LogicSandbox is still at `927ab988fc8a4b5133aa49d1174ceb7ca4f77abb`, matching the
earlier read-only inventory. Seven old includes (Camera2D, GlLoader, Image, Input,
Log, PathRenderer, ShapeRenderer under pixie/) require the current core/platform/render
routes. Its CMake comment still names the old per-game capi artifact; its code-owned
`assets/inter-regular.ttf` needs explicit ASSETS declaration. Its custom logic-shot
host, shared runtime and module loading need a separate migration validation. This
task records those static breaks; it neither migrates the consumer nor invents executed
compiler failures. Scene/save/unknown-data preservation remains a migration requirement.

## Residual limits

- Completion is the accepted local Windows PE64/matching-toolchain delivery. Linux/ELF,
  Unix transport, remote MCP, arbitrary DLL hot reload and cross-toolchain ABI are not
  qualified. No installer, additional export format or release pipeline was introduced.
- Actual Claude Code MCP use was recorded; Codex client registration/execution is not
  established by the standalone official MCP SDK client or the test fixtures.
- The initial approved visual suite is light-lab only. The separate light-machine
  experiment reached larger GPU/Mesa differences and has no approved golden. Other
  references require their own owner decision/reason. Shader-driver equality is bounded
  by the chosen scenario tolerance, not promised for arbitrary scenes.
- hello-gl's existing random-device initialization/rate display is not repeatable.
  No pixel-identical hello-gl or universal visual/layout guarantee is claimed.
- The editor scene-effect fixture requires its fresh default layout and compares a
  bounded viewport region; programmatic operation is not independent physical-pointer
  usability acceptance. Its functional undo bound is separate from the golden tolerance.
- Physics allocation observations changed scope after the DLL move: executable-local
  new/new[] hooks do not observe Engine/backend allocations. Physics acceptance belongs
  to its own completion record and is not broadened here.
- Future publish/release producers or new code-owned assets must be qualified; this
  record cannot certify outputs or dependencies that do not exist yet.
