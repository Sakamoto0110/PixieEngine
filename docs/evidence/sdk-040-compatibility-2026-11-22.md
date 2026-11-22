# SDK 0.4 compatible-project qualification

**Document ID:** GLOBAL-SDK-040-COMPATIBILITY-EVIDENCE-2026-11-22
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** Windows x64 candidate package compatibility, generation and reload checks
**Surface:** validation-evidence
**Boundary:** capi
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

## Inputs

Engine producer commit: `c68cea09e77f125e73c8d650b11d5261bf5f508c`.
Editor producer commit: `e79e785d1dec94ba6dc90a69d396af46e62f945b`.
Both products declare `0.4.0.0`. The clean detached Engine checkout produced
authoring Debug/Release and shipping Release, with the paired exporter, using
the existing `scripts/release/prepare_release.py` entrypoint through VAW.

| Identity | SHA-256 |
|---|---|
| SDK ID | `21a43e18fe3bf1bd7b6375e32e22830337a581fb0e9cdd80ee4ce25b37c6a7fa` |
| SDK catalog | `04428716ca3df20e3d6a31631301e28e79ddc6f750e10d0ed2271402d029d8ad` |
| SDK ZIP | `8d568aaaa2acf44461ec5de824879a0b1b88fb81b864da0b784746e7e0db63e0` |
| Runtime ZIP | `04e93d5e288d9e4f983359eee5738b97be73b6c793e280776bb796e2fb405e7e` |

## Executed checks

- Source-generation regression reproduced read-only generated files from
  read-only templates. After `NO_SOURCE_PERMISSIONS`, the preserving-generation
  suite passed, including ordinary writable access to all six generated files.
- CMake synthetic admission accepted a known predecessor and supported contracts
  with different hashes; rejected an exact-lock mismatch, unsupported contract
  and unknown predecessor; retained exact-identity acceptance.
- Public Editor build completed with zero warnings/errors. Its SDK selection
  suite passed 165 checks. Running the new suite against the previous Editor
  assembly failed on the declared-successor case, demonstrating the regression.
- The final SDK/Runtime archives were re-extracted and verified. Exporter/runtime
  identities matched the shipping tuple. The Editor release rehearsal produced
  and verified its ZIP against this exact SDK and locked Inspector packages.
- An existing C++ consumer retained its original `0.3.1.0` requirement with SDK
  ID `43ddf2c77b18233dd806b38184c622c8c1584e4e96e26d0eda8df33bd6ce8050`.
  It configured and compiled against the extracted candidate, including its game
  module and player, then passed paired shipping export and dependency checks.
  Hash comparison confirmed all 463 copied authored files and their originals
  remained unchanged; only separate build/export outputs were produced.
- The packaged public Editor assembly and extracted SDK passed seven native
  reload checks: initial trigger execution, compile-failure retention, distinct
  DLL staging, failed-admission retention, unsaved state preservation and new
  code execution in the same host context.
- Spritesheet builder: 34 checks, including the normal Editor export path.
  Animation playback: 30 checks, including save/reopen and FPS overrides.
  Inspector icon reload with the compiled predecessor project: 47 checks,
  including GPU pixels and handles across two session replacements.

Native graphics checks used automated hidden OpenGL 4.5 windows on an AMD Radeon
RX 580 2048SP. No human visual acceptance is claimed. The existing MSBuild
temporary-directory warning occurred during the Editor-managed module builds.

## Limits

This is bounded Windows x64 evidence, not an all-platform or universal source
compatibility proof. Native module identity checks remain exact; predecessor
modules were rebuilt. Synthetic contract checks do not establish ABI stability.
The independently installed-consumer baseline receipt, CI status, publication
and baseline activation are separate evidence layers. The existing Editor native
dependency provenance limitations remain recorded in its packaged manifest.

The canonical documentation check reported stale workspace catalog projections;
the already-modified external registry was preserved. In the detached producer,
217 documents passed structural checking, with unavailable optional workspace
and peer scopes explicitly not validated. Local logs and artifact receipts are
retained by the release attempt; GitHub release receipts identify published bytes.
