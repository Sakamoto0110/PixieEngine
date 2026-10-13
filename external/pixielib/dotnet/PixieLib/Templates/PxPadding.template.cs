using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// Space around something, one value per side, in __T__, in the same order as PxRegion: left, top,
// right, bottom. Same layout as pxPadding_t in C++ (docs/layout.md).
[StructLayout(LayoutKind.Sequential)]
public struct PxPadding__S__ : IEquatable<PxPadding__S__>
{
    public static readonly PxPadding__S__ Empty = default;

    private __T__ left;
    private __T__ top;
    private __T__ right;
    private __T__ bottom;

    public PxPadding__S__(__T__ all) : this(all, all, all, all)
    {
    }

    public PxPadding__S__(__T__ left, __T__ top, __T__ right, __T__ bottom)
    {
        this.left = left;
        this.top = top;
        this.right = right;
        this.bottom = bottom;
    }

    public __T__ Left
    {
        readonly get => left;
        set => left = value;
    }

    public __T__ Top
    {
        readonly get => top;
        set => top = value;
    }

    public __T__ Right
    {
        readonly get => right;
        set => right = value;
    }

    public __T__ Bottom
    {
        readonly get => bottom;
        set => bottom = value;
    }

    // The space taken across and along. In int the sum wraps around (5.2).
    public readonly __T__ Horizontal => left + right;
    public readonly __T__ Vertical => top + bottom;

    public readonly bool IsEmpty => left == 0 && top == 0 && right == 0 && bottom == 0;

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3).
#if PX_DOUBLE
    public static implicit operator PxPadding(PxPaddingf p) => new PxPadding(p.Left, p.Top, p.Right, p.Bottom);
    public static implicit operator PxPadding(PxPaddingi p) => new PxPadding(p.Left, p.Top, p.Right, p.Bottom);
    public static explicit operator PxPaddingf(PxPadding p) => new PxPaddingf((float)p.left, (float)p.top, (float)p.right, (float)p.bottom);
    public static explicit operator PxPaddingi(PxPadding p) => new PxPaddingi(PxConvert.ToInt32(p.left), PxConvert.ToInt32(p.top), PxConvert.ToInt32(p.right), PxConvert.ToInt32(p.bottom));
#elif PX_FLOAT
    public static explicit operator PxPaddingf(PxPaddingi p) => new PxPaddingf(p.Left, p.Top, p.Right, p.Bottom);
    public static explicit operator PxPaddingi(PxPaddingf p) => new PxPaddingi(PxConvert.ToInt32(p.left), PxConvert.ToInt32(p.top), PxConvert.ToInt32(p.right), PxConvert.ToInt32(p.bottom));
#endif

    public static bool operator ==(PxPadding__S__ a, PxPadding__S__ b) =>
        a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
    public static bool operator !=(PxPadding__S__ a, PxPadding__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the primitive works as
    // a key; == follows IEEE, where NaN differs from everything, as in C++ (5.5).
    public readonly bool Equals(PxPadding__S__ other) =>
        left.Equals(other.left) && top.Equals(other.top) && right.Equals(other.right) && bottom.Equals(other.bottom);
    public override readonly bool Equals(object? obj) => obj is PxPadding__S__ other && Equals(other);
    public override readonly int GetHashCode() =>
        PxHash.Combine(PxHash.Of(left), PxHash.Of(top), PxHash.Of(right), PxHash.Of(bottom));
    public override readonly string ToString() =>
        PxText.Tuple(PxText.Number(left), PxText.Number(top), PxText.Number(right), PxText.Number(bottom));
}
