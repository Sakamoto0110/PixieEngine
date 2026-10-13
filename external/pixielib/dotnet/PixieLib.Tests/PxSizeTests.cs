namespace PixieLib.Tests;

internal static class PxSizeTests
{
    public static void Run()
    {
        var sz = new PxSize(3, 4.5);
        Check.That(sz.Width == 3 && sz.Height == 4.5);
        Check.That(PxSize.Empty.IsEmpty && new PxSize().IsEmpty);
        Check.That(!sz.IsEmpty);

        Check.That(sz + new PxSize(1, 1) == new PxSize(4, 5.5));
        Check.That(sz - new PxSize(1, 1) == new PxSize(2, 3.5));
        Check.That(sz * 2 == new PxSize(6, 9));
        Check.That(2 * sz == new PxSize(6, 9));
        Check.That(sz / 2 == new PxSize(1.5, 2.25));
        Check.That(+sz == sz);
        Check.That(-sz == new PxSize(-3, -4.5));

        // Between precisions: implicit when nothing is lost, explicit (truncating) otherwise.
        PxSize fromFloat = new PxSizef(1.5f, 2.5f);
        PxSize fromInt = new PxSizei(3, 4);
        Check.That(fromFloat == new PxSize(1.5, 2.5));
        Check.That(fromInt == new PxSize(3, 4));
        Check.That((PxSizei)new PxSize(2.9, -2.9) == new PxSizei(2, -2));
        Check.That(Check.Implicit<PxSizef, PxSize>() && Check.Implicit<PxSizei, PxSize>());
        Check.That(Check.Explicit<PxSize, PxSizef>() && Check.Explicit<PxSize, PxSizei>());
        Check.That(Check.Explicit<PxSizei, PxSizef>() && Check.Explicit<PxSizef, PxSizei>());
        Check.That(!Check.Implicit<PxSize, PxSizef>() && !Check.Implicit<PxSizei, PxSizef>());

        // The same rules as PxPoint (5.2, 5.3, 5.5).
        Check.That(new PxSizei(int.MaxValue, 1) + new PxSizei(1, 1) == new PxSizei(int.MinValue, 2));
        Check.That(-new PxSizei(int.MinValue, 0) == new PxSizei(int.MinValue, 0));
        Check.That((PxSizei)new PxSize(-1e300, double.NaN) == new PxSizei(int.MinValue, 0));
        Check.That(new PxSize(double.NaN, 1).Equals(new PxSize(double.NaN, 1)));
    }
}
