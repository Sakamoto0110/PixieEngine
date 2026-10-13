# Primitives Module Manifest

**Document ID:** PRIMITIVES-MANIFEST

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** identity and routing of the primitives module

**Surface:** manifest

**Boundary:** primitives

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

```text
Projects: cpp/CMakeLists.txt, dotnet/PixieLib/PixieLib.csproj
Language: cpp, csharp
Artifact: header-only, managed-assembly
Platforms: windows, linux
Target frameworks: net10.0, net481
Module dependencies: generator
Third-party dependencies: none
Distribution: source, package
Stability: preview
Public API: cpp/include/pixie, dotnet/PixieLib/PixieLib.csproj
Profiles: none
Technical reference: docs/modules/primitives/REFERENCE.md
Related boundaries: math, generator
Source: cpp/include/pixie, dotnet/PixieLib/Templates/PxPoint.template.cs, dotnet/PixieLib/Templates/PxSize.template.cs, dotnet/PixieLib/Templates/PxRect.template.cs, dotnet/PixieLib/Templates/PxRegion.template.cs, dotnet/PixieLib/Templates/PxPadding.template.cs, dotnet/PixieLib/PxColorRgba.cs, dotnet/PixieLib/PxColorHsl.cs, dotnet/PixieLib/PxText.cs, dotnet/PixieLib/PxConvert.cs, dotnet/PixieLib/PxHash.cs
Tests: cpp/tests/pxPoint.test.cpp, cpp/tests/pxSize.test.cpp, cpp/tests/pxRect.test.cpp, cpp/tests/pxRegion.test.cpp, cpp/tests/pxPadding.test.cpp, cpp/tests/pxColorRgba.test.cpp, cpp/tests/pxColorHsl.test.cpp, cpp/tests/pxToString.test.cpp, dotnet/PixieLib.Tests/PxPointTests.cs, dotnet/PixieLib.Tests/PxSizeTests.cs, dotnet/PixieLib.Tests/PxRectTests.cs, dotnet/PixieLib.Tests/PxRegionTests.cs, dotnet/PixieLib.Tests/PxPaddingTests.cs, dotnet/PixieLib.Tests/PxColorRgbaTests.cs, dotnet/PixieLib.Tests/PxColorHslTests.cs, dotnet/PixieLib.Tests/ToStringTests.cs, dotnet/PixieLib.Tests/SystemDrawingTests.cs, dotnet/PixieLib.Tests/LayoutTests.cs
Runtime scenarios: none
```

## Routes

- [Introduction](README.md) and [technical reference](REFERENCE.md).
- The layout of every type here, and the semantics both sides repeat, are in
  the [layout contract](../../layout.md).
- The C++ side is the `pixie::pixie` CMake target, headers only. The C# side is
  part of the one `PixieLib` assembly, which it shares with `math`; the types
  in three precisions come from templates the [generator](../generator/README.md)
  copies.
- The arithmetic between a point and a vector, and moving a rectangle, need the
  vectors and are in [`math`](../math/REFERENCE.md#points-and-vectors).
- The decisions behind the module are in
  [PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md), sections 4.2
  to 4.5, 4.9 to 4.12, and 4.18; the source cites them by question number.
- Proposals that touch the module:
  [PXL-PROPOSAL-004](../../proposals/std-formatter.md).
- No on-demand surface exists yet.
