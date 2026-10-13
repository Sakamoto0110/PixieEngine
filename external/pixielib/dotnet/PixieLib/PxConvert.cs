namespace PixieLib;

// The conversion between precisions that can lose (5.3), the same as pxConvert in C++. To int it
// truncates toward zero, saturates out of range and takes NaN to 0. .NET 9 and later already do that
// in a cast, but net481 gives int.MinValue out of range, so both targets go through here.
internal static class PxConvert
{
    public static int ToInt32(double value)
    {
        if (double.IsNaN(value))
            return 0;
        if (value >= 2147483648.0)
            return int.MaxValue;
        if (value <= -2147483649.0)
            return int.MinValue;
        return (int)value;
    }

    public static int ToInt32(float value) => ToInt32((double)value);
}
