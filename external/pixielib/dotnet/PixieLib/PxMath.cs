using System;

namespace PixieLib;

// The trigonometry of the matrices, in the precision of the matrix, as std::cos is in C++: a float
// goes to MathF, which calls the cosf of the C runtime, as C++ does. MathF does not exist in net481,
// where a float goes through the double function and is rounded, which can differ in the last bit.
internal static class PxMath
{
    public static double Cos(double x) => Math.Cos(x);
    public static double Sin(double x) => Math.Sin(x);
    public static double Tan(double x) => Math.Tan(x);

#if NETCOREAPP
    public static float Cos(float x) => MathF.Cos(x);
    public static float Sin(float x) => MathF.Sin(x);
    public static float Tan(float x) => MathF.Tan(x);
#else
    public static float Cos(float x) => (float)Math.Cos(x);
    public static float Sin(float x) => (float)Math.Sin(x);
    public static float Tan(float x) => (float)Math.Tan(x);
#endif
}
