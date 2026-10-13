# Primitives

**Document ID:** PRIMITIVES-INTRODUCTION

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** introduction to the primitives module

**Surface:** introduction

**Boundary:** primitives

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

The primitives are the small value types of a 2D interface and engine: a point,
a size, a rectangle, a region (a rectangle as two corners), a padding, and an
RGBA and an HSL color. They exist in C++ and C#, with the same names, fields,
and memory layout, so that one crosses P/Invoke as the other.

```csharp
var screen = new PxRect(0, 0, 640, 480);
bool inside = screen.Contains(new PxPoint(100, 50));   // true: left and top edges are inside
PxRect content = screen.Deflate(new PxPadding(8));    // (8, 8, 624, 464)
```

```cpp
pxRect screen(0, 0, 640, 480);
bool inside = screen.Contains(pxPoint(100, 50));
pxColorRgba orange = pxColorRgba::FromHex(0xFF8000FF);
```

Each geometric type comes in `double` (no suffix), `float` (`f`), and `int`
(`i`); the colors have one precision. Conversions between precisions are
implicit when nothing is lost and explicit otherwise.

The contract is the [technical reference](REFERENCE.md), and the layout is in
the [layout contract](../../layout.md).
