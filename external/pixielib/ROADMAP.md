# PixieLib Roadmap

**Document ID:** GLOBAL-ROADMAP

**Schema version:** 1

**Kind:** roadmap/status

**Lifecycle:** current

**Subject:** what PixieLib is and is not, who uses it, direction, guardrails, and freezes

**Surface:** roadmap

**Boundary:** global

**Authority role:** roadmap

**Mutation:** authored

**Indexing:** include

**Direction decision date:** 2026-10-12

## Purpose

This document owns what PixieLib is, what it is not, where it is heading, and
which boundaries must be preserved. It does not authorize implementation;
[`TODO.md`](TODO.md) is the only scheduler. Unpromoted ideas live in
[`docs/proposals/`](docs/proposals/README.md), and the rationale behind the
decisions below is in the [archive](docs/archive/README.md): the numbered
questions and the sections the source cites are in
[PXL-ARCHIVE-001](docs/archive/decision-notes-2026-10-12.md).

## What PixieLib is

- **One library in two languages**, C++ and C#, with the same vocabulary (`px`
  in C++, `Px` in C#) and the same memory layout. Decided 2026-10-03 (1.1).
- **The layout is a contract.** Every shared type is a sequential struct of
  fixed-size fields that crosses P/Invoke without conversion: a `PxRect` made
  by the C# editor arrives in PixieEngine as the C++ `pxRect`. The contract is
  [`docs/layout.md`](docs/layout.md).
- **Primitives and math, from 2D toward 3D**: points, sizes, rectangles,
  regions, paddings, RGBA and HSL colors, vectors, matrices, the 2D transforms,
  the 3D projections, and segments.
- **In `double`, `float`, and `int`**: templates with aliases in C++, and a
  source generator in C#. The name without a suffix is `double`; `f` is
  `float` and `i` is `int` (1.2).
- **A point is a position and a vector a displacement** (8.1, 2026-10-12):
  what depends on where the origin is, such as point + point, does not exist.
- **In C++ only, events** for the engine, shaped like C#'s (1.8, 1.10, 4.1 to
  4.8).
- **Published**: the C# side as the `PixieLib` package on nuget.org, for
  .NET 10 and the .NET Framework 4.8.1; the C++ side from the same release tag,
  through CMake's `FetchContent`.

## What PixieLib is not

- **Not an implementation of vector and matrix math.** In C++ the math is
  GLM's; in C# the `float` operations go to `System.Numerics` where it gives
  GLM's bits, and the rest copies GLM's formulas in the same order (6.1, 4.5).
  "Reinvent the wheel, but not the wood" (2026-10-09).
- **Not a native-code caller.** The C# side is fully managed; PixieEngine does
  the P/Invoke, and PixieLib only guarantees that the types cross it.
- **Not an engine**: no window, rendering, input, or assets.
- **Not tied to a UI framework** (1.5). The conversions with `System.Drawing`
  are part of PixieLib; those with WinForms and WPF live in their consumer.

## Who uses it

- **PixieEngine**, in C++: 2D first, then 3D, on OpenGL.
- **PixieEngine's C# editor**, which talks to the engine through P/Invoke, and
  **PixieInspector**, its inspector; that is why the layout is a contract.
- **NekoLib**, on the .NET Framework too; that is why the `net481` target
  exists.

## Current direction

1. Serve PixieEngine as it grows: the matrix that turns a vector
   ([PXL-PROPOSAL-001](docs/proposals/mat3-times-vec2.md)) and the infinite
   line ([PXL-PROPOSAL-002](docs/proposals/infinite-line.md)) wait for its
   requests.
2. **3D**, when the engine reaches it: the quaternion, with GLM's math on the
   C++ side, and a 3D counterpart of the segment named by the dimension
   convention the library adopts (as `PxVec2` and `PxVec3`).
3. **The terminal module (`vt`)**, C++ only: the concept of `pxvt.h` from
   pxCryptLib (see [PXL-ARCHIVE-004](docs/archive/history-2026-10-02.md)).
   Not started.

## Active guardrails

- **The layout is a contract.** A type crosses between the languages only if
  its fields, order, and size are the same on both sides, checked by
  `static_assert` in C++ and by the tests in C#. A new shared type enters the
  [layout contract](docs/layout.md) before it gets code on either side.
- **The same name means the same layout** on both sides (1.2).
- **The same bits where the contract says so.** The math operations give the
  same bits in C++ (without fused multiply-add) and C#; a change that breaks
  that is a change to the contract.
- **Conversions:** implicit when nothing is lost, explicit when something may
  be lost or thrown (P8.5).
- **What depends on the origin does not compile** (8.1): no point + point,
  point * k, point / k, or -point; the conversion between point and vector
  stays explicit, because an implicit one reopens point + point in C#.
- **What GLM refuses on an integer vector, the integer types do not have**
  (8.4), with one recorded exception: the length of an integer vector, in
  `double` (8.3).
- **One DLL**: the C# package is one assembly with no dependency; the source
  generator runs at build time and does not ship.
- **Below 1.0 the API may break**, and a break raises the minor version (7.1,
  8.1).

## Freezes

None.
