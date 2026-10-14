namespace PixieEditor.Ui;

// The last thing the editor has to tell: a save, or why an action failed. The toolbar shows it for a
// few seconds; it also goes to the log.
internal static class Status
{
    private const double ShownFor = 5.0;

    private static double ShownAt = double.NegativeInfinity;

    public static string Text { get; private set; } = "";
    public static bool IsError { get; private set; }

    public static bool Visible(double now) => now - ShownAt < ShownFor;

    public static void Show(string text, bool error = false)
    {
        Text = text;
        IsError = error;
        ShownAt = Hexa.NET.GLFW.GLFW.GetTime();
        Console.Error.WriteLine(error ? $"[error] {text}" : $"[info] {text}");
    }
}
