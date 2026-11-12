# G6 pinned native Editor candidate

**Document ID:** GLOBAL-G6-PINNED-NATIVE-EVIDENCE-2026-11-12
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** G6 closure for the hosted Windows x64 Editor with pinned native inputs
**Surface:** validation-evidence
**Boundary:** global
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

G6 passed for the Windows x64 SDK and Runtime from
[Engine run 42784780770](https://github.com/Sakamoto0110/PixieEngine/actions/runs/42784780770)
and the Editor from
[run 43189478188](https://github.com/Sakamoto0110/PixieEditor/actions/runs/43189478188).
The Editor workflow consumed the Engine run's byte-identical public mirror
and the previously qualified Inspector pair from
[run 43183515245](https://github.com/Sakamoto0110/PixieEditor/actions/runs/43183515245).
The [previous hosted candidate](g6-hosted-editor-2026-11-12.md) preserves the
record of the historical Hexa binaries and their evidence limits.

## Candidate identities

| Input or output | Identity |
|---|---|
| Engine SDK ZIP | `d49c8c0e07dad4b28eefc10df722443e036c6aca9a48b9ba7909e97e8201efa1` |
| Engine Runtime ZIP | `abf39e0a3530ebf41c94d24070045c8421c367afb426cff4f60e82f5dfd4c78a` |
| Engine SDK catalog | `20cc87ee7ca0f3c4d9f5fa6f9fbc4ee6e14064a819c01f3ce090ea41d02f12bc` |
| Editor source | `c88b316ea0360183d10502c9600b82e35c5acb35` |
| Editor Actions artifact | `13930715797` |
| Editor ZIP | `a988daefa3b5245de2a3ebf7458596b3cae62ddf10875e7d83dcd15c8fc3da8f` |
| Editor manifest | `7d9a2006d14b428a995e0c9d214fe2812dd7b8152dc5f071159aed59750a8eef` |
| Native build receipt | `65b92fa64bdda7d8feeae61277807e3230b90de12ca9d915ff48e871d7cf8fff` |

## Affected checks

- The hosted recipe rebuilt `cimgui.dll`, `ImGuiImpl.dll` and
  `ImGuiImplGLFW.dll` using the [pinned inputs](https://github.com/Sakamoto0110/PixieEditor/blob/c88b316ea0360183d10502c9600b82e35c5acb35/eng/native/win-x64/rebuild/pins.json).
  It regenerated the cimgui wrapper against Dear ImGui 1.92.2b docking,
  matched 1,626 Hexa signatures, 126 structures and 84 generated enums,
  and matched each DLL's previous export set (1,552, 69 and 21).
  The rebuilt DLLs introduced no new dynamic imports.
- The pinned vcpkg registry supplied FreeType 2.13.3 and Brotli 1.1.0 port
  revision 1. The build checked the Brotli source archive SHA-512 and
  confirmed the decoder in the cimgui link map. The notice in the ZIP
  matches the pinned source archive. A runtime check loaded the W3C WOFF2
  fixture with the final DLL and baked U+0046 through the FreeType loader.
- Independent checks matched `SHA256SUMS.txt`, `release.json`,
  `editor.json`, `native-build.json`, and all 49 payload entries. The
  GLFW DLL, eight Microsoft runtime DLLs and packaged notices matched the
  previous candidate byte for byte. The GLFW headers came from pinned
  upstream source and matched the SDK input hashes. The GLFW import
  library was derived from the SDK's authoring/Release DLL.
- The unpacked ZIP loaded the three rebuilt DLLs and a single selected SDK
  GLFW provider in each of authoring/Release and authoring/Debug. Both
  processes opened Movement laboratory, rendered the viewport and Inspector
  in OpenGL 4.5 Core, executed Play/Stop, and exited with code 0. The
  captures matched across the two configurations.

The SDK and Runtime archives and the locked Inspector pair carry forward
their prior evidence. This closure applies to the recorded G6 candidates.
The output is an Actions verification artifact.
