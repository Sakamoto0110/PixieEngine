using System;
using System.Runtime.InteropServices;

namespace PixieLib;

// A segment of a line, from Start to End, in __T__. Same layout as pxSegment_t in C++
// (docs/layout.md): the two points, one after the other. For debug drawing, gizmos, edges, collision,
// and the lines cast with a maximum distance, as Box2D's RayCast with its two points
// (docs/notas.md, 4.17). There is no ray: a ray with a length is a segment. In int it does what GLM
// does with an ivec2: PointAt truncates, as glm::mix, and the length comes in double (8.3).
[StructLayout(LayoutKind.Sequential)]
public struct PxSegment__S__ : IEquatable<PxSegment__S__>
{
    private PxPoint__S__ start;
    private PxPoint__S__ end;

    public PxSegment__S__(PxPoint__S__ start, PxPoint__S__ end)
    {
        this.start = start;
        this.end = end;
    }

    public PxPoint__S__ Start
    {
        readonly get => start;
        set => start = value;
    }

    public PxPoint__S__ End
    {
        readonly get => end;
        set => end = value;
    }

    // End - Start, not normalized: its length is the segment's. In int it wraps around, as the rest of
    // the point (5.2); Length does not.
    public readonly PxVec2__S__ Direction() => end - start;

    // The point a fraction amount of the way, the Lerp of the two ends: Start at 0, End at 1.
#if PX_INT
    public readonly PxPointi PointAt(double amount) => PxPointi.Lerp(start, end, amount);

    // PxVec2i.Distance, with the difference in 64 bits; a double, since it is seldom an integer.
    public readonly double Length() => PxVec2i.Distance((PxVec2i)start, (PxVec2i)end);
#else
    public readonly PxPoint__S__ PointAt(__T__ amount) => PxPoint__S__.Lerp(start, end, amount);

    // glm::distance, as PxVec2.Distance.
    public readonly __T__ Length() => PxVec2__S__.Distance((PxVec2__S__)start, (PxVec2__S__)end);
#endif

    // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
    // saturates out of range and takes NaN to 0 (5.3).
#if PX_DOUBLE
    public static implicit operator PxSegment(PxSegmentf s) => new PxSegment(s.Start, s.End);
    public static implicit operator PxSegment(PxSegmenti s) => new PxSegment(s.Start, s.End);
    public static explicit operator PxSegmentf(PxSegment s) => new PxSegmentf((PxPointf)s.start, (PxPointf)s.end);
    public static explicit operator PxSegmenti(PxSegment s) => new PxSegmenti((PxPointi)s.start, (PxPointi)s.end);
#elif PX_FLOAT
    public static explicit operator PxSegmentf(PxSegmenti s) => new PxSegmentf((PxPointf)s.Start, (PxPointf)s.End);
    public static explicit operator PxSegmenti(PxSegmentf s) => new PxSegmenti((PxPointi)s.start, (PxPointi)s.end);
#endif

    public static bool operator ==(PxSegment__S__ a, PxSegment__S__ b) => a.start == b.start && a.end == b.end;
    public static bool operator !=(PxSegment__S__ a, PxSegment__S__ b) => !(a == b);

    // Equals compares each field with its own Equals, so a NaN equals itself and the segment works as a
    // key; == follows IEEE, as in C++ (5.5).
    public readonly bool Equals(PxSegment__S__ other) => start.Equals(other.start) && end.Equals(other.end);
    public override readonly bool Equals(object? obj) => obj is PxSegment__S__ other && Equals(other);
    public override readonly int GetHashCode() =>
        PxHash.Combine(PxHash.Of(start.X), PxHash.Of(start.Y), PxHash.Of(end.X), PxHash.Of(end.Y));
    public override readonly string ToString() =>
        PxText.Tuple(PxText.Number(start.X), PxText.Number(start.Y), PxText.Number(end.X), PxText.Number(end.Y));
}
