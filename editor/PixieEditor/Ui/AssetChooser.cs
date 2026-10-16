using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Editing;

namespace PixieEditor.Ui;

// Picks a file of one asset kind (a PNG for a sprite, a TrueType font for a text), in a modal window:
// a FolderBrowser that lists the files of that kind, starting in the project's folder for the kind
// (images/, fonts/), or the project's, or the folder of the last choice of that kind; a click on a
// file chooses it. One for the whole editor, drawn every frame at the top level, so the inspector and
// the viewport's menu can both open it.
internal sealed class AssetChooser(string? projectFolder)
{
    private const string Title = "Choose a file";

    private FolderBrowser? Browser;
    private Action<string>? Chosen;
    private bool OpenRequested;
    private string Kind = "png";
    private readonly Dictionary<string, string> LastFolders = [];

    // Shows the chooser for files of `kind` ("png", "ttf"); `chosen` gets the file's full path when
    // one is clicked.
    public void Open(string kind, Action<string> chosen)
    {
        Kind = kind;
        Chosen = chosen;
        var start = LastFolders.GetValueOrDefault(kind) ?? ProjectAssets.StartFolder(projectFolder, kind) ??
                    Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        Browser = new FolderBrowser(start, "." + kind);
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

        var what = Kind == "png" ? "A PNG file." : Kind == "ttf" ? "A TrueType font (.ttf)." : $"A .{Kind} file.";
        ImGui.TextDisabled(projectFolder == null
            ? what
            : $"{what} One outside the project is copied into its {ProjectAssets.FolderFor(Kind)} folder.");
        Browser!.Draw("files", Math.Max(120f, ImGui.GetContentRegionAvail().Y - 60f));
        var close = false;
        if (Browser.ChosenFile is { } file)
        {
            if (Path.GetDirectoryName(file) is { } folder)
                LastFolders[Kind] = folder;
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
