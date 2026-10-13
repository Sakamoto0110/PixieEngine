using System.Reflection;
using System.Runtime.CompilerServices;

namespace PixieLib.Tests;

// A failed check prints where it is and is counted; Main returns non-zero when any failed.
internal static class Check
{
    public static int Failures { get; private set; }

    public static void That(bool condition, [CallerArgumentExpression(nameof(condition))] string expression = "",
        [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        if (condition)
            return;
        Failures++;
        Console.WriteLine($"{Path.GetFileName(file)}:{line}: check failed: {expression}");
    }

    // The text of a primitive. A failure also prints the text that came out and the bits of each
    // number in it, since some runtimes (the .NET Framework, on Windows) are only seen in the CI.
    public static void Text(object actual, string expected, [CallerArgumentExpression(nameof(actual))] string expression = "",
        [CallerFilePath] string file = "", [CallerLineNumber] int line = 0)
    {
        string text = actual.ToString() ?? "";
        if (text == expected)
            return;
        Failures++;
        Console.WriteLine($"{Path.GetFileName(file)}:{line}: text of {expression}");
        Console.WriteLine($"    expected {expected}");
        Console.WriteLine($"    got      {text}");
        Console.WriteLine($"    bits     {string.Join(" ", Bits(actual))}");
    }

    private static IEnumerable<string> Bits(object value)
    {
        switch (value)
        {
            case double d:
                yield return BitConverter.DoubleToInt64Bits(d).ToString("X16");
                yield break;
            case float f:
                yield return BitConverter.ToInt32(BitConverter.GetBytes(f), 0).ToString("X8");
                yield break;
        }
        foreach (var field in value.GetType().GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic))
            foreach (string bits in Bits(field.GetValue(value)!))
                yield return bits;
    }

    public static void Throws<TException>(Action action, [CallerArgumentExpression(nameof(action))] string expression = "",
        [CallerFilePath] string file = "", [CallerLineNumber] int line = 0) where TException : Exception
    {
        try
        {
            action();
        }
        catch (TException)
        {
            return;
        }
        catch (Exception)
        {
        }
        Failures++;
        Console.WriteLine($"{Path.GetFileName(file)}:{line}: did not throw {typeof(TException).Name}: {expression}");
    }

    // Whether TFrom converts to TTo implicitly or explicitly, through an operator of either type.
    public static bool Implicit<TFrom, TTo>() => HasOperator<TFrom, TTo>("op_Implicit");

    public static bool Explicit<TFrom, TTo>() => HasOperator<TFrom, TTo>("op_Explicit");

    // Whether an operator of either operand's type takes (TA, TB), as op_Addition: those are the only
    // candidates C# looks at, so with no implicit conversion between them, a false here means that
    // a + b does not compile.
    public static bool Operator<TA, TB>(string name) =>
        typeof(TA).GetMethods(BindingFlags.Public | BindingFlags.Static)
            .Concat(typeof(TB).GetMethods(BindingFlags.Public | BindingFlags.Static))
            .Any(m => m.Name == name && m.GetParameters() is { Length: 2 } p
                && p[0].ParameterType == typeof(TA) && p[1].ParameterType == typeof(TB));

    // The same for a unary operator, as op_UnaryNegation.
    public static bool Operator<T>(string name) =>
        typeof(T).GetMethods(BindingFlags.Public | BindingFlags.Static)
            .Any(m => m.Name == name && m.GetParameters() is { Length: 1 } p && p[0].ParameterType == typeof(T));

    private static bool HasOperator<TFrom, TTo>(string name) =>
        typeof(TFrom).GetMethods(BindingFlags.Public | BindingFlags.Static)
            .Concat(typeof(TTo).GetMethods(BindingFlags.Public | BindingFlags.Static))
            .Any(m => m.Name == name && m.ReturnType == typeof(TTo)
                && m.GetParameters() is { Length: 1 } p && p[0].ParameterType == typeof(TFrom));
}
