using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// A position, in __T__: where something is (8.1). Same layout as pxPoint_t in C++ (docs/layout.md).
// A point and a size hold the same data, so each converts to the other, explicitly; a point plus or
// minus a size is a point, as the corner across a rectangle. A point plus or minus a vector, the
// displacement, is a point, and point - point is the vector between them. What has no meaning for a
// position is not here: point + point, point * k, point / k and -point all give another answer when
// the origin of the world moves. The conversion to PxVec2 is explicit, so none of them comes back
// through the vector's operators.
[StructLayout(LayoutKind.Sequential)]
public struct PxPoint__S__ : IEquatable<PxPoint__S__>
{
    public static readonly PxPoint__S__ Empty = default;

    private __T__ x;
    private __T__ y;

    public PxPoint__S__(__T__ x, __T__ y)
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

    public readonly bool IsEmpty => x == 0 && y == 0;

    // In int the arithmetic wraps around, as in C++ (5.2).
    public static PxVec2__S__ operator -(PxPoint__S__ a, PxPoint__S__ b) => new PxVec2__S__(a.x - b.x, a.y - b.y);
    public static PxPoint__S__ operator +(PxPoint__S__ pt, PxVec2__S__ v) => new PxPoint__S__(pt.x + v.X, pt.y + v.Y);
    public static PxPoint__S__ operator -(PxPoint__S__ pt, PxVec2__S__ v) => new PxPoint__S__(pt.x - v.X, pt.y - v.Y);
    public static PxPoint__S__ operator +(PxPoint__S__ pt, PxSize__S__ size) => new PxPoint__S__(pt.x + size.Width, pt.y + size.Height);
    public static PxPoint__S__ operator -(PxPoint__S__ pt, PxSize__S__ size) => new PxPoint__S__(pt.x - size.Width, pt.y - size.Height);
    public static PxPoint__S__ operator +(PxPoint__S__ pt) => pt;

    // The point a fraction amount of the way from a to b. It has a meaning where point + point does
    // not: the weights add up to 1, so the origin cancels out. The formula is glm::mix's, a * (1 -
    // amount) + b * amount, as pxPoint_t::Lerp in C++; in int the amount is a double and the result is
    // truncated, as glm::mix does with an ivec2 (docs/notas.md, 4.17), saturating out of range (5.3).
#if PX_INT
    public static PxPointi Lerp(PxPointi a, PxPointi b, double amount) =>
        new PxPointi(PxConvert.ToInt32(a.x * (1 - amount) + b.x * amount), PxConvert.ToInt32(a.y * (1 - amount) + b.y * amount));
#else
    public static PxPoint__S__ Lerp(PxPoint__S__ a, PxPoint__S__ b, __T__ amount) =>
        new PxPoint__S__(a.x * (1 - amount) + b.x * amount, a.y * (1 - amount) + b.y * amount);
#endif

    public static explicit operator PxSize__S__(PxPoint__S__ pt) => new PxSize__S__(pt.x, pt.y);
    public static explicit operator PxPoint__S__(PxSize__S__ size) => new PxPoint__S__(size.Width, size.Height);

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3).
#if PX_DOUBLE
    public static implicit operator PxPoint(PxPointf pt) => new PxPoint(pt.X, pt.Y);
    public static implicit operator PxPoint(PxPointi pt) => new PxPoint(pt.X, pt.Y);
    public static explicit operator PxPointf(PxPoint pt) => new PxPointf((float)pt.x, (float)pt.y);
    public static explicit operator PxPointi(PxPoint pt) => new PxPointi(PxConvert.ToInt32(pt.x), PxConvert.ToInt32(pt.y));
#elif PX_FLOAT
    public static explicit operator PxPointf(PxPointi pt) => new PxPointf(pt.X, pt.Y);
    public static explicit operator PxPointi(PxPointf pt) => new PxPointi(PxConvert.ToInt32(pt.x), PxConvert.ToInt32(pt.y));
#endif

#if PX_DOUBLE
    // With System.Drawing (4.4): from it nothing is lost, so the conversion is implicit; back to it the
    // value is rounded (Point) or narrowed (PointF), so it is explicit.
    public static implicit operator PxPoint(System.Drawing.Point pt) => new PxPoint(pt.X, pt.Y);
    public static implicit operator PxPoint(System.Drawing.PointF pt) => new PxPoint(pt.X, pt.Y);
    public static explicit operator System.Drawing.Point(PxPoint pt) =>
        new System.Drawing.Point(PxConvert.ToInt32(Math.Round(pt.x)), PxConvert.ToInt32(Math.Round(pt.y)));
    public static explicit operator System.Drawing.PointF(PxPoint pt) => new System.Drawing.PointF((float)pt.x, (float)pt.y);
#endif

    public static bool operator ==(PxPoint__S__ a, PxPoint__S__ b) => a.x == b.x && a.y == b.y;
    public static bool operator !=(PxPoint__S__ a, PxPoint__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the primitive works as
    // a key; == follows IEEE, where NaN differs from everything, as in C++ (5.5).
    public readonly bool Equals(PxPoint__S__ other) => x.Equals(other.x) && y.Equals(other.y);
    public override readonly bool Equals(object? obj) => obj is PxPoint__S__ other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(PxHash.Of(x), PxHash.Of(y));
    public override readonly string ToString() => PxText.Tuple(PxText.Number(x), PxText.Number(y));
}
