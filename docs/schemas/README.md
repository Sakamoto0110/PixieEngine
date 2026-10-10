# Documentation Schemas

**Document ID:** GLOBAL-DOCUMENTATION-SCHEMAS

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** structural documentation schema and its derivation from NekoLib

**Surface:** index

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

[`documentation-schema.json`](documentation-schema.json) is the structural
contract for document metadata, module manifests, the two schedulers, archive
records, stable record IDs, and validation vocabulary. The meaning of that
vocabulary is owned by the
[documentation policy](../governance/documentation-policy.md).

## Derivation

The schema is a fork of NekoLib `docs/schemas/documentation-schema.json`,
schema version 1, as observed on 2026-10-09. NekoLib is a released .NET library
family; PixieEngine is a pre-code C++ and C# engine. Parts of the source that
only fit the former were changed or left out deliberately:

| Area | NekoLib | PixieEngine | Reason |
|---|---|---|---|
| Module required surfaces | 9 files and 2 directories | `MANIFEST`, `README`, `REFERENCE`, `HISTORY`; the rest on demand | Avoid empty scaffolds that prove nothing |
| Manifest fields | `.csproj`, target frameworks, packages, API baselines | Build projects, language, artifact, platforms, third-party dependencies, public API surface | C++ has no compiled-assembly API oracle; packages are not the distribution unit |
| API oracle | Assembly-derived manifests under `eng/public-api/` | A module's public header, once it exists | Same reason |
| Schedulers | `TODO.md` only | `TODO.md` plus its child `TASKLIST.md` | Owner decision, 2026-10-09 |
| Chronology | `docs/history/` plus `docs/audit/` | `docs/archive/` only, with an `archive-record` surface | Owner decision, 2026-10-09 |
| Dependency registry | Package and project files only | Root `DEPENDENCIES.md` with status and candidates | Owner decision, 2026-10-09 |
| Unversioned working state | Not part of the documentation schema | `.local/`, never authority | Owner decision, 2026-10-09 |
| Template for module introductions | `docs/templates/README.md` | `docs/templates/INTRODUCTION.md` | A file named `README.md` reads as a directory index |
| Validation profiles | Eight .NET library and runtime profiles | None yet | Defined with the first module that needs one |
| Validation categories and boundaries | Includes package, provider, database, network, hardware | Adds `rendering` and `gpu`; drops package-feed and provider terms | Engine evidence differs |
| Work campaigns and premises | Separate schemas and policies | Not adopted | Frozen in the originating tooling; not needed yet |
| Local SQLite documentation index | Policy and tooling | Not adopted | No tooling here |

No verifier checks this schema yet; see the policy's enforcement section.
