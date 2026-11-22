# SDK 0.4 publication and workspace adoption

**Document ID:** GLOBAL-SDK-040-PUBLICATION-EVIDENCE-2026-11-22
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** Published Windows x64 SDK and public Editor qualification
**Surface:** validation-evidence
**Boundary:** capi
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

## Published artifacts

The owner authorized qualification, publication and subsequent baseline
promotion on 2026-11-22. The [Engine release](https://github.com/Sakamoto0110/PixieEngine/releases/tag/v0.4.0.0)
and [public Editor release](https://github.com/Sakamoto0110/PixieEditor/releases/tag/v0.4.0.0)
were published after the [Engine CI run](https://github.com/Sakamoto0110/PixieEngine/actions/runs/45032562248)
passed Debug/Release builds and tests and documentation validation.

Tags identify the exact producer code commits recorded in the
[candidate evidence](sdk-040-compatibility-2026-11-22.md). Documentation commits
follow those code commits. Uploaded asset digests matched the qualified local
archives. The public Editor ZIP SHA-256 is
`8df188640efb71b888ee8b0d67ab3fe13cdf35bf4a85542a6b2e7bd18d4316e6`.
Engine SDK and Runtime archive hashes are recorded in the candidate evidence.

The Editor used its official local release rehearsal against the exact Engine
archive. No hosted Editor CI execution or manual visual acceptance is claimed.
Existing native dependency provenance limitations remain in its manifest.

## Installed consumer evidence

An independent consumer configured and ran against the extracted SDK. Its
receipt recorded matching header/runtime identities, API 28, entity lifecycle
success and the loaded DLL path/hash. The shared fixture remained unchanged;
an attempt-owned root CMake adapter supplied the integration entry point.
Producer sources remained accessible elsewhere on the machine, so this is not
full physical source-isolation qualification.

Known predecessor projects can rebuild with the published successor without
rewriting their original SDK requirement. This does not admit old native DLLs:
native module identity checks remain exact. The
[C API reference](../modules/capi/REFERENCE.md#source-project-compatibility)
owns the compatibility policy.

## Workspace adoption

The installed VAW propagation plan completed for generation 2, snapshot
`b14aaf36eba39c34d9fa7bc30af040806aa2f26a5c5e77e8f277a1759c2ce15c`.
Run `547569ad8790934c12070f919e458056c84858bcc675714afea8b58e952ad152`
reported seven adopted current workspaces and no missing consumers:

- Engine, its detached producer, Lib and Inspector completed context sync.
- The registered SDK consumer completed configuration and execution.
- Both the canonical Editor and the existing SDK/reload reconciliation worktree
  completed preparation, build and final effective-input inspection.

The separately pinned candidate consumer was intentionally skipped by current
baseline propagation. Existing installations and location keys were retained;
the new SDK was installed below the central `SDKs/public/0.4.0.0` location.
Context sync does not port source changes into historical worktrees or prove
their UI behavior. Publication and workspace adoption remain distinct outcomes.

The workspace documentation catalog still has stale projections. Its already
modified registry was preserved; this release does not claim that catalog
maintenance is complete.
