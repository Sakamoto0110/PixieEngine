using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace PixieLib;

// A vector of two components, in __T__. Same layout as pxVec2 in C++, which is the vec2 of GLM
// (docs/layout.md). The operations have the names of System.Numerics.Vector2 and give the same bits
// as the GLM function of the same job (docs/notas.md, 4.13): in float they go to System.Numerics
// (4.5), in double and int they are written here, with the formulas of GLM.
[StructLayout(LayoutKind.Sequential)]
public struct PxVec2__S__ : IEquatable<PxVec2__S__>
{
    public static readonly PxVec2__S__ Zero = default;
    public static readonly PxVec2__S__ One = new PxVec2__S__(1);
    public static readonly PxVec2__S__ UnitX = new PxVec2__S__(1, 0);
    public static readonly PxVec2__S__ UnitY = new PxVec2__S__(0, 1);

    private __T__ x;
    private __T__ y;

    public PxVec2__S__(__T__ value) : this(value, value)
    {
    }

    public PxVec2__S__(__T__ x, __T__ y)
    {
        this.x = x;
        this.y = y;
    }

    public __T__ X
    {
        readonly get => x;
        set => x = value;
    }

    public __T__ Y
    {
        readonly get => y;
        set => y = value;
    }

    // Component by component, as in GLM. In int the arithmetic wraps around (5.2) and dividing by zero
    // throws (5.4); in C++, through GLM, an int overflow is undefined, so it is a precondition there.
    public static PxVec2__S__ operator +(PxVec2__S__ a, PxVec2__S__ b) => new PxVec2__S__(a.x + b.x, a.y + b.y);
    public static PxVec2__S__ operator -(PxVec2__S__ a, PxVec2__S__ b) => new PxVec2__S__(a.x - b.x, a.y - b.y);
    public static PxVec2__S__ operator *(PxVec2__S__ a, PxVec2__S__ b) => new PxVec2__S__(a.x * b.x, a.y * b.y);
    public static PxVec2__S__ operator /(PxVec2__S__ a, PxVec2__S__ b) => new PxVec2__S__(a.x / b.x, a.y / b.y);
    public static PxVec2__S__ operator *(PxVec2__S__ v, __T__ k) => new PxVec2__S__(v.x * k, v.y * k);
    public static PxVec2__S__ operator *(__T__ k, PxVec2__S__ v) => v * k;
    public static PxVec2__S__ operator /(PxVec2__S__ v, __T__ k) => new PxVec2__S__(v.x / k, v.y / k);
    public static PxVec2__S__ operator +(PxVec2__S__ v) => v;
    public static PxVec2__S__ operator -(PxVec2__S__ v) => new PxVec2__S__(-v.x, -v.y);

#if PX_FLOAT
    // System.Numerics, which the JIT turns into SIMD (4.5). The conversion to and from Vector2 costs
    // nothing: the layout is the same.
    public static float Dot(PxVec2f a, PxVec2f b) => Vector2.Dot(a, b);
    public readonly float Length() => ((Vector2)this).Length();
    public readonly float LengthSquared() => ((Vector2)this).LengthSquared();
    public static float Distance(PxVec2f a, PxVec2f b) => Vector2.Distance(a, b);
    public static float DistanceSquared(PxVec2f a, PxVec2f b) => Vector2.DistanceSquared(a, b);

    // Normalize and Lerp follow the formulas of GLM, still on Vector2: Vector2.Normalize divides by the
    // length and Vector2.Lerp rounds in another order, and half the results would come out one bit off
    // those of C++. The float square root goes through double, which rounds to the same float and
    // exists in net481 (MathF does not).
    public static PxVec2f Normalize(PxVec2f v) => (Vector2)v * (1f / (float)Math.Sqrt(Vector2.Dot(v, v)));
    public static PxVec2f Lerp(PxVec2f a, PxVec2f b, float amount) => (Vector2)a * (1f - amount) + (Vector2)b * amount;

    public static PxVec2f Min(PxVec2f a, PxVec2f b) => Vector2.Min(a, b);
    public static PxVec2f Max(PxVec2f a, PxVec2f b) => Vector2.Max(a, b);
    public static PxVec2f Clamp(PxVec2f v, PxVec2f min, PxVec2f max) => Vector2.Clamp(v, min, max);
    public static PxVec2f Abs(PxVec2f v) => Vector2.Abs(v);

    // With System.Numerics, both ways: the same layout, nothing lost.
    public static implicit operator Vector2(PxVec2f v) => new Vector2(v.x, v.y);
    public static implicit operator PxVec2f(Vector2 v) => new PxVec2f(v.X, v.Y);
#else
#if PX_DOUBLE
    // The formulas of GLM, in the same order, so that a double gives the same bits as in C++: the dot
    // product adds from x on, and Normalize multiplies by 1 / length.
    public static double Dot(PxVec2 a, PxVec2 b) => a.x * b.x + a.y * b.y;
    public readonly double Length() => Math.Sqrt(Dot(this, this));
    public readonly double LengthSquared() => Dot(this, this);
    public static double Distance(PxVec2 a, PxVec2 b) => (b - a).Length();
    public static double DistanceSquared(PxVec2 a, PxVec2 b) => (b - a).LengthSquared();
    public static PxVec2 Normalize(PxVec2 v) => v * (1 / Math.Sqrt(Dot(v, v)));
    public static PxVec2 Lerp(PxVec2 a, PxVec2 b, double amount) => a * (1 - amount) + b * amount;
#endif

#if PX_INT
    // The length of an integer vector, which GLM refuses (glm::length asks for floating point): it is
    // seldom an integer, (3, 1) measures the root of 10, so it comes in double, and the squared one,
    // exact, in long, which compares distances without a root (8.3). In C++, pxLength,
    // pxLengthSquared, pxDistance and pxDistanceSquared, with the same bits.
    //
    // The squares are computed in 64 bits, and so is the difference of Distance, which in int would
    // wrap around. The squared length wraps around past long.MaxValue, as the rest of the integers
    // (5.2): the length only at (int.MinValue, int.MinValue), the distance between points about 3
    // billion apart. The lengths in double square each side in double, exact while a square fits in
    // 53 bits and rounded beyond, as in C++; they do not wrap.
    public readonly double Length() => Hypot(x, y);
    public readonly long LengthSquared() => unchecked((long)x * x + (long)y * y);
    public static double Distance(PxVec2i a, PxVec2i b) => Hypot((long)b.x - a.x, (long)b.y - a.y);

    public static long DistanceSquared(PxVec2i a, PxVec2i b)
    {
        long dx = (long)b.x - a.x;
        long dy = (long)b.y - a.y;
        return unchecked(dx * dx + dy * dy);
    }

    private static double Hypot(long dx, long dy)
    {
        double x = dx;
        double y = dy;
        return Math.Sqrt(x * x + y * y);
    }
#endif

    // GLM's min and max: (b < a) ? b : a, component by component.
    public static PxVec2__S__ Min(PxVec2__S__ a, PxVec2__S__ b) => new PxVec2__S__(b.x < a.x ? b.x : a.x, b.y < a.y ? b.y : a.y);
    public static PxVec2__S__ Max(PxVec2__S__ a, PxVec2__S__ b) => new PxVec2__S__(a.x < b.x ? b.x : a.x, a.y < b.y ? b.y : a.y);
    public static PxVec2__S__ Clamp(PxVec2__S__ v, PxVec2__S__ min, PxVec2__S__ max) => Min(Max(v, min), max);
    public static PxVec2__S__ Abs(PxVec2__S__ v) => new PxVec2__S__(v.x < 0 ? -v.x : v.x, v.y < 0 ? -v.y : v.y);
#endif

    // A point and a vec2 of the same precision hold the same data, so each converts to the other, as a
    // point and a size do: explicitly (6.6). In C++, pxToVec2(point) and pxToPoint(vector).
    public static explicit operator PxVec2__S__(PxPoint__S__ p) => new PxVec2__S__(p.X, p.Y);
    public static explicit operator PxPoint__S__(PxVec2__S__ v) => new PxPoint__S__(v.x, v.y);

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3). In C++, through GLM, all of them are explicit.
#if PX_DOUBLE
    public static implicit operator PxVec2(PxVec2f v) => new PxVec2(v.X, v.Y);
    public static implicit operator PxVec2(PxVec2i v) => new PxVec2(v.X, v.Y);
    public static explicit operator PxVec2f(PxVec2 v) => new PxVec2f((float)v.x, (float)v.y);
    public static explicit operator PxVec2i(PxVec2 v) => new PxVec2i(PxConvert.ToInt32(v.x), PxConvert.ToInt32(v.y));
#elif PX_FLOAT
    public static explicit operator PxVec2f(PxVec2i v) => new PxVec2f(v.X, v.Y);
    public static explicit operator PxVec2i(PxVec2f v) => new PxVec2i(PxConvert.ToInt32(v.x), PxConvert.ToInt32(v.y));
#endif

    public static bool operator ==(PxVec2__S__ a, PxVec2__S__ b) => a.x == b.x && a.y == b.y;
    public static bool operator !=(PxVec2__S__ a, PxVec2__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the vector works as a
    // key; == follows IEEE, as in C++ (5.5).
    public readonly bool Equals(PxVec2__S__ other) =>
        x.Equals(other.x) && y.Equals(other.y);
    public override readonly bool Equals(object? obj) => obj is PxVec2__S__ other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(PxHash.Of(x), PxHash.Of(y));
    public override readonly string ToString() => PxText.Tuple(PxText.Number(x), PxText.Number(y));
}
