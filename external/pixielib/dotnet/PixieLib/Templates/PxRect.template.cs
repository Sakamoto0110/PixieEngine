using System;
using System.Runtime.InteropServices;
#if PX_INT
using Wide = System.Int64;
#else
using Wide = __T__;
#endif

namespace PixieLib;

// A rectangle as a corner and a size, in __T__. Same layout as pxRect_t in C++ (docs/layout.md).
// PxRegion holds the same thing as two corners.
[StructLayout(LayoutKind.Sequential)]
public struct PxRect__S__ : IEquatable<PxRect__S__>
{
    public static readonly PxRect__S__ Empty = default;

    private __T__ x;
    private __T__ y;
    private __T__ width;
    private __T__ height;

    public PxRect__S__(__T__ x, __T__ y, __T__ width, __T__ height)
    {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }

    public PxRect__S__(PxPoint__S__ location, PxSize__S__ size) : this(location.X, location.Y, size.Width, size.Height)
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

    public __T__ Width
    {
        readonly get => width;
        set => width = value;
    }

    public __T__ Height
    {
        readonly get => height;
        set => height = value;
    }

    // In int, Right and Bottom wrap around when X + Width does not fit (5.2). The operations below
    // compute the edges in 64 bits instead, so they hold for any rectangle.
    public readonly __T__ Right => x + width;
    public readonly __T__ Bottom => y + height;

    public PxPoint__S__ Location
    {
        readonly get => new PxPoint__S__(x, y);
        set
        {
            x = value.X;
            y = value.Y;
        }
    }

    public PxSize__S__ Size
    {
        readonly get => new PxSize__S__(width, height);
        set
        {
            width = value.Width;
            height = value.Height;
        }
    }

    // In int, half the size is rounded toward zero, as an integer division.
    public readonly PxPoint__S__ Center => new PxPoint__S__((__T__)((Wide)x + (Wide)width / 2), (__T__)((Wide)y + (Wide)height / 2));

    public readonly bool IsEmpty => x == 0 && y == 0 && width == 0 && height == 0;

    // The left and top edges are inside, the right and bottom ones are not.
    public readonly bool Contains(PxPoint__S__ pt) => pt.X >= x && pt.X < Right64 && pt.Y >= y && pt.Y < Bottom64;

    // Whether r lies inside, its edges within these edges. Its size is not looked at: a rectangle with
    // no area on an edge, the right one included, is inside.
    public readonly bool Contains(PxRect__S__ r) => r.x >= x && r.Right64 <= Right64 && r.y >= y && r.Bottom64 <= Bottom64;

    // Whether the two share any point. Rectangles that only touch do not, since the right and bottom
    // edges are outside; one with no area shares nothing.
    public readonly bool IntersectsWith(PxRect__S__ r) =>
        r.x < Right64 && x < r.Right64 && r.y < Bottom64 && y < r.Bottom64 && HasArea && r.HasArea;

    // The part the two share, or Empty when they share nothing.
    public static PxRect__S__ Intersect(PxRect__S__ a, PxRect__S__ b)
    {
        if (!a.IntersectsWith(b))
            return Empty;
        return FromEdges(Math.Max(a.x, b.x), Math.Max(a.y, b.y), Math.Min(a.Right64, b.Right64), Math.Min(a.Bottom64, b.Bottom64));
    }

    // The smallest rectangle around both. One with no area adds nothing, so a Union that starts from
    // Empty does not grow toward (0, 0); when neither has area, the result is a.
    public static PxRect__S__ Union(PxRect__S__ a, PxRect__S__ b)
    {
        if (!b.HasArea)
            return a;
        if (!a.HasArea)
            return b;
        return FromEdges(Math.Min(a.x, b.x), Math.Min(a.y, b.y), Math.Max(a.Right64, b.Right64), Math.Max(a.Bottom64, b.Bottom64));
    }

    // Moved by the padding, inward or outward, into a new rectangle. The size can go negative, when the
    // padding is larger.
    public readonly PxRect__S__ Deflate(PxPadding__S__ p) =>
        new PxRect__S__(x + p.Left, y + p.Top, width - p.Horizontal, height - p.Vertical);
    public readonly PxRect__S__ Inflate(PxPadding__S__ p) =>
        new PxRect__S__(x - p.Left, y - p.Top, width + p.Horizontal, height + p.Vertical);

    // Moved by a vector, a displacement (8.1): the size stays. Putting it somewhere is setting Location.
    public static PxRect__S__ operator +(PxRect__S__ r, PxVec2__S__ v) => new PxRect__S__(r.x + v.X, r.y + v.Y, r.width, r.height);
    public static PxRect__S__ operator -(PxRect__S__ r, PxVec2__S__ v) => new PxRect__S__(r.x - v.X, r.y - v.Y, r.width, r.height);

    private readonly Wide Right64 => (Wide)x + width;
    private readonly Wide Bottom64 => (Wide)y + height;
    private readonly bool HasArea => width > 0 && height > 0;

    private static PxRect__S__ FromEdges(__T__ left, __T__ top, Wide right, Wide bottom) =>
        new PxRect__S__(left, top, (__T__)(right - left), (__T__)(bottom - top));

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3).
#if PX_DOUBLE
    public static implicit operator PxRect(PxRectf r) => new PxRect(r.X, r.Y, r.Width, r.Height);
    public static implicit operator PxRect(PxRecti r) => new PxRect(r.X, r.Y, r.Width, r.Height);
    public static explicit operator PxRectf(PxRect r) => new PxRectf((float)r.x, (float)r.y, (float)r.width, (float)r.height);
    public static explicit operator PxRecti(PxRect r) => new PxRecti(PxConvert.ToInt32(r.x), PxConvert.ToInt32(r.y), PxConvert.ToInt32(r.width), PxConvert.ToInt32(r.height));
#elif PX_FLOAT
    public static explicit operator PxRectf(PxRecti r) => new PxRectf(r.X, r.Y, r.Width, r.Height);
    public static explicit operator PxRecti(PxRectf r) => new PxRecti(PxConvert.ToInt32(r.x), PxConvert.ToInt32(r.y), PxConvert.ToInt32(r.width), PxConvert.ToInt32(r.height));
#endif

#if PX_DOUBLE
    // With System.Drawing (4.4): from it nothing is lost, so the conversion is implicit; back to it the
    // values are rounded (Rectangle) or narrowed (RectangleF), so it is explicit. Rectangle rounds the
    // edges, not the fields, so the width is what the rectangle covers (5.6): (0.5, 0, 0.5, 1) becomes
    // a width of 1, from 0 to 1, and not 0.
    public static implicit operator PxRect(System.Drawing.Rectangle r) => new PxRect(r.X, r.Y, r.Width, r.Height);
    public static implicit operator PxRect(System.Drawing.RectangleF r) => new PxRect(r.X, r.Y, r.Width, r.Height);
    public static explicit operator System.Drawing.Rectangle(PxRect r)
    {
        int left = PxConvert.ToInt32(Math.Round(r.x));
        int top = PxConvert.ToInt32(Math.Round(r.y));
        int right = PxConvert.ToInt32(Math.Round(r.x + r.width));
        int bottom = PxConvert.ToInt32(Math.Round(r.y + r.height));
        return new System.Drawing.Rectangle(left, top, right - left, bottom - top);
    }
    public static explicit operator System.Drawing.RectangleF(PxRect r) =>
        new System.Drawing.RectangleF((float)r.x, (float)r.y, (float)r.width, (float)r.height);
#endif

    public static bool operator ==(PxRect__S__ a, PxRect__S__ b) =>
        a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
    public static bool operator !=(PxRect__S__ a, PxRect__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the primitive works as
    // a key; == follows IEEE, where NaN differs from everything, as in C++ (5.5).
    public readonly bool Equals(PxRect__S__ other) =>
        x.Equals(other.x) && y.Equals(other.y) && width.Equals(other.width) && height.Equals(other.height);
    public override readonly bool Equals(object? obj) => obj is PxRect__S__ other && Equals(other);
    public override readonly int GetHashCode() =>
        PxHash.Combine(PxHash.Of(x), PxHash.Of(y), PxHash.Of(width), PxHash.Of(height));
    public override readonly string ToString() =>
        PxText.Tuple(PxText.Number(x), PxText.Number(y), PxText.Number(width), PxText.Number(height));
}
