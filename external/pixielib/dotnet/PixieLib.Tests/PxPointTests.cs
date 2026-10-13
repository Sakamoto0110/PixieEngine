namespace PixieLib.Tests;

internal static class PxPointTests
{
    public static void Run()
    {
        var pt = new PxPoint(1.5, -2);
        Check.That(pt.X == 1.5 && pt.Y == -2);
        Check.That(PxPoint.Empty.IsEmpty && new PxPoint().IsEmpty);
        Check.That(!pt.IsEmpty);

        Check.That(pt + new PxSize(2, 3) == new PxPoint(3.5, 1));
        Check.That(pt - new PxSize(2, 3) == new PxPoint(-0.5, -5));
        Check.That(+pt == pt);

        // A point and a vector (8.1): point - point is the vector between them, and a point plus or
        // minus a vector is a point.
        Check.That(new PxPoint(4, 6) - new PxPoint(1, 2) == new PxVec2(3, 4));
        Check.That((new PxPointi(4, 6) - new PxPointi(1, 2)).GetType() == typeof(PxVec2i));
        Check.That(pt + new PxVec2(1, 1) == new PxPoint(2.5, -1) && pt - new PxVec2(1, 1) == new PxPoint(0.5, -3));
        Check.That(new PxPoint(1, 2) + (new PxPoint(4, 6) - new PxPoint(1, 2)) == new PxPoint(4, 6));
        Check.That(new PxPointf(1, 2) + new PxVec2f(0.5f, 0.5f) == new PxPointf(1.5f, 2.5f));

        // What depends on the origin does not exist: point + point, point * k, point / k and -point.
        // The conversion to PxVec2 is explicit, so the vector's operators do not take a point either.
        Check.That(!Check.Operator<PxPoint, PxPoint>("op_Addition") && !Check.Operator<PxPointi, PxPointi>("op_Addition"));
        Check.That(!Check.Operator<PxPoint, double>("op_Multiply") && !Check.Operator<double, PxPoint>("op_Multiply"));
        Check.That(!Check.Operator<PxPoint, double>("op_Division") && !Check.Operator<PxPoint>("op_UnaryNegation"));
        Check.That(!Check.Operator<PxVec2, PxPoint>("op_Addition") && !Check.Implicit<PxPoint, PxVec2>());
        Check.That(Check.Operator<PxPoint, PxPoint>("op_Subtraction") && Check.Operator<PxPoint, PxVec2>("op_Addition"));

        // Lerp, glm::mix's formula: a * (1 - t) + b * t, the same as in C++.
        Check.That(PxPoint.Lerp(new PxPoint(0, 0), new PxPoint(10, 20), 0.25) == new PxPoint(2.5, 5));
        Check.That(PxPoint.Lerp(pt, new PxPoint(9, 9), 0) == pt && PxPoint.Lerp(pt, new PxPoint(9, 9), 1) == new PxPoint(9, 9));
        Check.That(PxPoint.Lerp(new PxPoint(0, 0), new PxPoint(10, 20), 2) == new PxPoint(20, 40));
        Check.That(PxPointf.Lerp(new PxPointf(7.6f, 0), new PxPointf(4.8f, 1), 0.22f).X == 6.9839993f);
        // In int, the amount is a double and the result is truncated, as glm::mix does with an ivec2; a
        // float amount would lose 2^24 + 1, a double does not.
        Check.That(PxPointi.Lerp(new PxPointi(1, -2), new PxPointi(4, -5), 0.5) == new PxPointi(2, -3));
        Check.That(PxPointi.Lerp(new PxPointi(0, 0), new PxPointi(3, -3), 0.5) == new PxPointi(1, -1));
        Check.That(PxPointi.Lerp(new PxPointi(16777217, 0), new PxPointi(16777217, 0), 0.5) == new PxPointi(16777217, 0));
        Check.That(PxPointi.Lerp(new PxPointi(0, 0), new PxPointi(int.MaxValue, int.MinValue), 2) == new PxPointi(int.MaxValue, int.MinValue));

        // A point and a size hold the same data; the conversion is explicit both ways.
        Check.That((PxSize)pt == new PxSize(1.5, -2));
        Check.That((PxPoint)new PxSize(3, 4) == new PxPoint(3, 4));
        Check.That(Check.Explicit<PxPoint, PxSize>() && Check.Explicit<PxSize, PxPoint>());
        Check.That(!Check.Implicit<PxPoint, PxSize>() && !Check.Implicit<PxSize, PxPoint>());

        // Between precisions: implicit when nothing is lost, explicit (truncating) otherwise.
        PxPoint fromFloat = new PxPointf(1.5f, 2.5f);
        PxPoint fromInt = new PxPointi(3, 4);
        Check.That(fromFloat == new PxPoint(1.5, 2.5));
        Check.That(fromInt == new PxPoint(3, 4));
        Check.That((PxPointi)new PxPoint(2.9, -2.9) == new PxPointi(2, -2));
        Check.That(Check.Implicit<PxPointf, PxPoint>() && Check.Implicit<PxPointi, PxPoint>());
        Check.That(!Check.Implicit<PxPoint, PxPointf>() && !Check.Implicit<PxPoint, PxPointi>());
        Check.That(!Check.Implicit<PxPointi, PxPointf>() && !Check.Implicit<PxPointf, PxPointi>());

        // Integer points wrap around, as in C++ (5.2), the difference too.
        var big = new PxPointi(int.MaxValue, int.MinValue);
        Check.That(big + new PxSizei(1, 0) == new PxPointi(int.MinValue, int.MinValue));
        Check.That(big - new PxSizei(-1, 1) == new PxPointi(int.MinValue, int.MaxValue));
        Check.That(big + new PxVec2i(1, -1) == new PxPointi(int.MinValue, int.MaxValue));
        Check.That(big - new PxVec2i(-1, 1) == new PxPointi(int.MinValue, int.MaxValue));
        Check.That(big - new PxPointi(-1, 1) == new PxVec2i(int.MinValue, int.MaxValue));

        // To int out of range: saturated, and NaN is 0 (5.3).
        Check.That((PxPointi)new PxPoint(3e9, -3e9) == new PxPointi(int.MaxValue, int.MinValue));
        Check.That((PxPointi)new PxPoint(double.NaN, -2147483648.9) == new PxPointi(0, int.MinValue));
        Check.That((PxPointi)new PxPointf(1e10f, -0.5f) == new PxPointi(int.MaxValue, 0));

        // Equals is reflexive with NaN, == is IEEE (5.5).
        var nan = new PxPoint(double.NaN, 0);
        var sameNan = nan;
        Check.That(nan.Equals(sameNan) && nan != sameNan);
        Check.That(new HashSet<PxPoint> { nan }.Contains(nan));
        Check.That(new PxPoint(0, -0.0).Equals(new PxPoint(0, 0)));
        Check.That(new PxPoint(0, -0.0).GetHashCode() == new PxPoint(0, 0).GetHashCode());
        Check.That(new PxPointf(float.NaN, 1).Equals(new PxPointf(float.NaN, 1)));
    }
}
