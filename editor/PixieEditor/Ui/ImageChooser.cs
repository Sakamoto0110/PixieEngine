using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Editing;

namespace PixieEditor.Ui;

// Picks a PNG for a sprite, in a modal window: a FolderBrowser that lists PNG files, starting in the
// project's images/ folder (or the project's, or the folder of the last choice); a click on a file
// chooses it. One for the whole editor, drawn every frame at the top level, so the inspector and the
// viewport's menu can both open it.
internal sealed class ImageChooser(string? projectFolder)
{
    private const string Title = "Choose an image";

    private FolderBrowser? Browser;
    private Action<string>? Chosen;
    private bool OpenRequested;
    private string? LastFolder;

    // Shows the chooser; `chosen` gets the file's full path when one is clicked.
    public void Open(Action<string> chosen)
    {
        Chosen = chosen;
        Browser = new FolderBrowser(LastFolder ?? ProjectImages.StartFolder(projectFolder) ??
                                    Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".png");
        OpenRequested = true;
    }

    public void Draw()
    {
        if (OpenRequested)
        {
            ImGui.OpenPopup(Title);
            OpenRequested = false;
        }

        ImGui.SetNextWindowSize(new Vector2(560, 420), ImGuiCond.Appearing);
        if (!ImGui.BeginPopupModal(Title, ImGuiWindowFlags.None))
            return;

        ImGui.TextDisabled(projectFolder == null
            ? "A PNG file."
            : "A PNG file. One outside the project is copied into its images folder.");
        Browser!.Draw("images", Math.Max(120f, ImGui.GetContentRegionAvail().Y - 60f));
        var close = false;
        if (Browser.ChosenFile is { } file)
        {
            LastFolder = Path.GetDirectoryName(file);
            Chosen?.Invoke(file);
            close = true;
        }

        if (ImGui.Button("Cancel", new Vector2(120, 0)) || ImGui.IsKeyPressed(ImGuiKey.Escape))
            close = true;
        if (close)
        {
            Chosen = null;
            ImGui.CloseCurrentPopup();
        }

        ImGui.EndPopup();
    }
}
