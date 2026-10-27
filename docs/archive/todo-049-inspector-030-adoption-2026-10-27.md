# Published Inspector 0.3.0 Adopted: PIXIE-TODO-049

**Document ID:** ARCHIVE-TODO-049-INSPECTOR-030-ADOPTION-20261027

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** historical

**Subject:** public-package Inspector 0.3.0 adoption and bounded real native consumer verification

**Surface:** archive-record

**Boundary:** global

**Authority role:** non-normative

**Mutation:** snapshot

**Indexing:** include

**Archive ID:** PIXIE-ARCHIVE-086

**Type:** completion

**Origin:** PIXIE-TODO-049 (PIXIE-TASK-199)

**Author:** codex

**Reference date:** 2026-10-27

**Commits:** `6c3d4705a3682466247a1171d3e3bbd3e9dd4184` (dependency configuration)

**Reference commit:** `703e860b40ccbb496178ae27d9c0e06258c10fe3`

**Current state:** [dependencies](../../DEPENDENCIES.md#inspector-030-consumer-adoption-2026-10-27), [editor](../../editor/README.md)

## Scope and mechanism

The owner authorized merge/publication across the three Pixie projects where
applicable. Inspector PRs 11/12 reached main and its coordinated 0.3.0 release
was published. The Engine editor selects PixieInspector.ImGui 0.3.0; its view
requires the core exactly [0.3.0]. Hexa.NET binding/backend versions and the
compatible managed PixieLib reference are unchanged. The separate PixieLib 0.2.1
release does not silently replace the Engine's vendored C++ snapshot.

The source-only Engine has no configured package/binary release workflow.
Its applicable publication is this dependency update through PR/main CI. Work
is isolated from the earlier dirty physics worktree and original checkout.
No native source, C API signature or particle layer-name bridge changes here.

## Task snapshot

**Parent:** PIXIE-TODO-049

- `PIXIE-TASK-199` — **Status:** done — Published ImGui/core 0.3.0 restored from nuget.org into an isolated cache; Editor Release and 17 real native consumer checks passed. Package provenance and remaining scene-layer bridge boundary recorded; PR/main CI is the publication gate.

## Verification and limits

VAW `c9d81ba1fe538ddaaaa44f2ca48e1dc9d3dcfcb1`, profile `pixieengine/1`, resolved bound with all seven routes checked. Binding is not build evidence.

The initial GitHub release-asset restore used its own cache and a local source;
that candidate evidence was not treated as public NuGet availability. During
indexing, the view appeared before its exact core and a public restore correctly
failed NU1102; no dependency update was merged during this partial state.

After availability, restore used a config with only
https://api.nuget.org/v3/index.json, no local feeds, no HTTP cache, and an
isolated packages-public directory (initially empty, with only public attempts).
Core and view metadata record that public source and repository commit
d9716b130d7ee4d426e1e71c8dd90cf3772075fe. Their DLL SHA-256 values match the
GitHub release artifacts. The Editor Release build passed with zero warnings/errors.

An external harness referencing this editor and its published dependency passed
17 checks with a hidden native OpenGL 4.5 context, EngineSession and the matching
C API 16 sparks library from the prior qualified worktree. It covers actual
conditional component admission, scalar read and native multi-bind writes, zero
mask write, dynamic bool write, probe cleanup, invalid-radius status and recovery,
authored mass write, calculated backend mass after the next physics tick, Stop
and unchanged authored scene SHA. The first test expected backend mass before
the tick; the harness was corrected to distinguish immediate authored state from
the last completed physics state. No production change was needed for that.

These are bounded real managed/native adapter checks, not physical-pointer or
ImGui layout acceptance and not newly committed CI tests. The external harness
and detailed evidence are retained locally under .local/validation/
inspector-030-adoption-2026-10-27, with source under the user's .codex/tmp directory.
Native C++ is unchanged from the green main baseline; PR/main CI is observed
separately before/after merging this dependency update. Documentation and
whitespace checks pass locally.

The particle category/mask names still require their explicit Engine metadata/
FlagChoices bridge; upgrading the package does not implement it. Scrub remains
a drag multiplier, not Range.Step. No 100k dynamic-particle, general consumer
parity or other package adoption claim is made.
