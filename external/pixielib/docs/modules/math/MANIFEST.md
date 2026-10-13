# Math Module Manifest

**Document ID:** MATH-MANIFEST

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** identity and routing of the math module

**Surface:** manifest

**Boundary:** math

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

```text
Projects: cpp/CMakeLists.txt, dotnet/PixieLib/PixieLib.csproj
Language: cpp, csharp
Artifact: static-library, managed-assembly
Platforms: windows, linux
Target frameworks: net10.0, net481
Module dependencies: primitives, generator
Third-party dependencies: GLM
Distribution: source, package
Stability: preview
Public API: cpp/math/include/pixie/math, dotnet/PixieLib/PixieLib.csproj
Profiles: none
Technical reference: docs/modules/math/REFERENCE.md
Related boundaries: primitives, generator
Source: cpp/math/include/pixie/math, cpp/math/src/pxMat.cpp, dotnet/PixieLib/Templates/PxVec2.template.cs, dotnet/PixieLib/Templates/PxVec3.template.cs, dotnet/PixieLib/Templates/PxVec4.template.cs, dotnet/PixieLib/Templates/PxMat3.template.cs, dotnet/PixieLib/Templates/PxMat4.template.cs, dotnet/PixieLib/Templates/PxSegment.template.cs, dotnet/PixieLib/PxMath.cs
Tests: cpp/tests/pxVec.test.cpp, cpp/tests/pxMat.test.cpp, cpp/tests/pxSegment.test.cpp, dotnet/PixieLib.Tests/PxVecTests.cs, dotnet/PixieLib.Tests/PxMatTests.cs, dotnet/PixieLib.Tests/PxSegmentTests.cs, dotnet/PixieLib.Tests/LayoutTests.cs
Runtime scenarios: none
```

## Routes

- [Introduction](README.md) and [technical reference](REFERENCE.md).
- The layout of the vectors, matrices, and segment, and the rules for the same
  bits on both sides, are in the [layout contract](../../layout.md).
- The C++ side is the `pixie::math` CMake target, a static library because of
  one compiled file (the SIMD inverse), behind the `PIXIE_MATH` option. GLM is
  pinned in [`DEPENDENCIES.md`](../../../DEPENDENCIES.md). The C# side is part
  of the one `PixieLib` assembly, which it shares with `primitives`.
- The point, size, and rectangle themselves are in
  [`primitives`](../primitives/REFERENCE.md); their arithmetic with vectors is
  here.
- The decisions behind the module are in
  [PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md), sections 4.13
  to 4.15, 4.17, and 4.18.
- Proposals that touch the module:
  [PXL-PROPOSAL-001](../../proposals/mat3-times-vec2.md),
  [PXL-PROPOSAL-002](../../proposals/infinite-line.md), and
  [PXL-PROPOSAL-003](../../proposals/named-point-conversion.md).
- No on-demand surface exists yet.
