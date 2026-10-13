using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// A width and a height, in __T__. Same layout as pxSize_t in C++ (docs/layout.md).
[StructLayout(LayoutKind.Sequential)]
public struct PxSize__S__ : IEquatable<PxSize__S__>
{
    public static readonly PxSize__S__ Empty = default;

    private __T__ width;
    private __T__ height;

    public PxSize__S__(__T__ width, __T__ height)
    {
        this.width = width;
        this.height = height;
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

    public readonly bool IsEmpty => width == 0 && height == 0;

    // In int the arithmetic wraps around, as in C++ (5.2), and dividing by zero throws (5.4).
    public static PxSize__S__ operator +(PxSize__S__ a, PxSize__S__ b) => new PxSize__S__(a.width + b.width, a.height + b.height);
    public static PxSize__S__ operator -(PxSize__S__ a, PxSize__S__ b) => new PxSize__S__(a.width - b.width, a.height - b.height);
    public static PxSize__S__ operator *(PxSize__S__ size, __T__ k) => new PxSize__S__(size.width * k, size.height * k);
    public static PxSize__S__ operator *(__T__ k, PxSize__S__ size) => size * k;
    public static PxSize__S__ operator /(PxSize__S__ size, __T__ k) => new PxSize__S__(size.width / k, size.height / k);
    public static PxSize__S__ operator +(PxSize__S__ size) => size;
    public static PxSize__S__ operator -(PxSize__S__ size) => new PxSize__S__(-size.width, -size.height);

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3).
#if PX_DOUBLE
    public static implicit operator PxSize(PxSizef size) => new PxSize(size.Width, size.Height);
    public static implicit operator PxSize(PxSizei size) => new PxSize(size.Width, size.Height);
    public static explicit operator PxSizef(PxSize size) => new PxSizef((float)size.width, (float)size.height);
    public static explicit operator PxSizei(PxSize size) => new PxSizei(PxConvert.ToInt32(size.width), PxConvert.ToInt32(size.height));
#elif PX_FLOAT
    public static explicit operator PxSizef(PxSizei size) => new PxSizef(size.Width, size.Height);
    public static explicit operator PxSizei(PxSizef size) => new PxSizei(PxConvert.ToInt32(size.width), PxConvert.ToInt32(size.height));
#endif

#if PX_DOUBLE
    // With System.Drawing (4.4): from it nothing is lost, so the conversion is implicit; back to it the
    // value is rounded (Size) or narrowed (SizeF), so it is explicit.
    public static implicit operator PxSize(System.Drawing.Size size) => new PxSize(size.Width, size.Height);
    public static implicit operator PxSize(System.Drawing.SizeF size) => new PxSize(size.Width, size.Height);
    public static explicit operator System.Drawing.Size(PxSize size) =>
        new System.Drawing.Size(PxConvert.ToInt32(Math.Round(size.width)), PxConvert.ToInt32(Math.Round(size.height)));
    public static explicit operator System.Drawing.SizeF(PxSize size) => new System.Drawing.SizeF((float)size.width, (float)size.height);
#endif

    public static bool operator ==(PxSize__S__ a, PxSize__S__ b) => a.width == b.width && a.height == b.height;
    public static bool operator !=(PxSize__S__ a, PxSize__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the primitive works as
    // a key; == follows IEEE, where NaN differs from everything, as in C++ (5.5).
    public readonly bool Equals(PxSize__S__ other) => width.Equals(other.width) && height.Equals(other.height);
    public override readonly bool Equals(object? obj) => obj is PxSize__S__ other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(PxHash.Of(width), PxHash.Of(height));
    public override readonly string ToString() => PxText.Tuple(PxText.Number(width), PxText.Number(height));
}
