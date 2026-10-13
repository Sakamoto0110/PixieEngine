namespace PixieLib.Tests;

internal static class PxPaddingTests
{
    public static void Run()
    {
        var p = new PxPadding(1, 2, 3, 4);
        Check.That(p.Left == 1 && p.Top == 2 && p.Right == 3 && p.Bottom == 4);
        Check.That(p.Horizontal == 4 && p.Vertical == 6);
        Check.That(new PxPadding(5) == new PxPadding(5, 5, 5, 5));
        Check.That(PxPadding.Empty.IsEmpty);
        Check.That(!p.IsEmpty);

        // A single value does not become a padding by accident.
        Check.That(!Check.Implicit<double, PxPadding>());

        PxPadding fromInt = new PxPaddingi(1, 2, 3, 4);
        Check.That(fromInt == p);
        Check.That(Check.Implicit<PxPaddingf, PxPadding>() && !Check.Implicit<PxPadding, PxPaddingf>());

        // The sums wrap around in int (5.2), and NaN equals itself in Equals (5.5).
        Check.That(new PxPaddingi(int.MaxValue, 0, 1, 0).Horizontal == int.MinValue);
        Check.That(new PxPadding(double.NaN).Equals(new PxPadding(double.NaN)));
    }
}
