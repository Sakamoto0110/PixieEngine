using System;
using System.Numerics;
using System.Runtime.InteropServices;

namespace PixieLib;

// A vector of three components, in __T__. Same layout as pxVec3 in C++, which is the vec3 of GLM
// (docs/layout.md). The operations have the names of System.Numerics.Vector3 and give the same bits
// as the GLM function of the same job (docs/notas.md, 4.13): in float they go to System.Numerics
// (4.5), in double and int they are written here, with the formulas of GLM.
[StructLayout(LayoutKind.Sequential)]
public struct PxVec3__S__ : IEquatable<PxVec3__S__>
{
    public static readonly PxVec3__S__ Zero = default;
    public static readonly PxVec3__S__ One = new PxVec3__S__(1);
    public static readonly PxVec3__S__ UnitX = new PxVec3__S__(1, 0, 0);
    public static readonly PxVec3__S__ UnitY = new PxVec3__S__(0, 1, 0);
    public static readonly PxVec3__S__ UnitZ = new PxVec3__S__(0, 0, 1);

    private __T__ x;
    private __T__ y;
    private __T__ z;

    public PxVec3__S__(__T__ value) : this(value, value, value)
    {
    }

    public PxVec3__S__(__T__ x, __T__ y, __T__ z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }

    public PxVec3__S__(PxVec2__S__ value, __T__ z) : this(value.X, value.Y, z)
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

    // Component by component, as in GLM. In int the arithmetic wraps around (5.2) and dividing by zero
    // throws (5.4); in C++, through GLM, an int overflow is undefined, so it is a precondition there.
    public static PxVec3__S__ operator +(PxVec3__S__ a, PxVec3__S__ b) => new PxVec3__S__(a.x + b.x, a.y + b.y, a.z + b.z);
    public static PxVec3__S__ operator -(PxVec3__S__ a, PxVec3__S__ b) => new PxVec3__S__(a.x - b.x, a.y - b.y, a.z - b.z);
    public static PxVec3__S__ operator *(PxVec3__S__ a, PxVec3__S__ b) => new PxVec3__S__(a.x * b.x, a.y * b.y, a.z * b.z);
    public static PxVec3__S__ operator /(PxVec3__S__ a, PxVec3__S__ b) => new PxVec3__S__(a.x / b.x, a.y / b.y, a.z / b.z);
    public static PxVec3__S__ operator *(PxVec3__S__ v, __T__ k) => new PxVec3__S__(v.x * k, v.y * k, v.z * k);
    public static PxVec3__S__ operator *(__T__ k, PxVec3__S__ v) => v * k;
    public static PxVec3__S__ operator /(PxVec3__S__ v, __T__ k) => new PxVec3__S__(v.x / k, v.y / k, v.z / k);
    public static PxVec3__S__ operator +(PxVec3__S__ v) => v;
    public static PxVec3__S__ operator -(PxVec3__S__ v) => new PxVec3__S__(-v.x, -v.y, -v.z);

#if PX_FLOAT
    // System.Numerics, which the JIT turns into SIMD (4.5). The conversion to and from Vector3 costs
    // nothing: the layout is the same.
    public static float Dot(PxVec3f a, PxVec3f b) => Vector3.Dot(a, b);
    public static PxVec3f Cross(PxVec3f a, PxVec3f b) => Vector3.Cross(a, b);
    public readonly float Length() => ((Vector3)this).Length();
    public readonly float LengthSquared() => ((Vector3)this).LengthSquared();
    public static float Distance(PxVec3f a, PxVec3f b) => Vector3.Distance(a, b);
    public static float DistanceSquared(PxVec3f a, PxVec3f b) => Vector3.DistanceSquared(a, b);

    // Normalize and Lerp follow the formulas of GLM, still on Vector3: Vector3.Normalize divides by the
    // length and Vector3.Lerp rounds in another order, and half the results would come out one bit off
    // those of C++. The float square root goes through double, which rounds to the same float and
    // exists in net481 (MathF does not).
    public static PxVec3f Normalize(PxVec3f v) => (Vector3)v * (1f / (float)Math.Sqrt(Vector3.Dot(v, v)));
    public static PxVec3f Lerp(PxVec3f a, PxVec3f b, float amount) => (Vector3)a * (1f - amount) + (Vector3)b * amount;

    public static PxVec3f Min(PxVec3f a, PxVec3f b) => Vector3.Min(a, b);
    public static PxVec3f Max(PxVec3f a, PxVec3f b) => Vector3.Max(a, b);
    public static PxVec3f Clamp(PxVec3f v, PxVec3f min, PxVec3f max) => Vector3.Clamp(v, min, max);
    public static PxVec3f Abs(PxVec3f v) => Vector3.Abs(v);

    // With System.Numerics, both ways: the same layout, nothing lost.
    public static implicit operator Vector3(PxVec3f v) => new Vector3(v.x, v.y, v.z);
    public static implicit operator PxVec3f(Vector3 v) => new PxVec3f(v.X, v.Y, v.Z);
#else
#if PX_DOUBLE
    // The formulas of GLM, in the same order, so that a double gives the same bits as in C++: the dot
    // product adds from x on, and Normalize multiplies by 1 / length.
    public static double Dot(PxVec3 a, PxVec3 b) => a.x * b.x + a.y * b.y + a.z * b.z;
    public static PxVec3 Cross(PxVec3 a, PxVec3 b) =>
        new PxVec3(a.y * b.z - b.y * a.z, a.z * b.x - b.z * a.x, a.x * b.y - b.x * a.y);
    public readonly double Length() => Math.Sqrt(Dot(this, this));
    public readonly double LengthSquared() => Dot(this, this);
    public static double Distance(PxVec3 a, PxVec3 b) => (b - a).Length();
    public static double DistanceSquared(PxVec3 a, PxVec3 b) => (b - a).LengthSquared();
    public static PxVec3 Normalize(PxVec3 v) => v * (1 / Math.Sqrt(Dot(v, v)));
    public static PxVec3 Lerp(PxVec3 a, PxVec3 b, double amount) => a * (1 - amount) + b * amount;
#endif

    // GLM's min and max: (b < a) ? b : a, component by component.
    public static PxVec3__S__ Min(PxVec3__S__ a, PxVec3__S__ b) => new PxVec3__S__(b.x < a.x ? b.x : a.x, b.y < a.y ? b.y : a.y, b.z < a.z ? b.z : a.z);
    public static PxVec3__S__ Max(PxVec3__S__ a, PxVec3__S__ b) => new PxVec3__S__(a.x < b.x ? b.x : a.x, a.y < b.y ? b.y : a.y, a.z < b.z ? b.z : a.z);
    public static PxVec3__S__ Clamp(PxVec3__S__ v, PxVec3__S__ min, PxVec3__S__ max) => Min(Max(v, min), max);
    public static PxVec3__S__ Abs(PxVec3__S__ v) => new PxVec3__S__(v.x < 0 ? -v.x : v.x, v.y < 0 ? -v.y : v.y, v.z < 0 ? -v.z : v.z);
#endif

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3). In C++, through GLM, all of them are explicit.
#if PX_DOUBLE
    public static implicit operator PxVec3(PxVec3f v) => new PxVec3(v.X, v.Y, v.Z);
    public static implicit operator PxVec3(PxVec3i v) => new PxVec3(v.X, v.Y, v.Z);
    public static explicit operator PxVec3f(PxVec3 v) => new PxVec3f((float)v.x, (float)v.y, (float)v.z);
    public static explicit operator PxVec3i(PxVec3 v) => new PxVec3i(PxConvert.ToInt32(v.x), PxConvert.ToInt32(v.y), PxConvert.ToInt32(v.z));
#elif PX_FLOAT
    public static explicit operator PxVec3f(PxVec3i v) => new PxVec3f(v.X, v.Y, v.Z);
    public static explicit operator PxVec3i(PxVec3f v) => new PxVec3i(PxConvert.ToInt32(v.x), PxConvert.ToInt32(v.y), PxConvert.ToInt32(v.z));
#endif

    public static bool operator ==(PxVec3__S__ a, PxVec3__S__ b) => a.x == b.x && a.y == b.y && a.z == b.z;
    public static bool operator !=(PxVec3__S__ a, PxVec3__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the vector works as a
    // key; == follows IEEE, as in C++ (5.5).
    public readonly bool Equals(PxVec3__S__ other) =>
        x.Equals(other.x) && y.Equals(other.y) && z.Equals(other.z);
    public override readonly bool Equals(object? obj) => obj is PxVec3__S__ other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(PxHash.Combine(PxHash.Of(x), PxHash.Of(y)), PxHash.Of(z));
    public override readonly string ToString() => PxText.Tuple(PxText.Number(x), PxText.Number(y), PxText.Number(z));
}
