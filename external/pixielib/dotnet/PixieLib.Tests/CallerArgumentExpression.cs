#if NETFRAMEWORK
namespace System.Runtime.CompilerServices;

// The .NET Framework does not have this attribute; the compiler only looks for it by name, so the
// checks print the failed expression on net481 too.
[AttributeUsage(AttributeTargets.Parameter)]
internal sealed class CallerArgumentExpressionAttribute(string parameterName) : Attribute
{
    public string ParameterName { get; } = parameterName;
}
#endif
