using Hexa.NET.GLFW;

namespace PixieEditor.Ui;

// The window's key presses and releases, as GLFW reports them. Installed before ImGui's GLFW backend,
// which chains to it, so ImGui sees the keys too; the viewport takes them from here for the game.
internal static unsafe class KeyEvents
{
    // Kept alive here, since GLFW holds only a native pointer to it.
    private static GLFWkeyfun? Callback;

    public static Queue<(int Key, bool Down)> Pending { get; } = new();

    public static void Install(GLFWwindowPtr window)
    {
        Callback = (_, key, _, action, _) =>
        {
            if (action != GLFW.GLFW_REPEAT)
                Pending.Enqueue((key, action == GLFW.GLFW_PRESS));
        };
        GLFW.SetKeyCallback(window, Callback);
    }
}
