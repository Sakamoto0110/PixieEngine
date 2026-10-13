namespace PixieLib;

// Combines the hashes of the fields of a primitive. System.HashCode does not exist in net481.
internal static class PxHash
{
    public static int Combine(int a, int b)
    {
        unchecked
        {
            return (a * 397) ^ b;
        }
    }

    public static int Combine(int a, int b, int c, int d) => Combine(Combine(Combine(a, b), c), d);

    // The hash of a field that Equals compares with its own Equals (5.5): 0 and -0 are equal, and so
    // is every NaN, so each group hashes the same, on net481 too.
    public static int Of(double value) => value == 0 ? 0 : double.IsNaN(value) ? -1 : value.GetHashCode();

    public static int Of(float value) => Of((double)value);

    public static int Of(int value) => value;
}
