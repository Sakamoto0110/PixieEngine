using System.Globalization;
using System.Reflection;

// Build metadata comes from the native header; no runtime source-tree access.
internal static class NativeApiVersion
{
    internal static readonly int Current = int.Parse(typeof(NativeApiVersion).Assembly
        .GetCustomAttributes<AssemblyMetadataAttribute>().Single(value => value.Key == "PixieNativeApiVersion").Value!,
        CultureInfo.InvariantCulture);
}
