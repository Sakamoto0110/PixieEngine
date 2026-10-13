using System.Globalization;

namespace PixieLib.Tests;

// The same strings as cpp/tests/pxToString.test.cpp.
internal static class ToStringTests
{
    public static void Run()
    {
        // In a culture with a decimal comma, to show the culture does not matter.
        var culture = CultureInfo.CurrentCulture;
        CultureInfo.CurrentCulture = new CultureInfo("pt-BR");
        try
        {
            Check.That(CultureInfo.CurrentCulture.NumberFormat.NumberDecimalSeparator == ",");
            Strings();
        }
        finally
        {
            CultureInfo.CurrentCulture = culture;
        }
    }

    private static void Strings()
    {
        // The fields in order, in parentheses, separated by ", ".
        Check.That(new PxPoint(1.5, -2).ToString() == "(1.5, -2)");
        Check.That(new PxSize(3, 4.25).ToString() == "(3, 4.25)");
        Check.That(new PxRect(1, 2, 3, 4).ToString() == "(1, 2, 3, 4)");
        Check.That(new PxRegion(1, 2, 4, 6).ToString() == "(1, 2, 4, 6)");
        Check.That(new PxPadding(1, 2, 3, 4).ToString() == "(1, 2, 3, 4)");
        Check.That(new PxColorRgba(255, 0, 128).ToString() == "(255, 0, 128, 255)");
        Check.That(new PxColorHsl(120, 1, 0.5).ToString() == "(120, 1, 0.5, 255)");

        // Integers and bytes as numbers.
        Check.That(new PxPointi(-3, 7).ToString() == "(-3, 7)");
        Check.That(new PxColorRgba(65, 66, 67, 0).ToString() == "(65, 66, 67, 0)");

        // The shortest form that reads back as the same value, in each precision, and never in
        // scientific notation.
        Check.That(new PxPointf(0.1f, 2.5f).ToString() == "(0.1, 2.5)");
        Check.That(new PxPoint(0.1, 1.0 / 3).ToString() == "(0.1, 0.3333333333333333)");
        Check.That(new PxPoint(0.1 + 0.2, 1.0 / 3).ToString() == "(0.30000000000000004, 0.3333333333333333)");
        Check.That(new PxPoint(100000, 0.0000001).ToString() == "(100000, 0.0000001)");
        Check.That(new PxPoint(-0.0, 1e15).ToString() == "(-0, 1000000000000000)");
        Check.That(new PxPointf(1e20f, -1.5e-10f).ToString() == "(100000000000000000000, -0.00000000015)");

        // The longest double there is.
        string text = new PxPoint(double.Epsilon, -double.MaxValue).ToString();
        Check.That(text.Length == 1 + 326 + 2 + 310 + 1);
        Check.That(text.StartsWith("(0.000") && text.Contains("5, -17976931348623157000"));

        Check.That(new PxPoint(double.NaN, double.PositiveInfinity).ToString() == "(nan, inf)");
        Check.That(new PxPoint(-double.NaN, double.NegativeInfinity).ToString() == "(nan, -inf)");
        Check.That(new PxPoint(1e23, -1.5e300).ToString().StartsWith("(100000000000000000000000, -15"));
    }
}
