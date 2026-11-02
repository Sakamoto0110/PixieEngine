using Hexa.NET.GLFW;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// Installed before ImGui's GLFW backend, which chains these callbacks. One queue preserves
// keyboard/mouse ordering; the viewport decides which events its capture policy admits.
internal static unsafe class KeyEvents
{
    private static GLFWkeyfun? Key;
    private static GLFWmousebuttonfun? Button;
    private static GLFWcursorposfun? Cursor;
    private static GLFWscrollfun? Scroll;
    private static GLFWwindowfocusfun? Focus;
    public static Queue<PixieInputEvent> Pending { get; } = new();
    internal static bool Collect { get; set; }
    private static void Enqueue(PixieInputEvent input) { if (Collect) Pending.Enqueue(input); }

    public static void Install(GLFWwindowPtr window)
    {
        Pending.Clear(); Collect = false;
        GLFW.SetInputMode(window, GLFW.GLFW_LOCK_KEY_MODS, GLFW.GLFW_TRUE);
        Key = (_, key, scanCode, action, modifiers) => Enqueue(new()
            { Kind = 0, Code = key, ScanCode = scanCode, Action = action, Modifiers = (uint)modifiers });
        Button = (handle, button, action, modifiers) =>
        {
            double x, y; GLFW.GetCursorPos(handle, &x, &y);
            Enqueue(new() { Kind = 1, Code = button, Action = action, Modifiers = (uint)modifiers, X = x, Y = y });
        };
        Cursor = (_, x, y) => Enqueue(new() { Kind = 2, X = x, Y = y });
        Scroll = (handle, x, y) =>
        {
            double cursorX, cursorY; GLFW.GetCursorPos(handle, &cursorX, &cursorY);
            Enqueue(new() { Kind = 2, X = cursorX, Y = cursorY });
            Enqueue(new() { Kind = 3, X = x, Y = y });
        };
        Focus = (_, focused) => { if (focused == 0) Enqueue(new() { Kind = 4 }); };
        GLFW.SetKeyCallback(window, Key);
        GLFW.SetMouseButtonCallback(window, Button);
        GLFW.SetCursorPosCallback(window, Cursor);
        GLFW.SetScrollCallback(window, Scroll);
        GLFW.SetWindowFocusCallback(window, Focus);
        double x, y; GLFW.GetCursorPos(window, &x, &y);
        Enqueue(new() { Kind = 2, X = x, Y = y });
    }
}
