using System.Drawing;

namespace PixieLib.Tests;

// The bridge with System.Drawing (4.4), only on the double types, as in the InteractiveEditor:
// implicit from it, explicit back, rounding to int and narrowing to float.
internal static class SystemDrawingTests
{
    public static void Run()
    {
        PxPoint fromPoint = new Point(3, -4);
        PxPoint fromPointF = new PointF(1.5f, 2.5f);
        Check.That(fromPoint == new PxPoint(3, -4) && fromPointF == new PxPoint(1.5, 2.5));
        Check.That((Point)new PxPoint(1.5, 2.5) == new Point(2, 2));
        Check.That((PointF)new PxPoint(1.5, 2.5) == new PointF(1.5f, 2.5f));

        PxSize fromSize = new Size(3, 4);
        Check.That(fromSize == new PxSize(3, 4));
        Check.That((Size)new PxSize(2.6, 3.4) == new Size(3, 3));

        PxRect fromRectangle = new Rectangle(1, 2, 3, 4);
        Check.That(fromRectangle == new PxRect(1, 2, 3, 4));
        // Rectangle rounds the edges, not the fields (5.6): from 0.4 to 2.9 is 0 to 3.
        Check.That((Rectangle)new PxRect(0.4, 0.6, 2.5, 3.5) == new Rectangle(0, 1, 3, 3));
        Check.That((Rectangle)new PxRect(0.5, 0, 0.5, 1) == new Rectangle(0, 0, 1, 1));
        Check.That((Rectangle)new PxRect(3e9, -3e9, 1, 1) == new Rectangle(int.MaxValue, int.MinValue, 0, 0));
        Check.That((Point)new PxPoint(double.NaN, 3e9) == new Point(0, int.MaxValue));
        Check.That((RectangleF)new PxRect(1, 2, 3, 4) == new RectangleF(1, 2, 3, 4));

        Check.That(Check.Implicit<Point, PxPoint>() && Check.Explicit<PxPoint, Point>());
        Check.That(!Check.Implicit<PxPoint, Point>());

        // The color, both ways, nothing lost.
        PxColorRgba fromColor = Color.FromArgb(200, 1, 2, 3);
        Check.That(fromColor == new PxColorRgba(1, 2, 3, 200));
        Color toColor = new PxColorRgba(1, 2, 3, 200);
        Check.That(toColor.A == 200 && toColor.R == 1 && toColor.G == 2 && toColor.B == 3);
    }
}
