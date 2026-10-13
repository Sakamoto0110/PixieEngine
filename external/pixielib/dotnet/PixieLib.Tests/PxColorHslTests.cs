namespace PixieLib.Tests;

internal static class PxColorHslTests
{
    public static void Run()
    {
        Check.That(new PxColorHsl(10, 0.5, 0.5) == new PxColorHsl(10, 0.5, 0.5, 255));
        Check.That(PxColorHsl.Empty.IsEmpty);

        // The primaries, white, black and a gray, both ways.
        Check.That(Near(PxColorHsl.FromRgba(new PxColorRgba(255, 0, 0)), new PxColorHsl(0, 1, 0.5)));
        Check.That(Near(PxColorHsl.FromRgba(new PxColorRgba(0, 255, 0)), new PxColorHsl(120, 1, 0.5)));
        Check.That(Near(PxColorHsl.FromRgba(new PxColorRgba(0, 0, 255)), new PxColorHsl(240, 1, 0.5)));
        Check.That(Near(PxColorHsl.FromRgba(new PxColorRgba(255, 0, 255)), new PxColorHsl(300, 1, 0.5)));
        Check.That(Near(PxColorHsl.FromRgba(new PxColorRgba(255, 255, 255)), new PxColorHsl(0, 0, 1)));
        Check.That(Near(PxColorHsl.FromRgba(new PxColorRgba(0, 0, 0, 7)), new PxColorHsl(0, 0, 0, 7)));
        Check.That(Near(PxColorHsl.FromRgba(new PxColorRgba(128, 128, 128)), new PxColorHsl(0, 0, 128 / 255.0)));

        Check.That(PxColorHsl.ToRgba(new PxColorHsl(0, 1, 0.5)) == new PxColorRgba(255, 0, 0));
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(120, 1, 0.5)) == new PxColorRgba(0, 255, 0));
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(240, 1, 0.5, 9)) == new PxColorRgba(0, 0, 255, 9));
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(0, 0, 1)) == new PxColorRgba(255, 255, 255));

        // The hue wraps around 360, in both directions.
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(420, 1, 0.5)) == new PxColorRgba(255, 255, 0));
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(-60, 1, 0.5)) == new PxColorRgba(255, 0, 255));

        // Every color survives the round trip.
        bool roundTrip = true;
        for (int v = 0; v < 256; v += 5)
        {
            byte b = (byte)v;
            foreach (var c in new[] { new PxColorRgba(255, b, 0), new PxColorRgba(b, 40, 200), new PxColorRgba(b, b, b, 3) })
                roundTrip &= PxColorHsl.ToRgba(PxColorHsl.FromRgba(c)) == c;
        }
        Check.That(roundTrip);

        // A half rounds to the even neighbor, like std::nearbyint in C++: 126.5 is 126, 127.5 is 128.
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(0, 0, 126.5 / 255)).R == 126);
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(0, 0, 127.5 / 255)).R == 128);

        // Out of range lightness is clamped instead of overflowing the byte.
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(0, 0, 1.5)) == new PxColorRgba(255, 255, 255));
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(0, 0, -0.5)) == new PxColorRgba(0, 0, 0));

        // NaN becomes 0 (5.3), and equals itself in Equals (5.5).
        Check.That(PxColorHsl.ToRgba(new PxColorHsl(0, 0, double.NaN, 7)) == new PxColorRgba(0, 0, 0, 7));
        Check.That(new PxColorHsl(double.NaN, 0, 0).Equals(new PxColorHsl(double.NaN, 0, 0)));
    }

    private static bool Near(PxColorHsl a, PxColorHsl b) =>
        Math.Abs(a.H - b.H) < 1e-9 && Math.Abs(a.S - b.S) < 1e-9 && Math.Abs(a.L - b.L) < 1e-9 && a.A == b.A;
}
