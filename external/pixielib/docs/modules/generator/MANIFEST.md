# Generator Module Manifest

**Document ID:** GENERATOR-MANIFEST

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** identity and routing of the generator module

**Surface:** manifest

**Boundary:** generator

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

```text
Projects: dotnet/PixieLib.Generators/PixieLib.Generators.csproj
Language: csharp
Artifact: analyzer
Platforms: windows, linux
Target frameworks: netstandard2.0
Module dependencies: none
Third-party dependencies: Microsoft.CodeAnalysis.CSharp
Distribution: unshipped
Stability: preview
Public API: none
Profiles: none
Technical reference: docs/modules/generator/REFERENCE.md
Related boundaries: primitives, math
Source: dotnet/PixieLib.Generators/PrecisionGenerator.cs, dotnet/PixieLib/Templates
Tests: dotnet/PixieLib.Tests
Runtime scenarios: none
```

## Routes

- [Introduction](README.md) and [technical reference](REFERENCE.md).
- It runs inside the compiler while `dotnet/PixieLib/PixieLib.csproj` builds,
  referenced as an analyzer, and does not ship in the package (1.7).
- The templates it reads belong to [`primitives`](../primitives/README.md) and
  [`math`](../math/README.md); the generated types are tested there, so a
  broken copy shows in their tests.
- The decision behind it is in
  [PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md), sections 4.1
  and 4.11; `INumber<T>` does not exist on the .NET Framework.
- No on-demand surface exists yet.
