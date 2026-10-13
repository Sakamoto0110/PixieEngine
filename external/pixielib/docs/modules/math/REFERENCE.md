# Math Technical Reference

**Document ID:** MATH-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the current contract of the vectors, matrices, transforms, projections, segment, and the arithmetic between points and vectors, in C++ and C#

**Surface:** technical-reference

**Boundary:** math

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

This reference owns the behavior of the math on both sides. The layout of each
type, and the rules under which both sides give the same bits, are owned by the
[layout contract](../../layout.md) and are not repeated here. Numbers in
parentheses, such as (6.1), are the questions answered in
[PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md).

## Architecture

- **C++**: the `pixie::math` target, a static library, built when the
  `PIXIE_MATH` option is on (the default). Its headers are in
  `cpp/math/include/pixie/math/`: `pxVec.hpp` (vectors, and points with
  vectors), `pxMat.hpp` (matrices and transforms), and `pxSegment.hpp`. The
  math is GLM's (6.1), pinned in [`DEPENDENCIES.md`](../../../DEPENDENCIES.md):
  the vectors and matrices are aliases of GLM types, and the functions are
  GLM's (`glm::dot`, `glm::cross`, `glm::length`, `glm::normalize`, `glm::mix`,
  `glm::transpose`, `glm::determinant`, `glm::translate`, `glm::rotate`,
  `glm::scale`, `glm::lookAt`, `glm::ortho`, `glm::perspective`, and on a
  `mat3` `glm::shearX` and `glm::shearY`). PixieLib adds only what GLM lacks,
  with a `px` prefix.
- Every file that links `pixie::math` gets `GLM_FORCE_EXPLICIT_CTOR` and
  `GLM_FORCE_CTOR_INIT` (6.3): a conversion between precisions is explicit, and
  a vector starts at zero and a matrix at the identity. GLM's experimental 2D
  transforms are included with `GLM_ENABLE_EXPERIMENTAL` defined only around
  that include. GLM's aligned types (`GLM_FORCE_DEFAULT_ALIGNED_GENTYPES`) would
  change the layout, and the `static_assert`s stop the build.
- The one compiled file, `cpp/math/src/pxMat.cpp`, holds GLM's SIMD inverse of
  a `float` `mat4`, which needs `GLM_FORCE_INTRINSICS` and would otherwise take
  `constexpr` away from every GLM type in the program (6.2). It sees no GLM
  type; `pxInverse` reaches it through raw `float` pointers.
- **C#**: structs in namespace `PixieLib`, from templates the
  [generator](../generator/README.md) copies: `PxVec2`, `PxVec3`, and `PxVec4`
  in `double`, `float` (`f`), and `int` (`i`); `PxMat3` and `PxMat4` in
  `double` and `float` only; `PxSegment` in the three. The names are
  `System.Numerics`' for the vectors (`Dot`, `Cross`, `Length()`, `Normalize`,
  `Lerp`, `Min`, `Max`, `Clamp`, `Abs`, `Zero`, `One`, `UnitX`) and GLM's in
  PascalCase for the matrices (`Translate`, `Inverse`, `Ortho`). In `float`, a
  vector operation goes to `System.Numerics` where that gives GLM's bits (4.5);
  everything else is written with GLM's formulas, in the same order.

## Vectors

- **Types**: `pxVec2`, `pxVec3`, and `pxVec4` are `glm::dvec2`, `glm::dvec3`,
  and `glm::dvec4`, with `f` for `glm::vec*` and `i` for `glm::ivec*`. The C#
  vectors have `X`, `Y`, `Z`, and `W` properties with `set`, a constructor
  from every component, one from a single value for all, and, for `PxVec3` and
  `PxVec4`, one from the smaller vector plus the last component.
- **Arithmetic**: component by component, `+`, `-`, `*`, and `/` between two
  vectors and with a scalar, and the unary ones. In C++ the scalar must be of
  the vector's own type (GLM is stricter than C#, where `PxVec3 * 2` works). On
  an integer vector, overflow and division by zero are undefined in C++,
  because the arithmetic is GLM's; in C# it wraps and division by zero throws.
- **Geometry**, in `double` and `float`: `Dot`, `Length()`,
  `LengthSquared()`, `Distance(a, b)` (the length of `b - a`),
  `DistanceSquared`, `Normalize` (multiplies by 1 / length, as GLM), `Lerp(a,
  b, t)` (`a * (1 - t) + b * t`), and `Cross` on the `vec3`. They give the same
  bits on both sides (4.13), with the exceptions the layout contract lists for
  MSVC and the `vec4`. `Normalize` and `Lerp` in `float` use GLM's formula
  over `System.Numerics`, whose own versions round differently.
- **`Min`, `Max`, `Clamp`, `Abs`** on every precision; with NaN or -0 each
  side follows its library.
- **Integer vectors** have no `Dot`, `Cross`, or `Normalize`, as in GLM, which
  refuses them with a `static_assert`; the length is the one exception, below.
  C# has no `Lerp` on an integer vector either. GLM's `glm::mix` does take an
  integer vector with a floating `t` and truncates the result; on the C# side,
  that operation exists as the point's `Lerp` and the segment's `PointAt`.
- **Conversions**: in C#, between precisions, implicit when nothing is lost and
  explicit otherwise, truncating and saturating to `int` (5.3); in C++ every
  conversion between precisions is explicit (`GLM_FORCE_EXPLICIT_CTOR`) and
  GLM's, undefined out of range. In C#, the `float` vectors convert to and from
  `Vector2`, `Vector3`, and `Vector4` implicitly, at no cost.
- **Text**: `pxToString(v)` in C++, since an alias cannot have a method, and
  `ToString()` in C#, in the format of the layout contract.

## The length of an integer vector

The length of an integer vector is seldom an integer, so it comes in `double`,
and the squared one, exact, in 64 bits (8.3), the one thing GLM refuses that
PixieLib adds (8.4). The `vec2` only: `Length()`, `LengthSquared()`,
`Distance(a, b)`, and `DistanceSquared(a, b)` on `PxVec2i`, and `pxLength`,
`pxLengthSquared`, `pxDistance`, and `pxDistanceSquared` on `pxVec2i`.

- The squared ones are computed in 64 bits and return `long` and `int64_t`.
  They wrap around past the maximum, as the rest of the integers (5.2): the
  length only at (`INT32_MIN`, `INT32_MIN`), whose square is 2^63; the distance
  between points about 3 billion apart.
- `Distance` takes the difference in 64 bits, where an `int` would wrap.
- `Length` and `Distance` square each side in `double`, add, and take the
  root, rather than the root of the exact square: C# has no wider type to do
  it in, and this way both sides give the same bits. It is exact while a square
  fits in 53 bits; beyond, a square rounds. They do not wrap.

## Points and vectors

A point is where something is; a vector is how far and which way something
moves (8.1). The operations between them:

- **point - point** is the vector from the second to the first, of the same
  precision.
- **point + vector** and **point - vector** are a point.
- **rectangle + vector** and **rectangle - vector** move the rectangle; the
  size stays.
- **mat3 * point** takes the point as `(x, y, 1)`, so the translation applies;
  in `double` and `float`, the matrices' precisions.

In `int`, the arithmetic of the point and the rectangle wraps around, the
difference too. There is no point + point, point * k, point / k, -point,
vector + point, or rectangle + point. The conversion between a point and a
`vec2` of the same precision is explicit: `pxToVec2` and `pxToPoint` in C++,
and a cast in C# (6.6). An implicit conversion would let point + point compile
in C#, through the point's own `+` with a vector (tested on 2026-10-12, 4.18).

In C++, these operators are templates in `pxVec.hpp` and `pxMat.hpp`, found by
argument-dependent lookup on the point, so that no conversion reaches them.
`point - point` is declared in `pxPoint.hpp` with a return type that only
`pxVec.hpp` provides: without `pixie::math`, it stops at a `static_assert` that
names the header and the option to use. A `vec2` through a `mat3`, as
`(x, y, 0)`, is not provided yet
([PXL-PROPOSAL-001](../../proposals/mat3-times-vec2.md)).

## Matrices

- **Types**: `pxMat3` and `pxMat4` are `glm::dmat3` and `glm::dmat4`, with `f`
  for `float`; there is no integer matrix. `PxMat3` and `PxMat4` in C# are made
  of `PxVec3` and `PxVec4` columns.
- **Convention** (1.12): a vector is a column, multiplied on the right,
  `M * v`; the memory is column-major. In `A * B * v`, `B` applies first.
- **Construction**: from a diagonal value (the rest zero), from the columns,
  a `mat3` from the top left corner of a `mat4`, and a `mat4` from a `mat3`
  plus the rest of the identity. A C++ matrix starts as the identity; a C#
  matrix's default is zero, and `Identity` is the identity (6.3).
- **Access**: `m[c]` is a column and `m[c, r]` in C# (`m[c][r]` in C++) an
  element; out of range, C# throws `ArgumentOutOfRangeException`.
- **Arithmetic**: `+`, `-`, unary `-`, a scalar `*` and `/`, `M * v`, `v * M`
  (the vector as a row), and `A * B`. The `M * v` of a `mat4` adds the columns
  in pairs, as GLM; the product of matrices and every `mat3` operation add in
  sequence.
- **`Transpose`, `Determinant`, `Inverse`**: GLM's formulas; the inverse
  multiplies by 1 / determinant, and a matrix with no inverse gives infinities
  and NaN. In C++ the inverse is `pxInverse`, which is `glm::inverse` and, on
  x86-64, takes a `float` `mat4` through GLM's SIMD version with the same bits.
- **Conversions**: `float` to `double` implicit in C#, the other way explicit;
  both explicit in C++. In C#, `PxMat4f` converts to and from
  `System.Numerics.Matrix4x4` implicitly, the same bytes: `Matrix4x4` puts the
  vector on the left, so the same bytes are the same transformation, and a
  product of two `Matrix4x4` comes in the opposite order.
- **Text**: the columns in memory order, `pxToString(m)` in C++.

## Transforms and projections

Each transform is `m` times the transform, so the transform applies before
`m`: in `T * R * S * p`, `S` comes first.

- **3D, on a `mat4`**: `Translate(m, vec3)`, `Rotate(m, angle, axis)` (radians,
  the axis normalized; counterclockwise seen from the tip of the axis),
  `Scale(m, vec3)`, `LookAt(eye, center, up)`, right-handed.
- **2D, on a `mat3`** (6.6): `Translate(m, vec2)`, `Rotate(m, angle)`,
  `Scale(m, vec2)`, `ShearX(m, k)`, which adds `k * x` to `y`, and
  `ShearY(m, k)`, which adds `k * y` to `x`, all with GLM's direction.
- **Projections** (1.13): OpenGL's, right-handed, with depth in [-1, 1]:
  `Ortho(left, right, bottom, top)` with no near and far planes (z comes out
  negated), `Ortho` with `zNear` and `zFar`, and `Perspective(fovy, aspect,
  zNear, zFar)`, `fovy` vertical in radians. `Ortho2D(width, height)`
  (`pxOrtho2D` in C++) is `Ortho(0, width, height, 0)`: the 2D has its origin
  at the top left and Y down, so (0, 0) goes to the top left corner of the
  screen, (-1, 1).
- With Y down, a positive angle around Z turns clockwise on the screen.
- The sine, cosine, and tangent come from the platform's C library on both
  sides: in C#, `MathF` for `float` on .NET 10, and the `double` functions
  rounded to `float` on .NET Framework, which has no `MathF`.

## Segment

A segment of a line, from `start` to `end` (4.17): for debug drawing, gizmos,
edges, collision, and lines cast with a maximum distance. There is no ray: a
ray with a length is a segment. In C++ it is `pxSegment_t<T>` in
`pxSegment.hpp`, with the aliases `pxSegment`, `pxSegmentf`, and `pxSegmenti`
and the fields `start` and `end`; in C#, `PxSegment`, `PxSegmentf`, and
`PxSegmenti`, with `Start` and `End` properties with `set`.

- **`Direction()`**: `end - start`, not normalized; its length is the
  segment's. In `int` it wraps around, as the point does.
- **`PointAt(t)`**: the point's `Lerp` of the two ends: `start` at 0, `end` at
  1, beyond them outside [0, 1]. `t` is of the segment's precision in `double`
  and `float`, and a `double` in `int`, where the result truncates as
  `glm::mix` does with an `ivec2` and saturates out of range. A `float`
  segment does not take a `double` `t`.
- **`Length()`**: `glm::distance` in `double` and `float`, of the segment's
  type; in `int`, the integer vector's `Distance`, a `double`, with the
  difference in 64 bits, so it does not wrap where `Direction()` does.
- Construction from the two points, all zeros by default; conversion between
  precisions as the points; `==` with IEEE equality; in C#, `Equals` and the
  hash as the [primitives](../primitives/REFERENCE.md#text-equality-and-hashing);
  the text is the four numbers, `(x1, y1, x2, y2)`.
- An infinite line is not provided
  ([PXL-PROPOSAL-002](../../proposals/infinite-line.md)).

## Non-goals

- No vector or matrix math of PixieLib's own in C++, and no SIMD of its own;
  the one SIMD path is GLM's.
- No integer matrix.
- No quaternion yet; it comes with the 3D ([`ROADMAP.md`](../../../ROADMAP.md)).

## Validation expectations

The C++ and C# tests use the same values and expect the same bits: the vector
geometry and its GLM formulas, the `vec4` dot in pairs (with MSVC's sequence
in C++), the matrix products, determinant, inverse (the SIMD path too), the
transforms, projections, and `Ortho2D`, the matrix text, the point and vector
arithmetic, the integer length with its wrapping and rounding cases, and the
segment. In C# the tests also check that no operator takes point + point,
point * k, point / k, -point, or rectangle + point. Bits were also compared on
random inputs across the two languages when each operation was written; the
counts are in [PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md),
sections 4.13 to 4.18. MSVC and the .NET Framework run only in CI.
