# Native entity parenting runtime fixture

**Document ID:** GLOBAL-NATIVE-PARENTING-RUNTIME-EVIDENCE-2026-11-07
**Schema version:** 1
**Kind:** audit
**Lifecycle:** historical
**Subject:** Release execution of the in-memory parenting slice and affected regressions
**Surface:** validation-evidence
**Boundary:** runtime
**Authority role:** evidence
**Mutation:** snapshot
**Indexing:** include

The owner promoted [Proposal 023](../proposals/native-entity-parenting.md) on
2026-11-07. This records the `PIXIE-TASK-257` slice of
[TODO-061](../../TODO.md#pixie-todo-061--native-entity-parenting). The existing
Windows x64 CMake tree was reused. The working tree included the runtime
changes under test on Engine HEAD `1f44ae74008f1c2df4ff9b86ee8bb44f8ce8e072`.
The Visual Studio generator used MSVC `19.51.36260.0` and CMake `4.3.1-msvc1`.

## Executed checks

From the Engine checkout, using the CMake and CTest executables supplied with
Visual Studio 18 Community:

```powershell
cmake --build --preset vs2026-release --target runtime-tests lighting-tests --parallel 4
ctest --preset vs2026-release -R '^(runtime-tests|lighting-tests)$' --output-on-failure --output-log '.local/parenting-023/runtime-release-ctest.log'
```

Both CTest entries passed. The runtime harness reported 314 tests with zero
failed checks, including five new parenting tests. The lighting harness
reported 42 tests with zero failed checks. The reused build emitted MSBuild
warning `MSB8028` about the `pixie_capi_objects` intermediate directory and
completed with exit code zero. Logs are in the working checkout under
`.local/parenting-023/`.

The [simple fixture](../../tests/runtime/ParentingTests.cpp) creates a root,
a pivot, two markers and an alternate root in a live World. Creation order
places the markers before their ancestors. The runtime test checks these
results with a position tolerance of `1e-9` world units:

- After a positive quarter turn, the pivot is at `(100, 120)`, marker A at
  `(90, 120)` and marker B at `(100, 130)` when PostUpdate reads them. Sampling
  the half-step render pose puts both markers on the composed rotating branch.
- A uniform root scale edit and a child local-pose edit update the world poses.
  Keep-world and keep-local edits, detach, deferred edits and deferred parent
  destruction preserve the intended relation and pose semantics.
- Self links, cycles, foreign or dead parents, scope separation, invalid scale
  and an independently moving child are rejected. A raw write to an attached
  world pose reports an error before the next step publishes an update.
- Authored attachment is refused at admission. Adding a scene ID directly to
  an attached child also results in refusal by both the scene and fragment
  writers. This guards against saving a flat substitute for the live graph.

## Artifact identities

These hashes identify the local Release test outputs in `bin/Release/`.

| File | SHA-256 |
| --- | --- |
| `pixie.dll` | `122cd169c7991aeb397759d185509ca5b295cbae6c74f0a6c9d504781cb9b606` |
| `runtime-tests.exe` | `211cf3293f703ea89f1fa7f8f731f95bfc814060bfc78c47950d6580b01ce00f` |
| `lighting-tests.exe` | `302c4beb018f5d3da609fc5820d8b8f21f9f895e27445d32b6d86daae390748a` |

## Coverage limits

This is the in-memory first stage of the simple fixture. The renderer paths
build with the shared pose sampler, and the fixture checks its numerical
output. A visual Editor and player run is part of the later integration gate.
The next slice owns authored persistence, format admission and installed SDK
compatibility. The Editor tree, Undo/Redo, code reopen and export remain open.
Weapons-Lab is reserved for acceptance after the complete simple fixture
qualifies. The [D09 freeze](../../ROADMAP.md#d09-glfw-distribution) still applies.
