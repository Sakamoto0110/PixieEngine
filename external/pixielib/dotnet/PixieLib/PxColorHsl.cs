using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// A color in hue (0 to 360), saturation and lightness (0 to 1), in double, with the alpha in a byte,
// last, as in PxColorRgba (2.3). Same layout as pxColorHsl in C++ (docs/layout.md). There is no
// conversion to or from PxColorRgba, only the static functions (P8.4), and no hex: the text of an HSL
// is its fields (4.2).
[StructLayout(LayoutKind.Sequential)]
public struct PxColorHsl : IEquatable<PxColorHsl>
{
    public static readonly PxColorHsl Empty = default;

    private double h;
    private double s;
    private double l;
    private byte a;

    public PxColorHsl(double h, double s, double l, byte a = 255)
    {
        this.h = h;
        this.s = s;
        this.l = l;
        this.a = a;
    }

    public double H
    {
        readonly get => h;
        set => h = value;
    }

    public double S
    {
        readonly get => s;
        set => s = value;
    }

    public double L
    {
        readonly get => l;
        set => l = value;
    }

    public byte A
    {
        readonly get => a;
        set => a = value;
    }

    public readonly bool IsEmpty => h == 0 && s == 0 && l == 0 && a == 0;

    public static PxColorHsl FromRgba(PxColorRgba color)
    {
        double r = color.R / 255.0;
        double g = color.G / 255.0;
        double b = color.B / 255.0;
        double max = Math.Max(r, Math.Max(g, b));
        double min = Math.Min(r, Math.Min(g, b));
        double delta = max - min;
        double lightness = (max + min) / 2;
        if (delta == 0)
            return new PxColorHsl(0, 0, lightness, color.A);

        double saturation = delta / (1 - Math.Abs(2 * lightness - 1));
        double hue;
        if (max == r)
            hue = 60 * (((g - b) / delta) % 6);
        else if (max == g)
            hue = 60 * (((b - r) / delta) + 2);
        else
            hue = 60 * (((r - g) / delta) + 4);
        if (hue < 0)
            hue += 360;
        return new PxColorHsl(hue, saturation, lightness, color.A);
    }

    public static PxColorRgba ToRgba(PxColorHsl color)
    {
        double hue = color.h % 360;
        if (hue < 0)
            hue += 360;
        double chroma = (1 - Math.Abs(2 * color.l - 1)) * color.s;
        double x = chroma * (1 - Math.Abs((hue / 60) % 2 - 1));
        double m = color.l - chroma / 2;

        double r, g, b;
        if (hue < 60)       { r = chroma; g = x;      b = 0; }
        else if (hue < 120) { r = x;      g = chroma; b = 0; }
        else if (hue < 180) { r = 0;      g = chroma; b = x; }
        else if (hue < 240) { r = 0;      g = x;      b = chroma; }
        else if (hue < 300) { r = x;      g = 0;      b = chroma; }
        else                { r = chroma; g = 0;      b = x; }
        return new PxColorRgba(ToByte(r + m), ToByte(g + m), ToByte(b + m), color.a);
    }

    // Math.Round takes a half to the even neighbor (126.5 to 126), like std::nearbyint in C++. Out of
    // range values, which only come from a saturation or lightness outside 0 to 1, are clamped (2.8),
    // and NaN is 0 (5.3).
    private static byte ToByte(double unit)
    {
        if (double.IsNaN(unit))
            return 0;
        double value = unit * 255;
        value = value < 0 ? 0 : value > 255 ? 255 : value;
        return (byte)Math.Round(value);
    }

    public static bool operator ==(PxColorHsl x, PxColorHsl y) => x.h == y.h && x.s == y.s && x.l == y.l && x.a == y.a;
    public static bool operator !=(PxColorHsl x, PxColorHsl y) => !(x == y);

    // Equals compares each field with its own Equals, so a NaN equals itself; == follows IEEE (5.5).
    public readonly bool Equals(PxColorHsl other) => h.Equals(other.h) && s.Equals(other.s) && l.Equals(other.l) && a == other.a;
    public override readonly bool Equals(object? obj) => obj is PxColorHsl other && Equals(other);
    public override readonly int GetHashCode() => PxHash.Combine(PxHash.Of(h), PxHash.Of(s), PxHash.Of(l), a);
    public override readonly string ToString() =>
        PxText.Tuple(PxText.Number(h), PxText.Number(s), PxText.Number(l), PxText.Number(a));
}
