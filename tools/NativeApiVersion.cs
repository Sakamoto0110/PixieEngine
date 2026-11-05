using System;
using System.Globalization;
using System.Linq;
using System.Reflection;

// Build metadata comes from the native header; no runtime source-tree access.
internal static class NativeApiVersion
{
    internal static readonly int Current = Read("PixieNativeApiVersion");
    internal static readonly int IdentitySchema = Read("PixieSdkIdentitySchema");

    private static int Read(string key)
    {
        var values = typeof(NativeApiVersion).Assembly.GetCustomAttributes<AssemblyMetadataAttribute>()
            .Where(value => value.Key == key).ToArray();
        if (values.Length != 1 || !int.TryParse(values[0].Value, NumberStyles.None,
                CultureInfo.InvariantCulture, out var value) || value <= 0)
            throw new InvalidOperationException($"Missing or invalid selected native metadata: {key}.");
        return value;
    }
}
