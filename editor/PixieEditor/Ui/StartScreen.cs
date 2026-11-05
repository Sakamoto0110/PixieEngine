using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// What the start screen asked for: a new project in a folder, or an existing one.
internal sealed record ProjectChoice(string Folder, bool Create, bool AdoptSdk = false);

// The editor's first screen, when no game or project was given: New project, Open project, and the
// recent projects. New project takes a location and a name, and makes the folder <location>/<name>;
// Open project takes a folder that holds a .pixie file. Nothing is written until a choice is made.
internal sealed class StartScreen(EditorSettings settings, EngineSdk? sdk = null, string? profile = null, string? configuration = null,
    string? expectedId = null, string? expectedBuildId = null, string? expectedManifestSha256 = null)
{
    private enum Page { Home, New, Open, Sdk }

    private static readonly Vector4 ErrorColor = new(1.0f, 0.45f, 0.4f, 1.0f);
    private static readonly Vector4 DimColor = new(0.6f, 0.6f, 0.6f, 1.0f);

    private const float MaxWidth = 760f;
    private const float Margin = 40f;
    private const float ButtonWidth = 180f;
    private const float ButtonGap = 8f;

    private Page Showing = Page.Home;
    // The column's height on the last frame, to center it in the window on this one.
    private float ContentHeight = 300f;
    private FolderBrowser? Browser;
    private string NewName = "";
    internal EngineSdk? SelectedSdk { get; private set; } = sdk;
    internal ProjectChoice? PendingChoice { get; private set; }
    private FolderBrowser? SdkBrowser;
    private string SelectedProfile = profile ?? settings.SdkProfile;
    private string SelectedConfiguration = configuration ?? settings.NativeConfiguration;
    private bool AdoptSdk;
    private readonly bool OverrideLocked = sdk?.Origin is "launch" or "environment";

    internal void RequestProject(ProjectChoice choice)
    {
        PendingChoice = choice;
        if (SelectedSdk == null) ChooseSdk();
    }

    private void ChooseSdk()
    {
        Showing = Page.Sdk;
        SdkBrowser = new FolderBrowser(settings.SdkRoot ?? Environment.GetFolderPath(Environment.SpecialFolder.UserProfile));
        Error = null;
    }
    internal void RequestSdk() => ChooseSdk();

    internal ProjectChoice? ConfirmSdk(string root)
    {
        var selected = EngineSdk.Load(root, SelectedProfile, SelectedConfiguration, expectedId: expectedId,
            expectedBuildId: expectedBuildId, expectedManifestSha256: expectedManifestSha256);
        settings.ConfirmSdk(selected);
        SelectedSdk = selected;
        Showing = Page.Home;
        var choice = PendingChoice; PendingChoice = null;
        return choice;
    }

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
            // A column centered in the window, both ways, with a margin all around.
            var width = Math.Max(1f, Math.Min(MaxWidth, viewport.WorkSize.X - 2f * Margin));
            var top = Math.Max(Margin, (viewport.WorkSize.Y - ContentHeight) / 2f);
            ImGui.SetCursorPos(new Vector2((viewport.WorkSize.X - width) / 2f, top));
            if (ImGui.BeginChild("##start", new Vector2(width, 0), ImGuiChildFlags.AutoResizeY))
            {
                ImGui.PushFont(ImGui.GetFont(), ImGui.GetFontSize() * 1.6f);
                Centered("PixieEditor");
                ImGui.PopFont();
                Centered("A project is a folder: copy it, version it, or open it on another machine as it is.", DimColor);
                ImGui.Spacing();
                ImGui.Spacing();
                ImGui.TextUnformatted("Engine SDK");
                ImGui.TextWrapped(SelectedSdk?.Summary ?? "Choose an installed Engine SDK to create or open a project.");
                ImGui.BeginDisabled(OverrideLocked);
                if (ImGui.Button("Choose SDK...")) ChooseSdk();
                ImGui.EndDisabled();
                if (SelectedSdk != null)
                    ImGui.Checkbox("Record this SDK when opening a legacy project", ref AdoptSdk);
                ImGui.Spacing();
                choice = Showing switch
                {
                    Page.Sdk => DrawSdk(),
                    Page.New => DrawNew(),
                    Page.Open => DrawOpen(),
                    _ => DrawHome(),
                };
                if (choice != null && SelectedSdk == null) { RequestProject(choice); choice = null; }
                else if (choice != null && AdoptSdk) choice = choice with { AdoptSdk = true };
                if (Error != null)
                {
                    ImGui.Spacing();
                    ImGui.TextColored(ErrorColor, Error);
                }
            }

            ImGui.EndChild();
            ContentHeight = ImGui.GetItemRectSize().Y;
        }

        ImGui.End();
        return choice;
    }

    private ProjectChoice? DrawSdk()
    {
        ImGui.TextWrapped("Choose the installed SDK folder containing sdk.json. Project folders remain portable.");
        SdkBrowser!.Draw("sdk", 260);
        if (ImGui.BeginCombo("Native configuration", SelectedConfiguration))
        {
            foreach (var value in new[] { "Release", "Debug" })
                if (ImGui.Selectable(value, value == SelectedConfiguration)) SelectedConfiguration = value;
            ImGui.EndCombo();
        }
#if PIXIE_DEVELOPMENT
        if (ImGui.BeginCombo("Profile", SelectedProfile))
        {
            foreach (var value in new[] { "authoring", "local-development" })
                if (ImGui.Selectable(value, value == SelectedProfile)) SelectedProfile = value;
            ImGui.EndCombo();
        }
#endif
        ProjectChoice? choice = null;
        if (ImGui.Button("Use SDK"))
        {
            try { choice = ConfirmSdk(SdkBrowser.Current); }
            catch (EngineException error) { Error = error.Message; }
        }
        ImGui.SameLine();
        if (ImGui.Button("Cancel")) { PendingChoice = null; Showing = Page.Home; Error = null; }
        return choice;
    }

    // A line of text in the middle of the column; wrapped when the column is narrower than it.
    private static void Centered(string text, Vector4? color = null)
    {
        var room = ImGui.GetContentRegionAvail().X;
        var size = ImGui.CalcTextSize(text).X;
        if (size < room)
            ImGui.SetCursorPosX(ImGui.GetCursorPosX() + (room - size) / 2f);
        ImGui.PushStyleColor(ImGuiCol.Text, color ?? ImGui.GetStyle().Colors[(int)ImGuiCol.Text]);
        if (size < room)
            ImGui.TextUnformatted(text);
        else
            ImGui.TextWrapped(text);
        ImGui.PopStyleColor();
    }

    private string Location => settings.LastLocation is { } last && Directory.Exists(last)
        ? last
        : Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);

    private ProjectChoice? DrawHome()
    {
        // The two buttons side by side in the middle, or one above the other when they do not fit.
        var room = ImGui.GetContentRegionAvail().X;
        var sideBySide = room >= 2f * ButtonWidth + ButtonGap;
        var buttonsX = ImGui.GetCursorPosX() + Math.Max(0f, (room - (sideBySide ? 2f * ButtonWidth + ButtonGap : ButtonWidth)) / 2f);
        ImGui.SetCursorPosX(buttonsX);
        if (ImGui.Button("New project", new Vector2(ButtonWidth, 40)))
        {
            Showing = Page.New;
            Browser = new FolderBrowser(Location);
            NewName = "";
            Error = null;
        }

        if (sideBySide)
            ImGui.SameLine(0f, ButtonGap);
        else
            ImGui.SetCursorPosX(buttonsX);
        if (ImGui.Button("Open project", new Vector2(ButtonWidth, 40)))
        {
            Showing = Page.Open;
            Browser = new FolderBrowser(Location);
            Error = null;
        }

        ImGui.Spacing();
        ImGui.Separator();
        Centered("Recent projects");
        if (settings.RecentProjects.Count == 0)
            Centered("None yet.", DimColor);

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
