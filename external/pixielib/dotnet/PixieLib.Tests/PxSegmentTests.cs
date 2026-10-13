namespace PixieLib.Tests;

// The same values as cpp/tests/pxSegment.test.cpp.
internal static class PxSegmentTests
{
    public static void Run()
    {
        var s = new PxSegment(new PxPoint(1, 2), new PxPoint(4, 6));
        Check.That(s.Start == new PxPoint(1, 2) && s.End == new PxPoint(4, 6));
        Check.That(new PxSegment() == new PxSegment(new PxPoint(), new PxPoint()));

        // The direction is End - Start, not normalized: its length is the segment's.
        Check.That(s.Direction() == new PxVec2(3, 4));
        Check.That(s.Length() == 5 && s.Direction().Length() == s.Length());
        Check.That(new PxSegmentf(new PxPointf(0, 0), new PxPointf(3, 4)).Length() == 5);

        // PointAt is the Lerp of the two ends, glm::mix's formula.
        Check.That(s.PointAt(0) == s.Start && s.PointAt(1) == s.End);
        Check.That(s.PointAt(0.5) == new PxPoint(2.5, 4) && s.PointAt(2) == new PxPoint(7, 10));
        Check.That(new PxSegmentf(new PxPointf(7.6f, 0), new PxPointf(4.8f, 1)).PointAt(0.22f).X == 6.9839993f);

        // In int, what GLM does with an ivec2 (docs/notas.md, 4.17): PointAt takes a double and
        // truncates, as glm::mix, and the length is a double (8.3).
        var si = new PxSegmenti(new PxPointi(1, -2), new PxPointi(4, -5));
        Check.That(si.Direction() == new PxVec2i(3, -3));
        Check.That(si.PointAt(0.5) == new PxPointi(2, -3));
        Check.That(si.Length() == 4.242640687119285);
        // The length takes the difference in 64 bits; the direction wraps around, as a point does (5.2).
        var far = new PxSegmenti(new PxPointi(int.MinValue, 0), new PxPointi(int.MaxValue, 0));
        Check.That(far.Length() == 4294967295 && far.Direction() == new PxVec2i(-1, 0));

        // Between precisions: implicit when nothing is lost, explicit (truncating) otherwise.
        PxSegment fromInt = si;
        Check.That(fromInt == new PxSegment(new PxPoint(1, -2), new PxPoint(4, -5)));
        Check.That((PxSegmenti)new PxSegment(new PxPoint(2.9, -2.9), new PxPoint(3e9, 0)) ==
            new PxSegmenti(new PxPointi(2, -2), new PxPointi(int.MaxValue, 0)));
        Check.That(Check.Implicit<PxSegmentf, PxSegment>() && Check.Implicit<PxSegmenti, PxSegment>());
        Check.That(Check.Explicit<PxSegment, PxSegmentf>() && Check.Explicit<PxSegmentf, PxSegmenti>());
        Check.That(!Check.Implicit<PxSegment, PxSegmentf>() && !Check.Implicit<PxSegmentf, PxSegmenti>());

        // The text: the two points, in the order of the fields.
        Check.That(s.ToString() == "(1, 2, 4, 6)");
        Check.That(new PxSegmentf(new PxPointf(0.1f, -1.5f), new PxPointf(1e20f, 0)).ToString() == "(0.1, -1.5, 100000000000000000000, 0)");

        // Equals is reflexive with NaN, == is IEEE (5.5).
        var nan = new PxSegment(new PxPoint(double.NaN, 0), new PxPoint());
        var sameNan = nan;
        Check.That(nan.Equals(sameNan) && nan != sameNan);
        Check.That(new HashSet<PxSegment> { nan }.Contains(nan));
        Check.That(new PxSegment(new PxPoint(0, -0.0), new PxPoint()).GetHashCode() == new PxSegment().GetHashCode());
    }
}
