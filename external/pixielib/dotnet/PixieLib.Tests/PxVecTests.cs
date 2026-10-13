using System.Numerics;

namespace PixieLib.Tests;

// The same values as cpp/tests/pxVec.test.cpp, where the math is GLM's.
internal static class PxVecTests
{
    public static void Run()
    {
        Check.That(new PxVec3() == PxVec3.Zero && PxVec4i.Zero == new PxVec4i(0));
        Check.That(PxVec3.One == new PxVec3(1, 1, 1) && PxVec3.UnitZ == new PxVec3(0, 0, 1));
        Check.That(new PxVec3(new PxVec2(1, 2), 3) == new PxVec3(1, 2, 3));
        Check.That(new PxVec4(new PxVec3(1, 2, 3), 4).W == 4);

        var a = new PxVec3(1, 2, 3);
        var b = new PxVec3(4, 5, 6);
        Check.That(a + b == new PxVec3(5, 7, 9));
        Check.That(b - a == new PxVec3(3, 3, 3));
        Check.That(a * b == new PxVec3(4, 10, 18));
        Check.That(a * 2 == new PxVec3(2, 4, 6) && 2 * a == new PxVec3(2, 4, 6));
        Check.That(b / 2 == new PxVec3(2, 2.5, 3));
        Check.That(-a == new PxVec3(-1, -2, -3));

        // The values of the C++ tests, in double (own code, GLM's formulas) and in float (System.Numerics).
        Check.That(PxVec3.Dot(a, b) == 32 && PxVec3f.Dot(new PxVec3f(1, 2, 3), new PxVec3f(4, 5, 6)) == 32);
        Check.That(PxVec3.Cross(PxVec3.UnitX, PxVec3.UnitY) == PxVec3.UnitZ);
        Check.That(PxVec3.Cross(a, b) == new PxVec3(-3, 6, -3));
        Check.That(PxVec3f.Cross(new PxVec3f(1, 2, 3), new PxVec3f(4, 5, 6)) == new PxVec3f(-3, 6, -3));
        Check.That(new PxVec3(3, 4, 0).Length() == 5 && PxVec3.Distance(a, b) == (b - a).Length());
        Check.That(new PxVec3f(3, 4, 0).Length() == 5 && new PxVec3f(3, 4, 0).LengthSquared() == 25);
        Check.That((PxVec3.Normalize(new PxVec3(3, 4, 0)) - new PxVec3(0.6, 0.8, 0)).Length() < 1e-15);
        Check.That(PxVec3.Normalize(new PxVec3(0, 0, 2)) == PxVec3.UnitZ);
        Check.That(PxVec3.Lerp(PxVec3.Zero, new PxVec3(10, 20, 30), 0.25) == new PxVec3(2.5, 5, 7.5));
        Check.That(PxVec3f.Lerp(PxVec3f.Zero, new PxVec3f(10, 20, 30), 0.25f) == new PxVec3f(2.5f, 5, 7.5f));
        Check.That(PxVec3.Min(a, new PxVec3(2, 1, 3)) == new PxVec3(1, 1, 3) && PxVec3.Max(a, new PxVec3(2, 1, 3)) == new PxVec3(2, 2, 3));
        Check.That(PxVec3.Min(new PxVec3(3, 2, 1), new PxVec3(1, 2, 3)) == new PxVec3(1, 2, 1));
        Check.That(PxVec3.Max(new PxVec3(3, 2, 1), new PxVec3(1, 2, 3)) == new PxVec3(3, 2, 3));
        Check.That(PxVec3i.Min(new PxVec3i(3, 2, 1), new PxVec3i(1, 2, 3)) == new PxVec3i(1, 2, 1));
        Check.That(PxVec3.Clamp(new PxVec3(-1, 5, 2), PxVec3.Zero, new PxVec3(3)) == new PxVec3(0, 3, 2));
        Check.That(PxVec3f.Clamp(new PxVec3f(-1, 5, 2), PxVec3f.Zero, new PxVec3f(3)) == new PxVec3f(0, 3, 2));
        Check.That(PxVec3i.Abs(new PxVec3i(-1, 0, 2)) == new PxVec3i(1, 0, 2));
        Check.That(new PxVec2f(1, 2) + new PxVec2f(3, 4) == new PxVec2f(4, 6));
        Check.That(PxVec4.Dot(new PxVec4(1, 2, 3, 4), PxVec4.One) == 10);

        // A point and a vec2 convert to each other, explicitly, in the same precision (6.6).
        Check.That((PxVec2)new PxPoint(1.5, -2) == new PxVec2(1.5, -2) && (PxPointi)new PxVec2i(3, 4) == new PxPointi(3, 4));
        Check.That((PxPointf)(PxVec2f)new PxPointf(0.1f, 7) == new PxPointf(0.1f, 7));
        Check.That(Check.Explicit<PxPoint, PxVec2>() && !Check.Implicit<PxPoint, PxVec2>() && Check.Explicit<PxVec2i, PxPointi>());

        // The length of an integer vec2, which GLM refuses (8.3): in double, and squared in long. The
        // same values as the C++ tests of pxLength and the others.
        Check.That(new PxVec2i(3, 4).Length() == 5 && new PxVec2i(3, 4).LengthSquared() == 25);
        Check.That(new PxVec2i(3, 1).Length() == 3.1622776601683795 && new PxVec2i(3, -1).LengthSquared() == 10);
        // 64 bits, where int would wrap: the square of int.MinValue is 2^62.
        Check.That(new PxVec2i(int.MinValue, 0).LengthSquared() == 4611686018427387904);
        // The one length whose square passes long.MaxValue: 2^63 wraps around (5.2), the length does not.
        Check.That(new PxVec2i(int.MinValue, int.MinValue).LengthSquared() == long.MinValue);
        Check.That(new PxVec2i(int.MinValue, int.MinValue).Length() == 3037000499.97605);
        // The distance takes the difference in 64 bits: from int.MinValue to int.MaxValue is 2^32 - 1,
        // which int would wrap to -1. Its square passes long.MaxValue and wraps.
        Check.That(PxVec2i.Distance(new PxVec2i(int.MinValue, 0), new PxVec2i(int.MaxValue, 0)) == 4294967295);
        Check.That(PxVec2i.DistanceSquared(new PxVec2i(int.MinValue, 0), new PxVec2i(int.MaxValue, 0)) == -8589934591);
        Check.That(PxVec2i.Distance(new PxVec2i(1, 1), new PxVec2i(4, 5)) == 5 && PxVec2i.DistanceSquared(new PxVec2i(4, 5), new PxVec2i(1, 1)) == 25);
        // Beyond 2^53 each square rounds in double: 94906267^2 is odd and above it.
        Check.That(new PxVec2i(94906267, 0).Length() == 94906267);
        Check.That(PxVec2i.Distance(PxVec2i.Zero, new PxVec2i(-123456789, 987654321)) == 995340462.62581);
        // And the root of the double sum, not of the exact one, which would be 2392612263.308584.
        Check.That(new PxVec2i(1721854523, -1661267722).Length() == 2392612263.3085847);

        // The dot product of a vec4 adds in pairs, (x + y) + (z + w), as GLM does; with 1e16, adding 1
        // is lost, and adding in sequence would give 1.
        Check.That(PxVec4.Dot(new PxVec4(1e16, 1, -1e16, 1), PxVec4.One) == 0);
        Check.That(PxVec4f.Dot(new PxVec4f(1e8f, 1, -1e8f, 1), PxVec4f.One) == 0);

        // The double ones give GLM's bits: Normalize multiplies by 1 / length, as glm::normalize.
        var n = PxVec3.Normalize(new PxVec3(3, 4, 0));
        Check.That(n.X == 3 * (1 / 5.0) && n.X != 0.6);

        // And so do the float ones, where System.Numerics alone would be one bit off: Vector2.Normalize
        // gives 0.8087361 and Vector2.Lerp 6.9839997. The C++ tests expect these same bits.
        Check.That(PxVec2f.Normalize(new PxVec2f(11, 8)).X == 0.808736f);
        Check.That(PxVec2f.Lerp(new PxVec2f(7.6f), new PxVec2f(4.8f), 0.22f).X == 6.9839993f);

        // Between precisions: implicit when nothing is lost, explicit otherwise; to int it truncates,
        // saturates and takes NaN to 0 (5.3).
        PxVec3 fromFloat = new PxVec3f(1.5f, 2, 3);
        Check.That(fromFloat == new PxVec3(1.5, 2, 3));
        Check.That((PxVec3i)new PxVec3(2.9, -2.9, 0.5) == new PxVec3i(2, -2, 0));
        Check.That((PxVec3i)new PxVec3(3e9, double.NaN, -3e9) == new PxVec3i(int.MaxValue, 0, int.MinValue));
        Check.That(Check.Implicit<PxVec3f, PxVec3>() && Check.Implicit<PxVec3i, PxVec3>());
        Check.That(Check.Explicit<PxVec3, PxVec3f>() && Check.Explicit<PxVec3, PxVec3i>() && !Check.Implicit<PxVec3, PxVec3i>());

        // Integer vectors wrap around (5.2).
        Check.That(new PxVec3i(int.MaxValue, 0, 0) + PxVec3i.UnitX == new PxVec3i(int.MinValue, 0, 0));

        // With System.Numerics, both ways.
        Vector3 sn = new PxVec3f(1, 2, 3);
        PxVec3f back = sn;
        Check.That(sn == new Vector3(1, 2, 3) && back == new PxVec3f(1, 2, 3));
        Check.That(Check.Implicit<PxVec4f, Vector4>() && Check.Implicit<Vector2, PxVec2f>());

        // Equals is reflexive with NaN, == is IEEE (5.5).
        var nan = new PxVec3(double.NaN, 0, 0);
        var sameNan = nan;
        Check.That(nan.Equals(sameNan) && nan != sameNan);
        Check.That(new HashSet<PxVec4f> { new PxVec4f(float.NaN) }.Contains(new PxVec4f(float.NaN)));

        // The text, the same as the primitives' and as pxToString in C++.
        Check.That(new PxVec3(1.5, -2, 0).ToString() == "(1.5, -2, 0)");
        Check.That(new PxVec2f(0.1f, 1e20f).ToString() == "(0.1, 100000000000000000000)");
        Check.That(new PxVec4i(1, -2, 3, int.MaxValue).ToString() == "(1, -2, 3, 2147483647)");
    }
}
