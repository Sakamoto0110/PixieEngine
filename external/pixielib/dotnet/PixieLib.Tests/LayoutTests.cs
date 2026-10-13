using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace PixieLib.Tests;

// The layout is the contract with C++ (docs/layout.md): the same sizes and offsets the static_asserts
// of cpp/include/pixie check, both as P/Invoke sees them (Marshal) and in managed memory (Unsafe).
internal static class LayoutTests
{
    public static void Run()
    {
        Layout<PxPoint>(16, ("x", 0), ("y", 8));
        Layout<PxPointf>(8, ("x", 0), ("y", 4));
        Layout<PxPointi>(8, ("x", 0), ("y", 4));

        Layout<PxSize>(16, ("width", 0), ("height", 8));
        Layout<PxSizef>(8, ("width", 0), ("height", 4));
        Layout<PxSizei>(8, ("width", 0), ("height", 4));

        Layout<PxRect>(32, ("x", 0), ("y", 8), ("width", 16), ("height", 24));
        Layout<PxRectf>(16, ("x", 0), ("y", 4), ("width", 8), ("height", 12));
        Layout<PxRecti>(16, ("x", 0), ("y", 4), ("width", 8), ("height", 12));

        Layout<PxRegion>(32, ("x1", 0), ("y1", 8), ("x2", 16), ("y2", 24));
        Layout<PxRegionf>(16, ("x1", 0), ("y1", 4), ("x2", 8), ("y2", 12));
        Layout<PxRegioni>(16, ("x1", 0), ("y1", 4), ("x2", 8), ("y2", 12));

        Layout<PxPadding>(32, ("left", 0), ("top", 8), ("right", 16), ("bottom", 24));
        Layout<PxPaddingf>(16, ("left", 0), ("top", 4), ("right", 8), ("bottom", 12));
        Layout<PxPaddingi>(16, ("left", 0), ("top", 4), ("right", 8), ("bottom", 12));

        // The vectors: the same layout as GLM's packed vec2, vec3 and vec4 (pxVec.hpp).
        Layout<PxVec2>(16, ("x", 0), ("y", 8));
        Layout<PxVec2f>(8, ("x", 0), ("y", 4));
        Layout<PxVec2i>(8, ("x", 0), ("y", 4));
        Layout<PxVec3>(24, ("x", 0), ("y", 8), ("z", 16));
        Layout<PxVec3f>(12, ("x", 0), ("y", 4), ("z", 8));
        Layout<PxVec3i>(12, ("x", 0), ("y", 4), ("z", 8));
        Layout<PxVec4>(32, ("x", 0), ("y", 8), ("z", 16), ("w", 24));
        Layout<PxVec4f>(16, ("x", 0), ("y", 4), ("z", 8), ("w", 12));
        Layout<PxVec4i>(16, ("x", 0), ("y", 4), ("z", 8), ("w", 12));

        // The segment: the two points, one after the other (pxSegment.hpp).
        Layout<PxSegment>(32, ("start", 0), ("end", 16));
        Layout<PxSegmentf>(16, ("start", 0), ("end", 8));
        Layout<PxSegmenti>(16, ("start", 0), ("end", 8));

        // The float ones are System.Numerics' in memory, so the conversion is a copy of the same bytes.
        Check.That(Unsafe.SizeOf<PxVec3f>() == Unsafe.SizeOf<System.Numerics.Vector3>());
        var v = new System.Numerics.Vector3(1, 2, 3);
        Check.That(Unsafe.As<System.Numerics.Vector3, PxVec3f>(ref v) == new PxVec3f(1, 2, 3));

        // The matrices: the columns one after the other, as GLM's packed mat3 and mat4 (pxMat.hpp).
        Layout<PxMat3>(72, ("c0", 0), ("c1", 24), ("c2", 48));
        Layout<PxMat3f>(36, ("c0", 0), ("c1", 12), ("c2", 24));
        Layout<PxMat4>(128, ("c0", 0), ("c1", 32), ("c2", 64), ("c3", 96));
        Layout<PxMat4f>(64, ("c0", 0), ("c1", 16), ("c2", 32), ("c3", 48));
        var matrix = new System.Numerics.Matrix4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
        Check.That(Unsafe.As<System.Numerics.Matrix4x4, PxMat4f>(ref matrix)[1] == new PxVec4f(5, 6, 7, 8));

        Layout<PxColorRgba>(4, ("r", 0), ("g", 1), ("b", 2), ("a", 3));
        Layout<PxColorHsl>(32, ("h", 0), ("s", 8), ("l", 16), ("a", 24));

        // In memory: R, G, B, A, the order OpenGL reads with GL_RGBA and GL_UNSIGNED_BYTE.
        var bytes = MemoryMarshal.AsBytes(new[] { new PxColorRgba(0x11, 0x22, 0x33, 0x44) }.AsSpan()).ToArray();
        Check.That(bytes.SequenceEqual(new byte[] { 0x11, 0x22, 0x33, 0x44 }));
    }

    // unmanaged: a primitive that held a reference would not compile here, on net481 too.
    private static void Layout<T>(int size, params (string Field, int Offset)[] fields) where T : unmanaged
    {
        string name = typeof(T).Name;
        Check.That(Marshal.SizeOf<T>() == size, $"{name}: Marshal.SizeOf == {size}");
        Check.That(Unsafe.SizeOf<T>() == size, $"{name}: Unsafe.SizeOf == {size}");
        foreach (var (field, offset) in fields)
            Check.That((int)Marshal.OffsetOf<T>(field) == offset, $"{name}.{field} at {offset}");
#if NET
        Check.That(!RuntimeHelpers.IsReferenceOrContainsReferences<T>(), $"{name} holds no references");
#endif
    }
}
