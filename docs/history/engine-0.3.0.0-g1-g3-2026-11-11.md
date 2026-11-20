# Engine 0.3.0.0 SDK and Runtime candidate qualification

**Lifecycle:** historical
**Roadmap source:** PIXIE-ROADMAP-060 and the [Proposal 021 acceptance matrix](../proposals/engine-sdk-editor-separation.md#8-gates-preservation-and-completion-criteria).
**Planning baseline:** `ac48e01ed040ee607662e6cdf1f4ff9742c4b0f4`.
**Work IDs:** PIXIE-TODO-060, PIXIE-TASK-243 through PIXIE-TASK-246, PIXIE-TASK-251.
**Outcome:** G1, G2 and G3 closed for the two Windows x64 archives from GitHub Actions run `42784780770`.
**Evidence:** The [Engine workflow run](https://github.com/Sakamoto0110/PixieEngine/actions/runs/42784780770) and the retained isolated-consumer receipt identified below.
**Limitations:** This disposition covers the two pinned archives. The Editor, private automation, and the combined Proposal 021 G6 disposition have their own acceptance criteria. Publication requires separate owner authorization.

## Preserved roadmap

PIXIE-ROADMAP-060 establishes independently consumable SDK, shipping Runtime
and Editor deliveries. For this candidate pair, G1 requires installed SDK
consumption without Engine source, G2 requires preserving project generation
and exact module admission, and G3 requires a standalone shipping export
with authored data and native dependency closure. The Proposal 021 matrix
retains the full roadmap criteria.

## Qualified inputs

| Artifact | SHA-256 |
| --- | --- |
| `PixieEngine-SDK-0.3.0.0-windows-x64.zip` | `d49c8c0e07dad4b28eefc10df722443e036c6aca9a48b9ba7909e97e8201efa1` |
| `PixieEngine-Runtime-0.3.0.0-windows-x64.zip` | `abf39e0a3530ebf41c94d24070045c8421c367afb426cff4f60e82f5dfd4c78a` |
| `release.json` | `40323b990e44b69e24267bbdd2366af2587ad3787c73226fa3416cbe5b185758` |
| SDK catalog | `20cc87ee7ca0f3c4d9f5fa6f9fbc4ee6e14064a819c01f3ce090ea41d02f12bc` |

The SDK offers authoring Debug, authoring Release and shipping Release. The
Runtime matches the SDK shipping Release build
`d6df37c1036d4915327e9f1a816bede617469fc310128f49460324e8aee510ef`.
The qualification used MSVC `19.51.36260.0`, Windows SDK `10.0.26100.0` and an
OpenGL 4.5 core driver. The package metadata records AVX2, FMA and F16C.

## Acceptance disposition

| Gate | Disposition | Evidence |
| --- | --- | --- |
| G1 | Passed | An external C++ consumer configured, compiled and passed 42 headless checks in each installed profile. It linked the packaged math library and exercised the public World, Scene, presentation and scope gravity surfaces. The earlier exact-candidate refusals cover absent SDK, absent header, absent math library and shipping Debug. |
| G2 | Passed | Both authoring profiles passed 14 combined legacy, registered and custom generation cases. The authoring Release module passed 74 identity/admission checks, live-session lease and teardown, and null-factory refusal. The isolated Contract1 probe accepted a current token and refused stale authored and World tokens while preserving published state. The scene probe refused unsupported registered v2 data and retained the positive v1 control. |
| G3 | Passed | The paired SDK suite passed 25 cases and the installed code export suite passed eight. Code and no-code exports and the Runtime ZIP player rendered on OpenGL 4.5. The cases covered authored data and asset preservation, app-local native closure, missing DLLs, mixed identity and development contamination. |

Each restricted Windows worker verified its token and received access denied
for the Engine checkout, root CMake file, source C API header and Git config
before and after execution. Both ZIPs and the extracted payloads were verified
against their manifests in the restricted environment. The formalization
rechecked the two original ZIPs and all 26 retained evidence hashes.

The detailed receipt is `REVIEW-ISOLATED.md` in the workspace consumer evidence
directory `PixieEngine_ConsumerTests/q42784780770`. Its `ISOLATED-SHA256SUMS.txt`
has SHA-256 `f99e58ca93d9ac7fb623d6692bff5afff6f17e1951271e9308824878d2b37cca`.
The [earlier milestone evidence](../evidence/sdk-editor-separation-2026-11-06.md#execution-results-and-gate-dispositions)
retains the scope of its original artifacts.

## Release state at closure

The hosted Engine CI and artifact-only packaging jobs succeeded at the
qualified commit. The draft job was skipped. The workflow artifact was
available at review and has a local copy with the two ZIPs, `SHA256SUMS.txt`
and `release.json`. The Engine producer portion of G6 has this hosted receipt.
The hosted Editor consumer and its native provenance remain separate parts of
the combined G6 work. The [D09 freeze](../../ROADMAP.md#d09-glfw-distribution)
continues to govern the Editor/SDK GLFW policy.

The release owner still needs to authorize the version tag at the qualified
commit and choose how to attach the accepted bytes. The
[release workflow](../../.github/workflows/publish.yml) rebuilds on a tagged
dispatch, so compare its output to these SHA-256 pins before carrying over
this acceptance. Using the retained artifacts directly preserves the candidate
identity. The [release guide](../development.md#prepare-the-engine-sdk-and-runtime-release)
records the draft review and separate publication step. The first release notes
should point consumers to the appropriate archive, the
[setup guide](../handbook/setup.md), and the platform prerequisites.
