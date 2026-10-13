namespace PixieLib.Tests;

internal static class PxRectTests
{
    public static void Run()
    {
        var r = new PxRect(1, 2, 3, 4);
        Check.That(r.Right == 4 && r.Bottom == 6);
        Check.That(new PxRect(new PxPoint(1, 2), new PxSize(3, 4)) == r);
        Check.That(PxRect.Empty.IsEmpty);
        Check.That(!r.IsEmpty);

        // The left and top edges are inside, the right and bottom ones are not.
        Check.That(r.Contains(new PxPoint(1, 2)));
        Check.That(r.Contains(new PxPoint(3.999, 5.999)));
        Check.That(!r.Contains(new PxPoint(4, 2)));
        Check.That(!r.Contains(new PxPoint(1, 6)));
        Check.That(!r.Contains(new PxPoint(0.999, 2)));
        Check.That(!r.Contains(new PxPoint(1, 1.999)));

        var ri = new PxRecti(0, 0, 2, 2);
        Check.That(ri.Contains(new PxPointi(1, 1)) && !ri.Contains(new PxPointi(2, 1)));

        PxRect fromInt = ri;
        Check.That(fromInt == new PxRect(0, 0, 2, 2));
        Check.That((PxRecti)new PxRect(1.9, 2.9, 3.9, 4.9) == new PxRecti(1, 2, 3, 4));
        Check.That(Check.Implicit<PxRectf, PxRect>() && !Check.Implicit<PxRect, PxRectf>());

        Check.That(r.Location == new PxPoint(1, 2) && r.Size == new PxSize(3, 4));
        Check.That(r.Center == new PxPoint(2.5, 4));
        Check.That(new PxRecti(0, 0, 3, -3).Center == new PxPointi(1, -1));
        Check.That(new PxRecti(int.MaxValue - 1, 0, 4, 2).Center == new PxPointi(int.MinValue, 1));
        var moved = r;
        moved.Location = new PxPoint(5, 6);
        moved.Size = new PxSize(7, 8);
        Check.That(moved == new PxRect(5, 6, 7, 8));

        // Moved by a vector, not by a point (8.1); the size stays.
        Check.That(r + new PxVec2(1, -1) == new PxRect(2, 1, 3, 4));
        Check.That(r - new PxVec2(1, -1) == new PxRect(0, 3, 3, 4));
        Check.That(new PxRecti(int.MaxValue, 0, 1, 1) + new PxVec2i(1, 0) == new PxRecti(int.MinValue, 0, 1, 1));
        Check.That(!Check.Operator<PxRect, PxPoint>("op_Addition") && !Check.Operator<PxRect, PxPoint>("op_Subtraction"));

        // Contains in int computes X + Width in 64 bits, so a rectangle near the end works (5.2), while
        // Right wraps around, as in C++.
        var far = new PxRecti(2000000000, 0, 200000000, 10);
        Check.That(far.Contains(new PxPointi(2000000001, 0)) && far.Contains(new PxPointi(int.MaxValue, 9)));
        Check.That(!far.Contains(new PxPointi(1999999999, 0)) && !far.Contains(new PxPointi(2000000001, 10)));
        Check.That(far.Right == -2094967296);

        // Contains a rectangle: its edges within these edges, whatever its size.
        Check.That(r.Contains(r) && r.Contains(new PxRect(2, 3, 1, 1)) && r.Contains(new PxRect(4, 6, 0, 0)));
        Check.That(!r.Contains(new PxRect(2, 3, 3, 1)) && !r.Contains(new PxRect(0, 3, 1, 1)));
        Check.That(far.Contains(new PxRecti(2100000000, 0, 100000000, 10)));

        // Intersect: touching is not sharing, and no area shares nothing.
        var a = new PxRect(0, 0, 4, 4);
        Check.That(PxRect.Intersect(a, new PxRect(2, 1, 4, 2)) == new PxRect(2, 1, 2, 2));
        Check.That(PxRect.Intersect(a, new PxRect(1, 1, 1, 1)) == new PxRect(1, 1, 1, 1));
        Check.That(a.IntersectsWith(new PxRect(3.5, 3.5, 1, 1)));
        Check.That(!a.IntersectsWith(new PxRect(4, 0, 1, 4)) && PxRect.Intersect(a, new PxRect(4, 0, 1, 4)).IsEmpty);
        Check.That(!a.IntersectsWith(new PxRect(1, 1, 0, 2)) && !a.IntersectsWith(new PxRect(1, 1, -1, 2)));
        Check.That(PxRecti.Intersect(far, new PxRecti(int.MaxValue - 5, 5, 10, 10)) == new PxRecti(int.MaxValue - 5, 5, 10, 5));

        // Union: one with no area adds nothing, so it can start from Empty.
        Check.That(PxRect.Union(a, new PxRect(6, -1, 1, 1)) == new PxRect(0, -1, 7, 5));
        Check.That(PxRect.Union(PxRect.Empty, new PxRect(6, 6, 1, 1)) == new PxRect(6, 6, 1, 1));
        Check.That(PxRect.Union(a, new PxRect(9, 9, 0, 5)) == a);
        Check.That(PxRect.Union(new PxRect(9, 9, 0, 5), new PxRect(1, 1, -1, 2)) == new PxRect(9, 9, 0, 5));

        // With a padding: the size can go negative.
        var p = new PxPadding(1, 2, 3, 4);
        Check.That(a.Deflate(p) == new PxRect(1, 2, 0, -2));
        Check.That(a.Inflate(p) == new PxRect(-1, -2, 8, 10));
        Check.That(a.Inflate(p).Deflate(p) == a);

        // To int out of range: saturated, and NaN is 0 (5.3); NaN equals itself in Equals (5.5).
        Check.That((PxRecti)new PxRect(3e9, -3e9, double.NaN, 0.5) == new PxRecti(int.MaxValue, int.MinValue, 0, 0));
        Check.That(new PxRect(double.NaN, 0, 1, 1).Equals(new PxRect(double.NaN, 0, 1, 1)));
    }
}
