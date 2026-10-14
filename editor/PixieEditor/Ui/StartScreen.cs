using System.Numerics;
using Hexa.NET.ImGui;

namespace PixieEditor.Ui;

// What the start screen asked for: a new project in a folder, or an existing one.
internal sealed record ProjectChoice(string Folder, bool Create);

// The editor's first screen, when no game or project was given: New project, Open project, and the
// recent projects. New project takes a location and a name, and makes the folder <location>/<name>;
// Open project takes a folder that holds a .pixie file. Nothing is written until a choice is made.
internal sealed class StartScreen(EditorSettings settings)
{
    private enum Page { Home, New, Open }

    private static readonly Vector4 ErrorColor = new(1.0f, 0.45f, 0.4f, 1.0f);
    private static readonly Vector4 DimColor = new(0.6f, 0.6f, 0.6f, 1.0f);

    private Page Showing = Page.Home;
    private FolderBrowser? Browser;
    private string NewName = "";

    // Why the last choice failed, shown until the next one.
    public string? Error { get; set; }

    public ProjectChoice? Draw()
    {
        ProjectChoice? choice = null;
        var viewport = ImGui.GetMainViewport();
        ImGui.SetNextWindowPos(viewport.WorkPos);
        ImGui.SetNextWindowSize(viewport.WorkSize);
        const ImGuiWindowFlags flags = ImGuiWindowFlags.NoDecoration | ImGuiWindowFlags.NoMove |
                                       ImGuiWindowFlags.NoSavedSettings | ImGuiWindowFlags.NoBringToFrontOnFocus;
        if (ImGui.Begin("Start", flags))
        {
            var width = Math.Min(760f, viewport.WorkSize.X - 40f);
            ImGui.SetCursorPos(new Vector2((viewport.WorkSize.X - width) / 2f, 40f));
            if (ImGui.BeginChild("##start", new Vector2(width, 0)))
            {
                ImGui.TextUnformatted("PixieEditor");
                ImGui.TextColored(DimColor, "A project is a folder: copy it, version it, or open it on another machine as it is.");
                ImGui.Spacing();
                choice = Showing switch
                {
                    Page.New => DrawNew(),
                    Page.Open => DrawOpen(),
                    _ => DrawHome(),
                };
                if (Error != null)
                {
                    ImGui.Spacing();
                    ImGui.TextColored(ErrorColor, Error);
                }
            }

            ImGui.EndChild();
        }

        ImGui.End();
        return choice;
    }

    private string Location => settings.LastLocation is { } last && Directory.Exists(last)
        ? last
        : Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);

    private ProjectChoice? DrawHome()
    {
        if (ImGui.Button("New project", new Vector2(180, 40)))
        {
            Showing = Page.New;
            Browser = new FolderBrowser(Location);
            NewName = "";
            Error = null;
        }

        ImGui.SameLine();
        if (ImGui.Button("Open project", new Vector2(180, 40)))
        {
            Showing = Page.Open;
            Browser = new FolderBrowser(Location);
            Error = null;
        }

        ImGui.Spacing();
        ImGui.Separator();
        ImGui.TextUnformatted("Recent projects");
        if (settings.RecentProjects.Count == 0)
            ImGui.TextColored(DimColor, "None yet.");

        string? forget = null;
        ProjectChoice? choice = null;
        foreach (var folder in settings.RecentProjects)
        {
            var exists = FolderBrowser.IsProject(folder);
            ImGui.PushID(folder);
            ImGui.BeginDisabled(!exists);
            if (ImGui.Selectable($"{Path.GetFileName(folder)}", false, ImGuiSelectableFlags.AllowOverlap, new Vector2(200, 0)))
                choice = new ProjectChoice(folder, Create: false);
            ImGui.EndDisabled();
            ImGui.SameLine(220);
            ImGui.TextColored(DimColor, exists ? folder : $"{folder}  (not found)");
            if (!exists)
            {
                ImGui.SameLine();
                if (ImGui.SmallButton("Forget"))
                    forget = folder;
            }

            ImGui.PopID();
        }

        if (forget != null)
            settings.Forget(forget);
        return choice;
    }

    private ProjectChoice? DrawNew()
    {
        ImGui.TextUnformatted("New project");
        ImGui.Spacing();
        ImGui.TextUnformatted("Location");
        Browser!.Draw("location", 260);
        ImGui.Spacing();
        ImGui.TextUnformatted("Name");
        ImGui.SetNextItemWidth(-1);
        // Not EnterReturnsTrue: the name must follow every key, so the hint below does.
        ImGui.InputText("##name", ref NewName, 128);
        var enter = ImGui.IsItemDeactivated() && (ImGui.IsKeyPressed(ImGuiKey.Enter) || ImGui.IsKeyPressed(ImGuiKey.KeypadEnter));

        var name = NewName.Trim();
        var folder = name.Length > 0 ? Path.Combine(Browser.Current, name) : null;
        var problem = name.Length == 0 ? "Give the project a name; its folder takes the same name."
            : name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || name is "." or ".." ? "A folder cannot have that name."
            : Directory.Exists(folder) && Directory.EnumerateFileSystemEntries(folder!).Any() ? "That folder exists and is not empty."
            : null;
        ImGui.TextColored(problem == null ? DimColor : ErrorColor, problem ?? $"Creates {folder}");

        ImGui.Spacing();
        ProjectChoice? choice = null;
        ImGui.BeginDisabled(problem != null);
        if (ImGui.Button("Create", new Vector2(120, 0)) || (enter && problem == null))
            choice = new ProjectChoice(folder!, Create: true);
        ImGui.EndDisabled();
        ImGui.SameLine();
        if (ImGui.Button("Back", new Vector2(120, 0)))
            Showing = Page.Home;
        return choice;
    }

    private ProjectChoice? DrawOpen()
    {
        ImGui.TextUnformatted("Open project");
        ImGui.TextColored(DimColor, "Go into a folder that holds a .pixie file; project folders are marked.");
        ImGui.Spacing();
        Browser!.Draw("open", 320);

        ImGui.Spacing();
        ProjectChoice? choice = null;
        var isProject = Browser.CurrentIsProject;
        ImGui.BeginDisabled(!isProject);
        if (ImGui.Button("Open", new Vector2(120, 0)))
            choice = new ProjectChoice(Browser.Current, Create: false);
        ImGui.EndDisabled();
        ImGui.SameLine();
        if (ImGui.Button("Back", new Vector2(120, 0)))
            Showing = Page.Home;
        ImGui.SameLine();
        ImGui.TextColored(DimColor, isProject ? Path.GetFileName(Browser.Current) : "This folder is not a project.");
        return choice;
    }
}
