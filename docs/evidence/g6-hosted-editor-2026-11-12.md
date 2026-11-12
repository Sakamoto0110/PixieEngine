# G6 hosted Editor candidate

**Document ID:** GLOBAL-G6-HOSTED-EDITOR-EVIDENCE-2026-11-12
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** Hosted Engine-to-Editor handoff and Windows x64 native provenance
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

## Hosted handoff

The [Engine run](https://github.com/Sakamoto0110/PixieEngine/actions/runs/42784780770)
provided the approved SDK ZIP and receipt. The Windows runner obtained the
existing public mirror of those inputs. Its SDK and receipt hashes matched
the files downloaded from the original run, and the Editor workflow verified
the catalog hash and SDK inventory. The Engine Runtime retains its
[G1–G3 qualification](../history/engine-0.3.0.0-g1-g3-2026-11-11.md).

| Input or output | Identity |
|---|---|
| Engine SDK archive | `d49c8c0e07dad4b28eefc10df722443e036c6aca9a48b9ba7909e97e8201efa1` |
| Engine release receipt | `40323b990e44b69e24267bbdd2366af2587ad3787c73226fa3416cbe5b185758` |
| Engine SDK catalog | `20cc87ee7ca0f3c4d9f5fa6f9fbc4ee6e14064a819c01f3ce090ea41d02f12bc` |
| Inspector core and ImGui `0.3.0-pixie021.3` | [Artifact run 43183515245](https://github.com/Sakamoto0110/PixieEditor/actions/runs/43183515245), artifact `13926887580` |
| Editor verification build | [Run 43184627291](https://github.com/Sakamoto0110/PixieEditor/actions/runs/43184627291) at `cc98795c12eb944bd60c93c73bd1686973f72de2` |
| Editor candidate ZIP | `1427b1e7879c2cc7c0c5b1f5d0f9b178a132dd240e3f7225f30b79485ed210fd` |
| Editor manifest | `51621022712af0e5b8ccb2a00461f36f064ad7bd81a352465811b29300a4fb57` |

The Inspector artifact reuses the exact qualified packages. Both files
matched their SHA-512 lock entries before upload and after download. The
workflow restored the pair in locked mode. The Editor ZIP was downloaded
from artifact `13927713540`. Independent local checks matched
`SHA256SUMS.txt`, `release.json`, `editor.json` and all 48 payload entries.
The extracted hosted Editor opened and rendered the Movement laboratory in
OpenGL 4.5 Core, captured a frame, and exited with code 0.

## Native provenance

All 12 native DLLs and 15 native notices in scope matched their declared
digests or SDK notice inputs. GLFW matched the selected authoring/Release
SDK file and notice. The eight Microsoft runtime DLLs and notice matched
the SDK's shipping/Release redistribution payload.

The remaining three DLLs matched the exact binary imports in the upstream
[cimgui PR](https://github.com/HexaEngine/Hexa.NET.ImGui/pull/85) and
[backend PR](https://github.com/HexaEngine/Hexa.NET.ImGui/pull/86). All
three declare linker version `14.44` and contain Rich header build IDs
`33140`, `35207` and `35215` among their compiler/linker product records.
The candidate includes the original cimgui and cimgui_impl wrapper notices
in addition to the package licenses. The cimgui export `igGetVersion`
returned `1.92.2b`.

The upstream recipes show the build flags and floating source refs. The
public repository history around the build includes cimgui snapshots with
the Dear ImGui 1.92.1 submodule, which cannot explain the observed 1.92.2b
binary. The cimgui_impl repository has a contemporary commit with the
1.92.2b submodule, with no surviving build receipt that binds it to the two
backend DLLs. The historical vcpkg tree references Brotli 1.1.0 port
revision 1, which is consistent with the dictionary evidence. The vcpkg
clone was unpinned, and the dictionary match does not identify the decoder
revision compiled into cimgui.dll.

The hosted handoff and candidate verification passed. The original
wrapper source commits, complete compiler inputs and exact Brotli decoder
revision remain open for G6. This result records the verified subset for
TASK-256. The output is a verification artifact.
