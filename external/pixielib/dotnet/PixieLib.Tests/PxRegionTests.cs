namespace PixieLib.Tests;

internal static class PxRegionTests
{
    public static void Run()
    {
        var rg = new PxRegion(1, 2, 4, 6);
        Check.That(rg.Width == 3 && rg.Height == 4);
        Check.That(PxRegion.Empty.IsEmpty);
        Check.That(!rg.IsEmpty);

        // The same thing as a rectangle, converted explicitly both ways.
        Check.That((PxRegion)new PxRect(1, 2, 3, 4) == rg);
        Check.That((PxRect)rg == new PxRect(1, 2, 3, 4));
        Check.That(!Check.Implicit<PxRect, PxRegion>() && !Check.Implicit<PxRegion, PxRect>());

        // The same edges as PxRect.Contains.
        var r = new PxRect(1, 2, 3, 4);
        foreach (var pt in new[] { new PxPoint(1, 2), new PxPoint(3.999, 5.999), new PxPoint(4, 2), new PxPoint(1, 6), new PxPoint(0.5, 3) })
            Check.That(rg.Contains(pt) == r.Contains(pt), $"Contains{pt} agrees with PxRect");
        Check.That(rg.Contains(new PxPoint(1, 2)) && !rg.Contains(new PxPoint(4, 2)) && !rg.Contains(new PxPoint(1, 6)));

        PxRegion fromInt = new PxRegioni(0, 0, 2, 2);
        Check.That(fromInt == new PxRegion(0, 0, 2, 2));
        Check.That(Check.Implicit<PxRegioni, PxRegion>() && !Check.Implicit<PxRegion, PxRegioni>());

        // The operations of PxRect, with the same rules.
        var a = new PxRegion(0, 0, 4, 4);
        Check.That(a.Contains(a) && a.Contains(new PxRegion(1, 1, 2, 2)) && a.Contains(new PxRegion(4, 4, 4, 4)));
        Check.That(!a.Contains(new PxRegion(1, 1, 5, 2)));
        Check.That(PxRegion.Intersect(a, new PxRegion(2, 1, 6, 3)) == new PxRegion(2, 1, 4, 3));
        Check.That(!a.IntersectsWith(new PxRegion(4, 0, 5, 4)) && PxRegion.Intersect(a, new PxRegion(4, 0, 5, 4)).IsEmpty);
        Check.That(!a.IntersectsWith(new PxRegion(3, 3, 1, 1)));
        Check.That(PxRegion.Union(a, new PxRegion(6, -1, 7, 0)) == new PxRegion(0, -1, 7, 4));
        Check.That(PxRegion.Union(PxRegion.Empty, new PxRegion(6, 6, 7, 7)) == new PxRegion(6, 6, 7, 7));

        // The same answers as PxRect, converted.
        var ra = new PxRect(0, 0, 4, 4);
        var rb = new PxRect(2, 1, 4, 2);
        Check.That((PxRegion)PxRect.Intersect(ra, rb) == PxRegion.Intersect((PxRegion)ra, (PxRegion)rb));
        Check.That((PxRegion)PxRect.Union(ra, rb) == PxRegion.Union((PxRegion)ra, (PxRegion)rb));

        // In int the size wraps around (5.2), and NaN equals itself in Equals (5.5).
        Check.That(new PxRegioni(int.MinValue, 0, int.MaxValue, 0).Width == -1);
        Check.That(new PxRegion(double.NaN, 0, 1, 1).Equals(new PxRegion(double.NaN, 0, 1, 1)));
    }
}
