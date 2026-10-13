# Layout Contract

**Document ID:** GLOBAL-LAYOUT-CONTRACT

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the layout of every type C++ and C# share, and the semantics both sides repeat

**Surface:** technical-reference

**Boundary:** global

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

The contract between C++ and C#: every shared type has the same fields, in the
same order and of the same types, in both languages, and crosses P/Invoke
without conversion. In C++, each header checks its own layout with
`static_assert` (size and offset of each field); in C#, the structs are
sequential (`[StructLayout(LayoutKind.Sequential)]`) with their fields in this
order, and the tests check size and offset through `Marshal` and `Unsafe`.

This is a global reference because every module depends on it; the module
references link to the parts that concern them. Its Portuguese original, as it
stood before this rewrite, is [PXL-ARCHIVE-006](archive/layout-contract-2026-10-12.md),
and the decisions behind it are in
[PXL-ARCHIVE-001](archive/decision-notes-2026-10-12.md), cited below by
section.

## 1. The types

Sizes are in bytes, per precision: `double` (no suffix), `float` (`f`), and
`int32_t` (`i`).

| C++ | C# | Fields, in order | double | float | int32 |
|---|---|---|---|---|---|
| `pxPoint` | `PxPoint` | `x`, `y` | 16 | 8 | 8 |
| `pxSize` | `PxSize` | `width`, `height` | 16 | 8 | 8 |
| `pxRect` | `PxRect` | `x`, `y`, `width`, `height` | 32 | 16 | 16 |
| `pxRegion` | `PxRegion` | `x1`, `y1`, `x2`, `y2` | 32 | 16 | 16 |
| `pxPadding` | `PxPadding` | `left`, `top`, `right`, `bottom` | 32 | 16 | 16 |
| `pxVec2` | `PxVec2` | `x`, `y` | 16 | 8 | 8 |
| `pxVec3` | `PxVec3` | `x`, `y`, `z` | 24 | 12 | 12 |
| `pxVec4` | `PxVec4` | `x`, `y`, `z`, `w` | 32 | 16 | 16 |
| `pxSegment` | `PxSegment` | `start`, `end`: two points, one after the other | 32 | 16 | 16 |
| `pxMat3` | `PxMat3` | three `pxVec3` columns, one after the other | 72 | 36 | none |
| `pxMat4` | `PxMat4` | four `pxVec4` columns, one after the other | 128 | 64 | none |

The C++ vectors and matrices are GLM's (4.13 and 4.14), in its default types,
without extra alignment. The matrices are column-major: the whole first column
comes first in memory, then the second. The C# `float` types have the layout of
`System.Numerics`' `Vector2`, `Vector3`, `Vector4`, and `Matrix4x4`: the bytes
of column 0 of a `PxMat4f` are `M11` to `M14` of the `Matrix4x4`.

The colors have one precision:

| C++ | C# | Fields, in order | Size |
|---|---|---|---|
| `pxColorRgba` | `PxColorRgba` | `r`, `g`, `b`, `a`, one byte each | 4 |
| `pxColorHsl` | `PxColorHsl` | `h`, `s`, `l` in `double`, `a` in a byte | 32 |

Only `pxColorHsl` has padding: the three `double` take 24 bytes, the alpha the
25th, and seven bytes complete the size to a multiple of 8, the alignment of a
`double`.

## 2. Rules

- An integer is always `int32_t` in C++ and `int` in C#; a byte is `uint8_t`
  and `byte`. No `long`, whose size changes between platforms in C++, and no
  `bool`, which has no fixed layout through P/Invoke.
- A new type enters the tables above before it gets code on either side.

## 3. The semantics both sides repeat

The layout guarantees that the data crosses; these rules guarantee that it
means the same thing.

- `Contains`, on the rectangle and the region: the left and top edges are
  inside, the right and bottom ones are not (`x >= x1 && x < x2`).
- Region and rectangle: `x2 = x + width`, `y2 = y + height`.
- Between precisions: implicit when nothing is lost (`float` and `int32` to
  `double`), explicit when something may be. To `int32`, it truncates,
  saturates out of range, and takes NaN to 0 (4.12).
- Integers: the arithmetic wraps around past `int32` (`INT32_MAX + 1` is
  `INT32_MIN`), and division by zero is a precondition. The edges of a
  rectangle, in its operations, are computed in 64 bits.
- Equality: `==` follows IEEE on both sides (NaN differs from everything); in
  C#, `Equals` says NaN equals NaN, so that a type can be a key.
- Operations of the rectangle and the region: `Intersect` gives the empty one
  (all fields zero) when the two do not meet; two that only touch do not
  intersect, and one with no area intersects nothing and does not enter a
  `Union`. The `Contains` of a rectangle looks only at the edges.
- HSL to RGBA: a half unit rounds to even (the default of C#'s `Math.Round`;
  `std::nearbyint` in C++), and the value is clamped to 0 to 255 first.
- Text (`ToString`): the fields in order, in parentheses, separated by `, `, as
  in `(1.5, -2)` and `(120, 1, 0.5, 255)`. The decimal separator is a period in
  any culture. A number comes out with the fewest significant digits that read
  back as the same value, padded with zeros and without scientific notation:
  `100000`, not `1e+05`; the `float` 1e20 is `100000000000000000000`. NaN is
  `nan` whatever its sign, and the infinities are `inf` and `-inf`. Bytes and
  integers come out as numbers. HSL has no hex (4.2). Checked: the text of
  44,905 random `double` and `float` values is the same in both languages
  (4.11). In C#, the digits come from the exact value, not from the runtime,
  because the .NET Framework does not always round the last one (4.16).
- Vectors: the C++ math is GLM's, and C# uses the same formulas in the same
  order; `Dot`, `Length`, `Distance`, `Cross`, `Normalize`, and `Lerp` give the
  same bits in `double` and `float` (4.13). That holds with C++ compiled
  without fusing a multiply and an add into one instruction (FMA): on GCC and
  Clang with `-ffp-contract=off`, because Clang fuses by default, and with
  optimization even on a processor without FMA, in the arithmetic it does while
  compiling; MSVC does not fuse with its default `/fp:precise` (4.16). The dot
  product of a `vec4` adds in pairs, `(x + y) + (z + w)`, on both sides, except
  in GLM compiled by MSVC, which adds in sequence: there, the `Dot`, `Length`,
  and `Normalize` of a `vec4`, and the `v * M` of a `mat4`, may differ in the
  last bit. `Min`, `Max`, `Clamp`, and `Abs` with NaN or -0 follow each
  library. On an integer vector, overflow and an out-of-range conversion are
  undefined in C++, because the arithmetic is GLM's, and wrap around and
  saturate in C#.
- Matrices: a vector is a column, multiplied on the right, `M * v`; in
  `A * B * v`, `B` applies first. C# uses GLM's formulas in the same order and
  gives the same bits in `double` and `float` (4.14): the `M * v` of a `mat4`
  adds the columns in pairs, the product of matrices and the `mat3` add in
  sequence, and the inverse multiplies by 1 / determinant. The sine, cosine,
  and tangent of `Rotate` and `Perspective` come from the platform's C library
  on both sides; on `net481` the `float` goes through `double`, and the cosine
  of the .NET Framework may differ (its cosine of pi / 2 is neither glibc's nor
  MSVC's; 4.16). `System.Numerics`' `Matrix4x4` has the same bytes but
  multiplies with the vector on the left (`v * M`), so the product of two comes
  in the opposite order, and its arithmetic does not give GLM's bits.
- Projections and axes: OpenGL's, right-handed, with depth in [-1, 1]. The 2D
  has the origin at the top left and Y down, and `Ortho2D(width, height)` takes
  (0, 0) to the top left corner of the screen, (-1, 1). With Y down, a
  positive angle around Z turns clockwise on the screen.
- 2D transforms: on the `mat3`, with a point as `(x, y, 1)`, GLM's on both
  sides (4.15). A point and a `vec2` of the same precision convert to each
  other, explicitly.
- Point and vector (4.18): point - point is a vector; point plus or minus a
  vector, and point plus or minus a size, is a point; a rectangle moves by a
  vector. Point + point, point * k, point / k, and -point do not exist.
  `mat3 * point` takes the point as `(x, y, 1)`, with the translation. The
  `Lerp` of two points and the `PointAt` of a segment use `glm::mix`'s formula,
  `a * (1 - t) + b * t`, in the type of `t`; in `int32`, `t` is a `double` and
  the result truncates, as `glm::mix` does with an `ivec2`, and saturates out of
  range, where GLM's conversion would be undefined.
- The length of an integer vector (4.17): `Length` and `Distance` in `double`,
  squaring each side in `double`, and `LengthSquared` and `DistanceSquared`
  exact in 64 bits, wrapping around past `int64`; the difference of `Distance`
  is computed in 64 bits. An integer segment measures through `Distance`; its
  `Direction` wraps around, as a point does.
- Text of a matrix: the columns in memory order, each in the format of a
  vector, as in `((1, 0, 0), (0, 1, 0), (0, 0, 1))`.
- Hex of a color: `0xRRGGBBAA`, the order the number is written in, built with
  bit shifts. In C#, `ToArgb()` in `0xAARRGGBB` exists only for
  `System.Drawing`.
