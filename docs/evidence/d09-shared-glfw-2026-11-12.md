# D09 shared GLFW provider

**Document ID:** GLOBAL-D09-SHARED-GLFW-EVIDENCE-2026-11-12
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** Windows x64 package parity and loaded GLFW provider
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

The owner authorized the [D09 implementation](../../ROADMAP.md#d09-glfw-distribution)
on 2026-11-12. The Editor changes were tested on top of `d9842a1`. The
rehearsal used the published SDK ZIP from Engine run `42784780770` and an
existing local private SDK for the development check.

## Package

| Input or output | SHA-256 |
|---|---|
| Engine SDK 0.3.0.0 ZIP | `d49c8c0e07dad4b28eefc10df722443e036c6aca9a48b9ba7909e97e8201efa1` |
| Public SDK catalog | `20cc87ee7ca0f3c4d9f5fa6f9fbc4ee6e14064a819c01f3ce090ea41d02f12bc` |
| Local Editor ZIP | `48a2c5529da59175ceb964bf92b4ba42763c90c1cf0560a8bafa32c66213a9d7` |

The Editor ZIP's `runtimes/win-x64/native/glfw3.dll` matches the public SDK's
`authoring/Release` file byte for byte. The staged `native-provenance.json`
records the SDK path, catalog and build identities, DLL hash, and notice. The
shipped GLFW notice matches the SDK notice. The package was built with the
locked Inspector pair and verified against the Editor ZIP inventory.

## Graphics execution

Each row ran the unpacked Editor and the matching SDK player with a disposable
copy of the Movement laboratory scene. Each process had one mapped
`glfw3.dll` at the selected SDK profile's path. The Editor also queried the
provider's `glfwGetCurrentContext` export and verified that it owned the
expected window's context before and after the SDK runtime loaded. All six
processes reported OpenGL 4.5 Core on the Radeon RX 580 2048SP, wrote a rendered
PNG, and exited with code 0.

| SDK profile and configuration | Observed GLFW SHA-256 in both processes |
|---|---|
| Public `authoring/Release` | `539f94b64f4b11a8bc8b59ed72cebc366b0291bfaad9b24597f1925444ff1bc5` |
| Public `authoring/Debug` | `a5a78d61e8fde4cbf5741066c81b80d3ce2680eb4e5273b3eba9a00c199255e0` |
| Private `local-development/Debug` | `521a12e7f19deb945c14b8f79495c225963465e940b366b32b98d74fbfa404fc` |

The private Debug Editor also rendered the start screen with its app-local
GLFW before selecting an SDK. The 144 SDK-selection checks passed, including
rejection of missing or changed GLFW and preservation of a pending create
action across the restart arguments. A separate process consumed those
arguments, created a project and opened it with the pinned SDK.

## SDK picker acceptance

The unpacked public Editor candidate opened the SDK picker with the disposable
Movement laboratory project pending. Its first process used the app-local
authoring/Release GLFW. Through the picker, the tester selected the installed
public SDK and changed the native configuration to Debug.

The first process exited and the replacement opened the pending project with the
pinned SDK catalog and authoring/Debug build. The replacement mapped exactly one
`glfw3.dll` at the SDK's Debug path with SHA-256
`a5a78d61e8fde4cbf5741066c81b80d3ce2680eb4e5273b3eba9a00c199255e0`.
The viewport rendered the scene in OpenGL 4.5. Play advanced the simulation and
Stop restored the authored scene. The replacement closed from its window and
the pre-test SDK selection and layout files were restored byte for byte.

This closes D09 for the recorded local Windows x64 tuples. Private Release
execution and hosted Editor qualification remain outside this result. Shipping
uses its own SDK profile. The Editor ZIP is a local rehearsal.
