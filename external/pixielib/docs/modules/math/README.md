# Math

**Document ID:** MATH-INTRODUCTION

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** introduction to the math module

**Surface:** introduction

**Boundary:** math

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

Vectors (2, 3, and 4 components), 3x3 and 4x4 matrices, the 2D transforms on
the 3x3 matrix, the 3D projections, the segment, and the arithmetic between a
point (where something is) and a vector (how far and which way it moves).

In C++ the vectors and matrices are GLM's, under PixieLib's names
(`pxVec2` is `glm::dvec2`), and the math is GLM's. In C# they are structs with
the same layout and `System.Numerics`' names, and their arithmetic follows
GLM's formulas in the same order, so both sides give the same bits.

```csharp
PxVec2 step = new PxPoint(4, 6) - new PxPoint(1, 2);              // (3, 4)
PxMat3 m = PxMat3.Translate(PxMat3.Identity, new PxVec2(10, 20));
PxPoint moved = m * new PxPoint(1, 1);                            // (11, 21): a point takes the translation
double length = new PxSegment(new PxPoint(1, 2), new PxPoint(4, 6)).Length();  // 5
```

```cpp
#include <pixie/math/pxMat.hpp>
#include <pixie/math/pxSegment.hpp>

pxVec2 step = pxPoint(4, 6) - pxPoint(1, 2);
pxPoint moved = glm::translate(pxMat3(1.0), pxVec2(10, 20)) * pxPoint(1, 1);
double length = pxSegment(pxPoint(1, 2), pxPoint(4, 6)).Length();
```

The contract is the [technical reference](REFERENCE.md).
