using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// A rectangle as two corners, in __T__. Same layout as pxRegion_t in C++ (docs/layout.md). (X1, Y1)
// is inside, (X2, Y2) is just outside, as in PxRect, where X2 is X + Width. It converts to and from
// PxRect explicitly, because the sums and differences can round in floating point.
[StructLayout(LayoutKind.Sequential)]
public struct PxRegion__S__ : IEquatable<PxRegion__S__>
{
    public static readonly PxRegion__S__ Empty = default;

    private __T__ x1;
    private __T__ y1;
    private __T__ x2;
    private __T__ y2;

    public PxRegion__S__(__T__ x1, __T__ y1, __T__ x2, __T__ y2)
    {
        this.x1 = x1;
        this.y1 = y1;
        this.x2 = x2;
        this.y2 = y2;
    }

    public __T__ X1
    {
        readonly get => x1;
        set => x1 = value;
    }

    public __T__ Y1
    {
        readonly get => y1;
        set => y1 = value;
    }

    public __T__ X2
    {
        readonly get => x2;
        set => x2 = value;
    }

    public __T__ Y2
    {
        readonly get => y2;
        set => y2 = value;
    }

    // In int the difference wraps around (5.2).
    public readonly __T__ Width => x2 - x1;
    public readonly __T__ Height => y2 - y1;

    public readonly bool IsEmpty => x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0;

    // The same edges as PxRect.Contains: X1 and Y1 are inside, X2 and Y2 are not.
    public readonly bool Contains(PxPoint__S__ pt) => pt.X >= x1 && pt.X < x2 && pt.Y >= y1 && pt.Y < y2;

    // The same rules as in PxRect: r is inside when its edges are, whatever its size.
    public readonly bool Contains(PxRegion__S__ r) => r.x1 >= x1 && r.x2 <= x2 && r.y1 >= y1 && r.y2 <= y2;

    // Regions that only touch share nothing, and neither does one with no area.
    public readonly bool IntersectsWith(PxRegion__S__ r) =>
        r.x1 < x2 && x1 < r.x2 && r.y1 < y2 && y1 < r.y2 && HasArea && r.HasArea;

    // The part the two share, or Empty when they share nothing.
    public static PxRegion__S__ Intersect(PxRegion__S__ a, PxRegion__S__ b)
    {
        if (!a.IntersectsWith(b))
            return Empty;
        return new PxRegion__S__(Math.Max(a.x1, b.x1), Math.Max(a.y1, b.y1), Math.Min(a.x2, b.x2), Math.Min(a.y2, b.y2));
    }

    // The smallest region around both. One with no area adds nothing; when neither has area, the result
    // is a.
    public static PxRegion__S__ Union(PxRegion__S__ a, PxRegion__S__ b)
    {
        if (!b.HasArea)
            return a;
        if (!a.HasArea)
            return b;
        return new PxRegion__S__(Math.Min(a.x1, b.x1), Math.Min(a.y1, b.y1), Math.Max(a.x2, b.x2), Math.Max(a.y2, b.y2));
    }

    private readonly bool HasArea => x2 > x1 && y2 > y1;

    public static explicit operator PxRegion__S__(PxRect__S__ r) => new PxRegion__S__(r.X, r.Y, r.Right, r.Bottom);
    public static explicit operator PxRect__S__(PxRegion__S__ rg) => new PxRect__S__(rg.x1, rg.y1, rg.Width, rg.Height);

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3).
#if PX_DOUBLE
    public static implicit operator PxRegion(PxRegionf rg) => new PxRegion(rg.X1, rg.Y1, rg.X2, rg.Y2);
    public static implicit operator PxRegion(PxRegioni rg) => new PxRegion(rg.X1, rg.Y1, rg.X2, rg.Y2);
    public static explicit operator PxRegionf(PxRegion rg) => new PxRegionf((float)rg.x1, (float)rg.y1, (float)rg.x2, (float)rg.y2);
    public static explicit operator PxRegioni(PxRegion rg) => new PxRegioni(PxConvert.ToInt32(rg.x1), PxConvert.ToInt32(rg.y1), PxConvert.ToInt32(rg.x2), PxConvert.ToInt32(rg.y2));
#elif PX_FLOAT
    public static explicit operator PxRegionf(PxRegioni rg) => new PxRegionf(rg.X1, rg.Y1, rg.X2, rg.Y2);
    public static explicit operator PxRegioni(PxRegionf rg) => new PxRegioni(PxConvert.ToInt32(rg.x1), PxConvert.ToInt32(rg.y1), PxConvert.ToInt32(rg.x2), PxConvert.ToInt32(rg.y2));
#endif

    public static bool operator ==(PxRegion__S__ a, PxRegion__S__ b) =>
        a.x1 == b.x1 && a.y1 == b.y1 && a.x2 == b.x2 && a.y2 == b.y2;
    public static bool operator !=(PxRegion__S__ a, PxRegion__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the primitive works as
    // a key; == follows IEEE, where NaN differs from everything, as in C++ (5.5).
    public readonly bool Equals(PxRegion__S__ other) =>
        x1.Equals(other.x1) && y1.Equals(other.y1) && x2.Equals(other.x2) && y2.Equals(other.y2);
    public override readonly bool Equals(object? obj) => obj is PxRegion__S__ other && Equals(other);
    public override readonly int GetHashCode() =>
        PxHash.Combine(PxHash.Of(x1), PxHash.Of(y1), PxHash.Of(x2), PxHash.Of(y2));
    public override readonly string ToString() =>
        PxText.Tuple(PxText.Number(x1), PxText.Number(y1), PxText.Number(x2), PxText.Number(y2));
}
