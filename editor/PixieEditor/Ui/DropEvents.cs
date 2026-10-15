using System.Runtime.InteropServices;
using Hexa.NET.GLFW;

namespace PixieEditor.Ui;

// Files dropped on the window, as GLFW reports them; the viewport takes them and makes sprites of the
// images, where the mouse is.
internal static unsafe class DropEvents
{
    // Kept alive here, since GLFW holds only a native pointer to it.
    private static GLFWdropfun? Callback;

    public static Queue<string[]> Pending { get; } = new();

    public static void Install(GLFWwindowPtr window)
    {
        Callback = (_, count, paths) =>
        {
            var files = new string[count];
            for (var i = 0; i < count; i++)
                files[i] = Marshal.PtrToStringUTF8((nint)paths[i]) ?? "";
            Pending.Enqueue(files);
        };
        GLFW.SetDropCallback(window, Callback);
    }
}
