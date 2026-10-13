using Hexa.NET.GLFW;

namespace PixieEditor.Ui;

// The few OpenGL functions the editor calls itself, loaded through GLFW for the current context. The
// engine and ImGui's OpenGL3 backend load their own.
internal static unsafe class Gl
{
    public const uint Rgba = 0x1908;
    public const uint UnsignedByte = 0x1401;
    public const uint PackAlignment = 0x0D05;

    private static delegate* unmanaged[Stdcall]<int, int, int, int, uint, uint, void*, void> ReadPixelsPtr;
    private static delegate* unmanaged[Stdcall]<uint, int, void> PixelStoreiPtr;

    public static void Load()
    {
        ReadPixelsPtr = (delegate* unmanaged[Stdcall]<int, int, int, int, uint, uint, void*, void>)GLFW.GetProcAddress("glReadPixels");
        PixelStoreiPtr = (delegate* unmanaged[Stdcall]<uint, int, void>)GLFW.GetProcAddress("glPixelStorei");
        if (ReadPixelsPtr == null || PixelStoreiPtr == null)
            throw new InvalidOperationException("OpenGL functions could not be loaded.");
    }

    public static void ReadPixels(int x, int y, int width, int height, uint format, uint type, void* pixels) =>
        ReadPixelsPtr(x, y, width, height, format, type, pixels);

    public static void PixelStorei(uint name, int value) => PixelStoreiPtr(name, value);
}
