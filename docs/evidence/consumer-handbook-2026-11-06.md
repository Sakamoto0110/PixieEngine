# Consumer handbook: local validation record

**Document ID:** GLOBAL-CONSUMER-HANDBOOK-EVIDENCE-2026-11-06
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** source review, public SDK example checks and Editor walkthrough
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

This record supports the [consumer handbook](../handbook/README.md). It records
a bounded local walkthrough, not a reopened 021/022 gate, clean-machine
qualification, renderer validation or release approval. No publishing, commits,
SDK modification or production Engine/Editor implementation changes were part
of this task. D09 remains frozen. Earlier documentation/history changes in both
working trees were preserved.

## Baseline and artifact locations

Artifact locations use placeholders for roots on the existing Windows account. Source was readable;
this was not an isolation test. Paths are provenance, not portable consumer setup.

| Input | Observed value |
|---|---|
| Engine HEAD | `1f44ae74008f1c2df4ff9b86ee8bb44f8ce8e072` |
| Editor HEAD | `c7cc2bfad332d70674cceb3e4474539ffccf528e` |
| SDK root | `<local-app-data>/Pixie/SDKs/0.3.0.0-99c9f678` |
| SDK version/class | `0.3.0.0`, public, authoring Release for Editor; paired shipping Release for export |
| SDK catalog SHA256 | `99c9f6789a1fd79f018f15e53f53c897619fde289454e938629d83bc8b85089a` |
| SDK ID | `ebbab809d3abb339e8753986dcc099a85323b78c2bde4db9d77ecc4301920938` |
| Authoring build ID | `8778598c387956bfc46fc8dacf703383a88bbc68f7ebcce6e308bef740583fc7` |
| Shipping build ID | `39e5edb66f0b0c57aebecbe1aebc1d8c80177da6270ba7c26a604d9b6ed1a166` |
| ABI/toolchain | C API 28, module identity 1, MSVC `19.51.36260.0`, Windows SDK `10.0.26100.0`, C++20, x64 |
| Editor executable | `<workspace>/PixieEditor/src/bin/Release/net10.0/PixieEditor.exe` |
| Editor executable SHA256 | `d8a68d84aa87bee10b0d7f63c5e4769926d25970f98dfda084ccf7ed42fa7f97` |
| Practice project | `<workspace>/PixieEngine_ConsumerTests/HandbookPractice` |
| Export | `<workspace>/PixieEngine_ConsumerTests/HandbookPractice-export-r1` |
| Exported executable SHA256 | `f96c3910eae9314620e2a4ae068ac5a55f2797c871c54b7685815694e5964733` |
| Exported pixie.dll SHA256 | `e42b94d3c4074704bebecbb3b0f943fa78c23ef565f1383e43edcc31b3d9e3d9` (matches selected shipping profile) |

The original dirty-state/hash ledger and local headless harness/logs are under
Engine `.local/consumer-handbook-20261106/`. They are local supporting artifacts,
not required inputs to the public handbook. The practice game was newly created;
existing consumer games were not edited.

## Source review

Reviewed the public runtime lifecycle, Entity/World/Behaviour, reflection,
InputControl, rendering/ScreenCanvas/camera, scene persistence, physics events,
particles and lighting surfaces needed by a first consumer, plus the Editor's
project/code/export routes and installed SDK catalog/build helpers. Installed
runtime headers were compared with the corresponding source headers. This was
targeted API and integration review, not a claim to have audited every source
line or every feature.

The SDK's root `features.pairedExport` and exporter descriptor are authoritative
for the current Editor route. An older profile-level pairedExportAvailable flag
does not by itself mean the root-declared paired exporter is absent. Instructions
use the selected SDK and include the current exporter `--sdk` argument.

## Executed through Computer Use

The Windows Computer Use plugin controlled the actual Editor with screenshots,
clicks and keyboard input. Initial capture/input interruptions before the user
reenabled the capability were not counted as successful tests.

| Action | Observed result |
|---|---|
| Choose SDK / Use SDK | Installed public SDK root accepted; authoring Release summary visible |
| New project | HandbookPractice created from the start screen; starter circle opened |
| Inspector edit | position x changed from 0 to 100; visible displacement and unsaved marker |
| Undo / Redo | Ctrl+Z restored 0; Ctrl+Y restored 100 |
| Shape / Save | Kind changed to Triangle; Ctrl+S cleared the unsaved marker |
| Code > Add code | Editor restarted; generated game module appeared in Session |
| Add Spinner | Reflected speed appeared; component attached to selected entity |
| Play / Pause / Step | Rotation and clock advanced; Pause stopped; Step advanced one tick |
| Stop | Rotation returned to 0 and authored x stayed 100 |
| Create PlayerController | UI generated reusable files/registry and attached the type after rebuilding |
| Update source / Build and reopen | Handbook example copied to user-owned files through file tools; UI rebuild exposed speed=180 while preserving unsaved content/selection |
| Attach movement dependencies | Velocity2D and generic KeyboardControl attached through Inspector |
| Save / New or open / recent project | Saved triangle and components reopened; x=100, PlayerController speed=180 observed |
| File > Export game | UI export created the independent game executable, scene/manifest and native dependencies in a fresh folder |
| Launch exported player | Launched through Computer Use; separate captures showed the triangle rotating; title reported roughly 100 FPS / 60 UPS; closed normally |

External C++ source editing was a file operation, not claimed as text editing
inside PixieEditor. Generated registry/CMake files were not hand-edited.
Copying example files initially retained older timestamps and left an incremental
build apparently unchanged. Refreshing the two user-owned source timestamps and
rebuilding made the new field appear. The guide advises saving copied source
before rebuilding; no Engine/Editor fix was made for this observation.

Two attempted drag gestures (viewport object and export-dialog title) produced
no visible movement through this control path. Numeric Inspector editing was
used successfully. A short synthetic D key press in Play produced no observed
displacement; sustained WASD interaction is **not** counted as visually verified.
The movement behavior was instead checked with persistent input in the headless
test below. These observations do not establish whether the control transport
or application is responsible for the unsuccessful gestures.

The export log panel opened near the window bottom and its expanded text was
clipped. Completion evidence is the produced output, matching shipping runtime
and successful independently launched player, not an invented reading of an
off-screen success line.

## Focused example checks against the installed SDK

A separate CMake consumer under `.local/consumer-handbook-20261106/headless`
compiled the handbook's PlayerController, PlatformerController, ControlPanel
and ContactBursts against `Pixie::Runtime` from the installed package, with
`pixie_stage_runtime`. Engine source targets were not built.

Generator: Visual Studio 18 2026, x64, Release. CMake resolved MSVC
`19.51.36260.0` and Windows SDK `10.0.26100.0`. An initial unquoted PowerShell
`-DCMAKE_SYSTEM_VERSION=10.0.26100.0` argument produced an extra-path warning;
the handbook command now quotes this argument. The selected SDK was still the
recorded required version. This did not change compiler compatibility checks.

The executable exited 0 with:

```text
PASS movement, normalized diagonal, disabled input
PASS platformer command deferral and intent (no motor scene)
PASS panel layout, held input, cancellation and press toggle
PASS scene contact events produce bounded particles
```

The contact check used one falling dynamic circle and a static floor in a
headless World, with scene movement and ParticleStep installed. It observed
particles emitted by the supplied ContactBursts system. The platformer check
verified command creation/intent only, not a visually exercised platform level.
The HUD helper compiled and its input/layout logic ran; drawing it was not
visually evaluated in this walkthrough.

## Reader review and remaining limits

A fresh reader agent, given the handbook files but no chat history, answered
seven consumer questions: setup/SDK root, required Computer Use, component
creation, HUD integration, collision-to-particle flow, persistence/versioning,
and export/assets. It found no blocking contradiction in the v1 path. It flagged
the Vulkan-loader prerequisite as insufficiently actionable for a clean machine;
setup now includes a launch smoke, diagnostic file check and explicit maintainer
prerequisite handoff. No clean-machine installation was performed.

Not visually exercised here: sustained movement input, HUD buttons/rendering,
platform-character collision/jump, lights/shadows/emission, PNG/font authoring,
particle rendering, compound-body events, v2 workflows, every Inspector mode,
external Open code file associations and recipient-machine setup. Their guide
sections derive from inspected public declarations/integration code, with the
focused execution above where stated. No claim of comprehensive UI coverage,
GPU matrix support, source isolation or publication follows from this record.

Claude's first independent game is the next consumer-use evidence. This record
does not pre-accept that game, its fictional mechanics or its interaction quality.

## Documentation and preservation checks

The generic documentation checker accepted 195 Engine documents (including the
new handbook/evidence paths) and 10 Editor documents with zero problems. Both
repositories passed `git diff --check`. A SHA256 comparison against the initial
ledger covered 570 tracked non-Markdown files outside vendored dependencies in
Engine and Editor; none changed. These are structural/preservation checks,
not product acceptance. The handoff ZIP contains the consolidated English guide,
examples, local setup, copyable prompt, this record and checksums, without SDK
or Editor binaries. Existing runnable local paths are supplied separately.
