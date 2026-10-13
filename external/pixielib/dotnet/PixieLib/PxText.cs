using System.Globalization;
using System.Numerics;

namespace PixieLib;

// How a primitive becomes text, the same as in C++ (docs/layout.md, 3): its fields in order, in
// parentheses, separated by ", ". A number is written with a point in any culture, in the shortest
// form that reads back as the same value, and never in scientific notation: 100000 stays 100000, not
// 1E+05. NaN and the infinities come out as nan, inf and -inf, like std::to_chars.
internal static class PxText
{
    private static readonly CultureInfo Invariant = CultureInfo.InvariantCulture;

    public static string Tuple(params string[] fields) => "(" + string.Join(", ", fields) + ")";

    public static string Number(int value) => value.ToString(Invariant);

    public static string Number(byte value) => value.ToString(Invariant);

    public static string Number(double value)
    {
        if (double.IsNaN(value))
            return "nan";
        if (double.IsInfinity(value))
            return value > 0 ? "inf" : "-inf";
        if (value == 0)
            return IsNegative(value) ? "-0" : "0";

        long bits = System.BitConverter.DoubleToInt64Bits(value);
        int biased = (int)((bits >> 52) & 0x7FF);
        long fraction = bits & 0xFFFFFFFFFFFFFL;
        return Shortest(value < 0, biased == 0 ? fraction : fraction | (1L << 52), biased == 0 ? -1074 : biased - 1075,
                        fraction == 0 && biased > 1, 17);
    }

    public static string Number(float value)
    {
        if (float.IsNaN(value))
            return "nan";
        if (float.IsInfinity(value))
            return value > 0 ? "inf" : "-inf";
        if (value == 0)
            return IsNegative(value) ? "-0" : "0";

        int bits = System.BitConverter.ToInt32(System.BitConverter.GetBytes(value), 0);
        int biased = (bits >> 23) & 0xFF;
        int fraction = bits & 0x7FFFFF;
        return Shortest(value < 0, biased == 0 ? fraction : fraction | (1 << 23), biased == 0 ? -149 : biased - 150,
                        fraction == 0 && biased > 1, 9);
    }

    // The fewest significant digits that read back as the value mantissa * 2^exponent: for each count
    // of digits, the value rounded to that many (half to even), until one reads back. The digits come
    // from the exact value, and not from the runtime: the .NET Framework does not round the 16th and
    // 17th digits of a double, nor the 9th of a float, always to the nearest, so the same value would
    // have another text there (09/10, in the CI). Below a power of two the neighbor is twice as close
    // (closerBelow).
    private static string Shortest(bool negative, long mantissa, int exponent, bool closerBelow, int maxDigits)
    {
        // The value is numerator / denominator, exactly, and lies in [10^power, 10^(power + 1)).
        BigInteger numerator = mantissa;
        BigInteger denominator = BigInteger.One;
        if (exponent >= 0)
            numerator <<= exponent;
        else
            denominator <<= -exponent;
        int power = (int)System.Math.Floor(System.Math.Log10(mantissa) + exponent * System.Math.Log10(2));
        while (Scaled(numerator, -power).CompareTo(Scaled(denominator, power)) < 0)
            power--;
        while (Scaled(numerator, -power - 1).CompareTo(Scaled(denominator, power + 1)) >= 0)
            power++;

        for (int digits = 1; ; digits++)
        {
            // The value times 10^(digits - 1 - power), rounded half to even: digits significant digits,
            // the last one at 10^last.
            int last = power - digits + 1;
            BigInteger divisor = Scaled(denominator, last);
            BigInteger quotient = BigInteger.DivRem(Scaled(numerator, -last), divisor, out BigInteger remainder);
            int half = (2 * remainder).CompareTo(divisor);
            if (half > 0 || (half == 0 && !quotient.IsEven))
                quotient++;

            if (digits == maxDigits || ReadsBack(quotient, last, mantissa, exponent, closerBelow))
                return Plain(negative, quotient, last);
        }
    }

    // value * 10^power when power is positive, and the value itself otherwise: a comparison or a
    // division of two values with powers of ten puts each power on the side where it is positive.
    private static BigInteger Scaled(BigInteger value, int power) => power > 0 ? value * BigInteger.Pow(10, power) : value;

    // Whether digits * 10^power reads back as mantissa * 2^exponent: it does when it falls between the
    // halfway points to the two neighbors, and on a halfway point when the mantissa is even (the
    // rounding of IEEE). Everything is counted in quarters of the last place, so the bounds are
    // integers.
    private static bool ReadsBack(BigInteger digits, int power, long mantissa, int exponent, bool closerBelow)
    {
        BigInteger numerator = Scaled(digits, power);
        BigInteger denominator = power < 0 ? BigInteger.Pow(10, -power) : BigInteger.One;
        if (2 - exponent >= 0)
            numerator <<= 2 - exponent;
        else
            denominator <<= exponent - 2;

        bool even = (mantissa & 1) == 0;
        int low = numerator.CompareTo((4 * (BigInteger)mantissa - (closerBelow ? 1 : 2)) * denominator);
        int high = numerator.CompareTo((4 * (BigInteger)mantissa + 2) * denominator);
        return (low > 0 || (low == 0 && even)) && (high < 0 || (high == 0 && even));
    }

    // digits * 10^power without an exponent and without trailing zeros: 15 * 10^-2 is 0.15, and 1 * 10^5
    // is 100000.
    private static string Plain(bool negative, BigInteger digits, int power)
    {
        while (!digits.IsZero && (digits % 10).IsZero)
        {
            digits /= 10;
            power++;
        }

        string text = digits.ToString(Invariant);
        int integerDigits = text.Length + power;
        string plain;
        if (integerDigits <= 0)
            plain = "0." + new string('0', -integerDigits) + text;
        else if (power >= 0)
            plain = text + new string('0', power);
        else
            plain = text.Substring(0, integerDigits) + "." + text.Substring(integerDigits);
        return negative ? "-" + plain : plain;
    }

    // -0 compares equal to 0, so the sign comes from the bits; net481 also drops it when formatting.
    private static bool IsNegative(double value) => System.BitConverter.DoubleToInt64Bits(value) < 0;
}
