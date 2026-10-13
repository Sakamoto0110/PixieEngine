# Primitives Technical Reference

**Document ID:** PRIMITIVES-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the current contract of the point, size, rectangle, region, padding, and the two colors, in C++ and C#

**Surface:** technical-reference

**Boundary:** primitives

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

This reference owns the behavior of the primitives on both sides. Their fields,
order, and sizes, and the rules every shared type follows, are owned by the
[layout contract](../../layout.md) and are not repeated here. Numbers in
parentheses, such as (5.2), are the questions answered in
[PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md), which the source
also cites.

## Architecture

| Type | C++ (`cpp/include/pixie/`) | C# (`dotnet/PixieLib/`) |
|---|---|---|
| Point | `pxPoint_t<T>` in `pxPoint.hpp` | `PxPoint`, from `Templates/PxPoint.template.cs` |
| Size | `pxSize_t<T>` in `pxSize.hpp` | `PxSize`, from `Templates/PxSize.template.cs` |
| Rectangle | `pxRect_t<T>` in `pxRect.hpp` | `PxRect`, from `Templates/PxRect.template.cs` |
| Region | `pxRegion_t<T>` in `pxRegion.hpp` | `PxRegion`, from `Templates/PxRegion.template.cs` |
| Padding | `pxPadding_t<T>` in `pxPadding.hpp` | `PxPadding`, from `Templates/PxPadding.template.cs` |
| RGBA color | `pxColorRgba` in `pxColorRgba.hpp` | `PxColorRgba` in `PxColorRgba.cs` |
| HSL color | `pxColorHsl` in `pxColorHsl.hpp` | `PxColorHsl` in `PxColorHsl.cs` |

- **C++**: headers only, the `pixie::pixie` target, C++20, in the global
  namespace. Each geometric type is a template accepted only for `double`,
  `float`, and `int32_t` (`pxIsPrecision`), with the aliases `pxPoint`,
  `pxPointf`, and `pxPointi`, and so on. The fields are public and lowercase
  (`x`, `width`); what C# has as a property is a method of the same name
  (`Right()`, `Location()`). `pxPrecision.hpp` holds the shared rules
  (conversion, scalars, wrapping arithmetic), and `pxToString.hpp` the text.
- **C#**: namespace `PixieLib`. Each geometric type is a struct written once in
  a template and copied by the [generator](../generator/README.md) into
  `PxPoint` (`double`), `PxPointf`, and `PxPointi`. The fields are private and
  exposed as properties with `get` and `set` (`X`, `Width`). `PxConvert`,
  `PxHash`, and `PxText` are internal helpers.

## Construction and state

- Every type has a constructor from its fields, in field order, and its default
  is all zeros. In C#, `Empty` is that default; on both sides `IsEmpty` is true
  when every field is zero, and only then.
- A padding also takes one value for all four sides (`explicit` in C++).
- A rectangle also takes a location and a size. `Location` and `Size` read
  them back; in C#, they also have `set`, which replaces the corner or the size
  and keeps the other.
- A color takes its components with the alpha last, 255 by default (2.3); the
  RGBA constructor takes four bytes.

## Conversions

- **Between precisions** (P8.5, 5.3): implicit when nothing is lost (`float`
  and `int` to `double`), explicit otherwise. To `int`, it truncates toward
  zero, saturates out of range, and takes NaN to 0, written by hand on both
  sides (`pxConvert`, `PxConvert.ToInt32`) because the plain cast is undefined
  out of range in C++ and gives `int.MinValue` on .NET Framework. In C++,
  `float` to `double` and `int` to `double` are the implicit ones, and `int`
  to `float` is explicit, as in C#.
- **Point and size** hold the same data and convert to each other explicitly.
- **Rectangle and region** convert to each other explicitly: the region's
  `x2` is `x + width`, which can round in floating point. A region's `Width`
  and `Height` are `x2 - x1` and `y2 - y1`.
- **With `System.Drawing`** (4.4), in C# only, on the `double` types and the
  RGBA color: from `Point`, `PointF`, `Size`, `SizeF`, `Rectangle`, and
  `RectangleF` implicitly; back explicitly, rounding to the nearest even for
  the integer types and narrowing for the `float` ones. Back to a `Rectangle`,
  the edges are rounded, not the fields, so the width is what the rectangle
  covers (5.6): `(0.5, 0, 0.5, 1)` becomes `x = 0` and width 1. `PxColorRgba`
  and `Color` convert implicitly both ways.

## Arithmetic

In `int` the arithmetic wraps around, as C#'s `unchecked` default (5.2); in C++
it goes through `uint32_t`, so it is never undefined behavior. Integer division
by zero is a precondition (5.4): undefined in C++, an exception in C#. The
scalar of `*` and `/` is of the type's precision, or one that C# converts to it
implicitly (5.1): `pxSizei * 0.5` does not compile, as in C#.

- **Size**: size ± size, size × k, k × size, size / k, +size, -size.
- **Point**: point ± size is a point (the corner across a rectangle); +point.
  A point has no point + point, point × k, point / k, or -point (8.1): they
  depend on where the origin is. Point - point, and a point plus or minus a
  vector, are in [`math`](../math/REFERENCE.md#points-and-vectors), with the
  vectors.
- **`Lerp(a, b, t)`** on the point, static: `a * (1 - t) + b * t` per
  component, `glm::mix`'s formula, computed in the type of `t`. `t` is of the
  point's precision for `double` and `float`, and a `double` for `int`, where
  the result is truncated as `glm::mix` does with an `ivec2` and saturated out
  of range. A `float` point does not take a `double` `t` (5.1). It is the
  `PointAt` of the [segment](../math/REFERENCE.md#segment).
- **Padding**: `Horizontal` is `left + right` and `Vertical` is
  `top + bottom`, wrapping in `int`.
- **Rectangle**: `Right` and `Bottom` are `x + width` and `y + height`,
  wrapping in `int`. A rectangle moves by a vector, in
  [`math`](../math/REFERENCE.md#points-and-vectors); it has no `+` with a
  point (8.1). Putting it somewhere is setting `Location`.

## Rectangle and region operations

The rectangle and the region have the same operations with the same rules
(5.7, 5.8). On the rectangle in `int`, the edges are computed in 64 bits, so
these hold for any rectangle, even one whose `Right` wraps.

- **`Contains(point)`**: the left and top edges are inside, the right and
  bottom ones are not.
- **`Contains(rectangle)`**: the other's edges lie within these edges; its size
  is not looked at, so one with no area on an edge, the right one included, is
  inside.
- **`IntersectsWith`**: the two share a point. Two that only touch do not, and
  one with no area (width or height not positive) intersects nothing.
- **`Intersect(a, b)`**, static: the shared part, or the empty value (all
  zeros) when they do not intersect.
- **`Union(a, b)`**, static: the smallest one around both. One with no area
  adds nothing, so a union that starts from the empty value does not grow
  toward (0, 0); when neither has area, the result is `a`.
- **`Center`**, on the rectangle: `x + width / 2` and `y + height / 2`; in
  `int`, computed in 64 bits and rounded toward zero, as an integer division.
- **`Deflate(padding)` and `Inflate(padding)`**, on the rectangle: moved
  inward or outward by the padding. The size is not clamped and can go
  negative.

## Colors

- **`pxColorRgba` and `PxColorRgba`**: four bytes, R, G, B, A in memory, the
  order of `GL_RGBA` with `GL_UNSIGNED_BYTE` (4.2). `FromHex` and `ToHex` use
  `0xRRGGBBAA`, the number as written (2.4). In C#, `FromArgb` and `ToArgb`
  also use the `0xAARRGGBB` of `System.Drawing`.
- **`pxColorHsl` and `PxColorHsl`**: hue in degrees (0 to 360), saturation and
  lightness from 0 to 1 in `double`, and the alpha as a byte, last (2.3). There
  is no conversion operator to or from RGBA, only the static `FromRgba` and
  `ToRgba` (P8.4), with the same formulas on both sides. `ToRgba` takes the hue
  modulo 360, clamps each channel to 0 to 255 before rounding (2.8), rounds a
  half to even, and takes NaN to 0. An HSL color has no hex (4.2).

## Text, equality, and hashing

- **`ToString()`** writes the fields in order, as the
  [layout contract](../../layout.md#3-the-semantics-both-sides-repeat) says,
  the same text on both sides (4.10). C++ has only the method: no
  `operator<<` and no `std::formatter` (2.9; see
  [PXL-PROPOSAL-004](../../proposals/std-formatter.md)).
- **`==`** compares the fields with IEEE equality on both sides: NaN differs
  from everything, and 0 equals -0.
- **C# `Equals`** compares each field with its own `Equals`, so a NaN equals
  itself and a primitive works as a `Dictionary` or `HashSet` key (5.5).
  `GetHashCode` treats 0 and -0, and every NaN, as equal, on .NET Framework
  too; the RGBA color hashes as its `ToHex`.

## Non-goals

- No conversion with WinForms or WPF types; those live in their consumer
  (4.4).
- No `long` or `bool` field, and no precision besides `double`, `float`, and
  `int` (2.5).
- No point arithmetic that depends on the origin (8.1).

## Validation expectations

The C++ and C# tests use the same values. They cover, on both sides: the
layout, the conversions with saturation and NaN, the wrapping arithmetic, the
rejected scalars, the edges of `Contains`, the 64-bit edges, `Intersect`,
`Union`, `Center`, `Deflate`, `Inflate`, the HSL formulas with rounding and
clamping, the hex order, the text, and in C# `Equals`, the hashes, and the
`System.Drawing` bridge. The C++ tests run under the address and
undefined-behavior sanitizers in CI. The .NET Framework hash and conversion
paths differ from .NET 10 only on `net481`, which runs only in CI.
