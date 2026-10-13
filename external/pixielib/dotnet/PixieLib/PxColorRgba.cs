using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// A color in bytes, in memory as R, G, B, A: the order of GL_RGBA with GL_UNSIGNED_BYTE, the same
// layout as pxColorRgba in C++ (docs/layout.md). The order in memory and the order of a hex number are
// not the same thing, so the hex conversions shift bits: FromHex and ToHex use 0xRRGGBBAA, the number
// as it is written (2.4); FromArgb and ToArgb use the 0xAARRGGBB of System.Drawing.
[StructLayout(LayoutKind.Sequential)]
public struct PxColorRgba : IEquatable<PxColorRgba>
{
    public static readonly PxColorRgba Empty = default;

    private byte r;
    private byte g;
    private byte b;
    private byte a;

    public PxColorRgba(byte r, byte g, byte b, byte a = 255)
    {
        this.r = r;
        this.g = g;
        this.b = b;
        this.a = a;
    }

    public byte R
    {
        readonly get => r;
        set => r = value;
    }

    public byte G
    {
        readonly get => g;
        set => g = value;
    }

    public byte B
    {
        readonly get => b;
        set => b = value;
    }

    public byte A
    {
        readonly get => a;
        set => a = value;
    }

    public readonly bool IsEmpty => r == 0 && g == 0 && b == 0 && a == 0;

    public static PxColorRgba FromHex(uint rrggbbaa) =>
        new PxColorRgba((byte)(rrggbbaa >> 24), (byte)(rrggbbaa >> 16), (byte)(rrggbbaa >> 8), (byte)rrggbbaa);

    public readonly uint ToHex() => ((uint)r << 24) | ((uint)g << 16) | ((uint)b << 8) | a;

    public static PxColorRgba FromArgb(int argb) =>
        new PxColorRgba((byte)(argb >> 16), (byte)(argb >> 8), (byte)argb, (byte)(argb >> 24));

    public readonly int ToArgb() => (a << 24) | (r << 16) | (g << 8) | b;

    // Nothing is lost either way, so both are implicit.
    public static implicit operator System.Drawing.Color(PxColorRgba c) => System.Drawing.Color.FromArgb(c.a, c.r, c.g, c.b);
    public static implicit operator PxColorRgba(System.Drawing.Color c) => new PxColorRgba(c.R, c.G, c.B, c.A);

    public static bool operator ==(PxColorRgba x, PxColorRgba y) => x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a;
    public static bool operator !=(PxColorRgba x, PxColorRgba y) => !(x == y);

    public readonly bool Equals(PxColorRgba other) => this == other;
    public override readonly bool Equals(object? obj) => obj is PxColorRgba other && Equals(other);
    public override readonly int GetHashCode() => unchecked((int)ToHex());
    public override readonly string ToString() =>
        PxText.Tuple(PxText.Number(r), PxText.Number(g), PxText.Number(b), PxText.Number(a));
}
