# PixieLib

Primitives and math for C++ and C#, with one vocabulary and one memory layout. A `PxRect` made in C#
crosses P/Invoke and arrives in C++ as a `pxRect`, with no conversion.

- Points, sizes, rectangles, regions, paddings, and RGBA and HSL colors, in `double`, `float` and
  `int`.
- Vectors, segments and matrices, with the 2D transforms and the 3D projections. In C++ they are
  [GLM](https://github.com/g-truc/glm)'s; C# follows GLM's formulas, in the same order, so both
  sides give the same bits.
- In C++ only: events with the shape of C#'s (`+=`, `-=`, and subscriptions that end on their own).

A point is a position and a vector a displacement: point - point is a vector, a point or a rectangle
moves by a vector, and a 3x3 matrix applies its translation to a point. What depends on where the
origin is, as point + point or point * 2, does not compile.

It is not an engine: no window, rendering, input or assets, and no dependency on a UI framework.
The C# package is fully managed and calls no native code.

## C#

```
dotnet add package PixieLib
```

For .NET 10 and .NET Framework 4.8.1. A name without a suffix is `double`; `f` is `float` and `i`
is `int` (`PxPoint`, `PxPointf`, `PxPointi`).

```csharp
using PixieLib;

var screen = new PxRect(0, 0, 640, 480);
bool inside = screen.Contains(new PxPoint(100, 50));     // true
var orange = new PxColorRgba(255, 128, 0);                // alpha last, 255 by default
var turn = PxMat3.Rotate(PxMat3.Identity, Math.PI / 2);  // 2D transforms on a 3x3 matrix
PxVec3 moved = turn * new PxVec3(1, 0, 1);                // the vector is a column: M * v
PxVec2 step = new PxPoint(4, 6) - new PxPoint(1, 2);     // (3, 4): point - point is a vector
double length = new PxSegment(new PxPoint(1, 2), new PxPoint(4, 6)).Length();  // 5
```

## C++

C++20 and CMake 3.20. The CMake project is in `cpp/`:

```cmake
include(FetchContent)
FetchContent_Declare(pixielib
    GIT_REPOSITORY https://github.com/Sakamoto0110/PixieLib.git
    GIT_TAG v0.2.0
    SOURCE_SUBDIR cpp)
FetchContent_MakeAvailable(pixielib)

target_link_libraries(engine PRIVATE pixie::pixie pixie::events pixie::math)
```

- `pixie::pixie`: the primitives, only headers.
- `pixie::events`: the events, only headers.
- `pixie::math`: the vectors and matrices. It brings GLM 1.0.3; `PIXIE_SYSTEM_GLM=ON` takes GLM from
  `find_package` instead, and `PIXIE_MATH=OFF` leaves the math (and GLM) out.

```cpp
#include <pixie/pxRect.hpp>
#include <pixie/events/pxEvent.hpp>
#include <pixie/math/pxMat.hpp>

class Button {
public:
    pxEvent<Button, Button&> Click;  // only Button can raise it
    void Press() { Click(*this); }
};

void Example() {
    pxRect screen(0, 0, 640, 480);
    bool inside = screen.Contains(pxPoint(100, 50));  // true

    Button button;
    pxSubscription s = button.Click.Subscribe([](Button&) { /* ... */ });
    button.Press();

    pxMat4 projection = pxOrtho2D(640.0, 480.0);  // 2D keeps Y down
}
```

## The same bits on both sides

The layout and the rules both sides follow are in [docs/layout.md](https://github.com/Sakamoto0110/PixieLib/blob/main/docs/layout.md).
The math gives the same bits in C++ and C# when the C++ compiler does not fuse a multiply and an add
into one instruction: GCC and Clang need `-ffp-contract=off`, and MSVC does not fuse with its
default `/fp:precise`. GLM compiled by MSVC adds the dot product of a `vec4` in sequence instead of
in pairs, so there the last bit of a few `vec4` and `mat4` operations can differ.

## Documentation

- [ROADMAP.md](https://github.com/Sakamoto0110/PixieLib/blob/main/ROADMAP.md): what PixieLib is and
  is not, who uses it, and where it is going.
- [TODO.md](https://github.com/Sakamoto0110/PixieLib/blob/main/TODO.md): promoted work.
  [TASKLIST.md](https://github.com/Sakamoto0110/PixieLib/blob/main/TASKLIST.md): its task breakdowns.
- [DEPENDENCIES.md](https://github.com/Sakamoto0110/PixieLib/blob/main/DEPENDENCIES.md): every
  dependency and its status.
- [docs/](https://github.com/Sakamoto0110/PixieLib/blob/main/docs/README.md): the documentation
  index, the module references, the development guide, the proposals, and the archive, where the
  decisions behind the library are kept with their reasons.

## License

MIT, in [LICENSE](https://github.com/Sakamoto0110/PixieLib/blob/main/LICENSE).
