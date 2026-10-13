using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace PixieLib;

// A vector of four components, in __T__. Same layout as pxVec4 in C++, which is the vec4 of GLM
// (docs/layout.md). The operations have the names of System.Numerics.Vector4 and give the same bits
// as the GLM function of the same job (docs/notas.md, 4.13): in float they go to System.Numerics
// (4.5), in double and int they are written here, with the formulas of GLM.
[StructLayout(LayoutKind.Sequential)]
public struct PxVec4__S__ : IEquatable<PxVec4__S__>
{
    public static readonly PxVec4__S__ Zero = default;
    public static readonly PxVec4__S__ One = new PxVec4__S__(1);
    public static readonly PxVec4__S__ UnitX = new PxVec4__S__(1, 0, 0, 0);
    public static readonly PxVec4__S__ UnitY = new PxVec4__S__(0, 1, 0, 0);
    public static readonly PxVec4__S__ UnitZ = new PxVec4__S__(0, 0, 1, 0);
    public static readonly PxVec4__S__ UnitW = new PxVec4__S__(0, 0, 0, 1);

    private __T__ x;
    private __T__ y;
    private __T__ z;
    private __T__ w;

    public PxVec4__S__(__T__ value) : this(value, value, value, value)
    {
    }

    public PxVec4__S__(__T__ x, __T__ y, __T__ z, __T__ w)
    {
        this.x = x;
        this.y = y;
        this.z = z;
        this.w = w;
    }

    public PxVec4__S__(PxVec3__S__ value, __T__ w) : this(value.X, value.Y, value.Z, w)
    {
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

    public __T__ Z
    {
        readonly get => z;
        set => z = value;
    }

    public __T__ W
    {
        readonly get => w;
        set => w = value;
    }

    // Component by component, as in GLM. In int the arithmetic wraps around (5.2) and dividing by zero
    // throws (5.4); in C++, through GLM, an int overflow is undefined, so it is a precondition there.
    public static PxVec4__S__ operator +(PxVec4__S__ a, PxVec4__S__ b) => new PxVec4__S__(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
    public static PxVec4__S__ operator -(PxVec4__S__ a, PxVec4__S__ b) => new PxVec4__S__(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
    public static PxVec4__S__ operator *(PxVec4__S__ a, PxVec4__S__ b) => new PxVec4__S__(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
    public static PxVec4__S__ operator /(PxVec4__S__ a, PxVec4__S__ b) => new PxVec4__S__(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
    public static PxVec4__S__ operator *(PxVec4__S__ v, __T__ k) => new PxVec4__S__(v.x * k, v.y * k, v.z * k, v.w * k);
    public static PxVec4__S__ operator *(__T__ k, PxVec4__S__ v) => v * k;
    public static PxVec4__S__ operator /(PxVec4__S__ v, __T__ k) => new PxVec4__S__(v.x / k, v.y / k, v.z / k, v.w / k);
    public static PxVec4__S__ operator +(PxVec4__S__ v) => v;
    public static PxVec4__S__ operator -(PxVec4__S__ v) => new PxVec4__S__(-v.x, -v.y, -v.z, -v.w);

#if PX_FLOAT
    // System.Numerics, which the JIT turns into SIMD (4.5). The conversion to and from Vector4 costs
    // nothing: the layout is the same.
    public static float Dot(PxVec4f a, PxVec4f b) => Vector4.Dot(a, b);
    public readonly float Length() => ((Vector4)this).Length();
    public readonly float LengthSquared() => ((Vector4)this).LengthSquared();
    public static float Distance(PxVec4f a, PxVec4f b) => Vector4.Distance(a, b);
    public static float DistanceSquared(PxVec4f a, PxVec4f b) => Vector4.DistanceSquared(a, b);

    // Normalize and Lerp follow the formulas of GLM, still on Vector4: Vector4.Normalize divides by the
    // length and Vector4.Lerp rounds in another order, and half the results would come out one bit off
    // those of C++. The float square root goes through double, which rounds to the same float and
    // exists in net481 (MathF does not).
    public static PxVec4f Normalize(PxVec4f v) => (Vector4)v * (1f / (float)Math.Sqrt(Vector4.Dot(v, v)));
    public static PxVec4f Lerp(PxVec4f a, PxVec4f b, float amount) => (Vector4)a * (1f - amount) + (Vector4)b * amount;

    public static PxVec4f Min(PxVec4f a, PxVec4f b) => Vector4.Min(a, b);
    public static PxVec4f Max(PxVec4f a, PxVec4f b) => Vector4.Max(a, b);
    public static PxVec4f Clamp(PxVec4f v, PxVec4f min, PxVec4f max) => Vector4.Clamp(v, min, max);
    public static PxVec4f Abs(PxVec4f v) => Vector4.Abs(v);

    // With System.Numerics, both ways: the same layout, nothing lost.
    public static implicit operator Vector4(PxVec4f v) => new Vector4(v.x, v.y, v.z, v.w);
    public static implicit operator PxVec4f(Vector4 v) => new PxVec4f(v.X, v.Y, v.Z, v.W);
#else
#if PX_DOUBLE
    // The formulas of GLM, in the same order, so that a double gives the same bits as in C++: the dot
    // product of a vec4 adds in pairs, as Vector4.Dot does in float (on MSVC, GLM adds in sequence), and
    // Normalize multiplies by 1 / length.
    public static double Dot(PxVec4 a, PxVec4 b) => (a.x * b.x + a.y * b.y) + (a.z * b.z + a.w * b.w);
    public readonly double Length() => Math.Sqrt(Dot(this, this));
    public readonly double LengthSquared() => Dot(this, this);
    public static double Distance(PxVec4 a, PxVec4 b) => (b - a).Length();
    public static double DistanceSquared(PxVec4 a, PxVec4 b) => (b - a).LengthSquared();
    public static PxVec4 Normalize(PxVec4 v) => v * (1 / Math.Sqrt(Dot(v, v)));
    public static PxVec4 Lerp(PxVec4 a, PxVec4 b, double amount) => a * (1 - amount) + b * amount;
#endif

    // GLM's min and max: (b < a) ? b : a, component by component.
    public static PxVec4__S__ Min(PxVec4__S__ a, PxVec4__S__ b) => new PxVec4__S__(b.x < a.x ? b.x : a.x, b.y < a.y ? b.y : a.y, b.z < a.z ? b.z : a.z, b.w < a.w ? b.w : a.w);
    public static PxVec4__S__ Max(PxVec4__S__ a, PxVec4__S__ b) => new PxVec4__S__(a.x < b.x ? b.x : a.x, a.y < b.y ? b.y : a.y, a.z < b.z ? b.z : a.z, a.w < b.w ? b.w : a.w);
    public static PxVec4__S__ Clamp(PxVec4__S__ v, PxVec4__S__ min, PxVec4__S__ max) => Min(Max(v, min), max);
    public static PxVec4__S__ Abs(PxVec4__S__ v) => new PxVec4__S__(v.x < 0 ? -v.x : v.x, v.y < 0 ? -v.y : v.y, v.z < 0 ? -v.z : v.z, v.w < 0 ? -v.w : v.w);
#endif

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3). In C++, through GLM, all of them are explicit.
#if PX_DOUBLE
    public static implicit operator PxVec4(PxVec4f v) => new PxVec4(v.X, v.Y, v.Z, v.W);
    public static implicit operator PxVec4(PxVec4i v) => new PxVec4(v.X, v.Y, v.Z, v.W);
    public static explicit operator PxVec4f(PxVec4 v) => new PxVec4f((float)v.x, (float)v.y, (float)v.z, (float)v.w);
    public static explicit operator PxVec4i(PxVec4 v) => new PxVec4i(PxConvert.ToInt32(v.x), PxConvert.ToInt32(v.y), PxConvert.ToInt32(v.z), PxConvert.ToInt32(v.w));
#elif PX_FLOAT
    public static explicit operator PxVec4f(PxVec4i v) => new PxVec4f(v.X, v.Y, v.Z, v.W);
    public static explicit operator PxVec4i(PxVec4f v) => new PxVec4i(PxConvert.ToInt32(v.x), PxConvert.ToInt32(v.y), PxConvert.ToInt32(v.z), PxConvert.ToInt32(v.w));
#endif

    public static bool operator ==(PxVec4__S__ a, PxVec4__S__ b) => a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    public static bool operator !=(PxVec4__S__ a, PxVec4__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the vector works as a
    // key; == follows IEEE, as in C++ (5.5).
    public readonly bool Equals(PxVec4__S__ other) =>
        x.Equals(other.x) && y.Equals(other.y) && z.Equals(other.z) && w.Equals(other.w);
    public override readonly bool Equals(object? obj) => obj is PxVec4__S__ other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(PxHash.Of(x), PxHash.Of(y), PxHash.Of(z), PxHash.Of(w));
    public override readonly string ToString() => PxText.Tuple(PxText.Number(x), PxText.Number(y), PxText.Number(z), PxText.Number(w));
}
