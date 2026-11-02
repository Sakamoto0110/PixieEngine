using System.Numerics;
using Hexa.NET.ImGui;

namespace PixieEditor.Ui;

// A folder picker drawn with ImGui, so it works on every editor host: the current folder as text that
// can be typed over, Up, New folder, the drives on Windows, and the folders inside, where a click goes in.
// Folders that hold a project are marked. Given a file extension, it also lists the files with it,
// after the folders, and a click on one chooses it (ChosenFile).
internal sealed class FolderBrowser
{
    private static readonly Vector4 ProjectColor = new(0.55f, 0.8f, 1.0f, 1.0f);
    private static readonly Vector4 ErrorColor = new(1.0f, 0.45f, 0.4f, 1.0f);
    private static readonly Vector4 FileColor = new(0.75f, 0.95f, 0.7f, 1.0f);

    private string Typed = "";
    private (string Path, string Name, bool Project)[] Folders = [];
    private (string Path, string Name)[] Files = [];
    private readonly string? Extension;
    private string? Error;
    private bool CreatingFolder;
    private string NewFolderName = "";
    private bool FocusFolderName;

    public FolderBrowser(string start, string? extension = null)
    {
        Extension = extension;
        Current = "";
        if (!Go(start))
            Go(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile));
    }

    public string Current { get; private set; }

    // The file clicked, or typed and confirmed with Enter, when the browser lists files.
    public string? ChosenFile { get; private set; }

    // Whether the current folder holds a project.
    public bool CurrentIsProject => IsProject(Current);

    public static bool IsProject(string folder)
    {
        try
        {
            return Directory.Exists(folder) && Directory.EnumerateFiles(folder, "*.pixie").Any();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            return false;
        }
    }

    public bool Go(string path)
    {
        try
        {
            var full = Path.GetFullPath(path);
            if (Extension != null && File.Exists(full) && Path.GetExtension(full).Equals(Extension, StringComparison.OrdinalIgnoreCase))
            {
                ChosenFile = full;
                return true;
            }

            if (!Directory.Exists(full))
            {
                Error = $"There is no folder {full}.";
                Typed = Current;
                return false;
            }

            Current = full;
            Typed = full;
            Error = null;
            Folders = Directory.GetDirectories(full)
                .Where(d => !new DirectoryInfo(d).Attributes.HasFlag(FileAttributes.Hidden))
                .Select(d => (d, Path.GetFileName(d), IsProject(d)))
                .OrderBy(f => f.Item2, StringComparer.OrdinalIgnoreCase)
                .ToArray();
            Files = Extension == null
                ? []
                : Directory.GetFiles(full)
                    .Where(f => Path.GetExtension(f).Equals(Extension, StringComparison.OrdinalIgnoreCase))
                    .Select(f => (f, Path.GetFileName(f)))
                    .OrderBy(f => f.Item2, StringComparer.OrdinalIgnoreCase)
                    .ToArray();
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException or NotSupportedException)
        {
            Error = e.Message;
            Folders = [];
            Files = [];
            Typed = Current;
            return false;
        }
    }

    // Creates one child of the current location; an existing entry is never reused.
    internal bool CreateFolder(string name)
    {
        name = name.Trim();
        if (name.Length == 0 || name is "." or ".." || name.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 ||
            OperatingSystem.IsWindows() && name.EndsWith('.'))
        {
            Error = "Enter a valid folder name.";
            return false;
        }

        try
        {
            var folder = Path.Combine(Current, name);
            if (Path.Exists(folder))
            {
                Error = "A file or folder with that name already exists.";
                return false;
            }
            Directory.CreateDirectory(folder);
            return Go(folder);
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException or NotSupportedException)
        {
            Error = e.Message;
            return false;
        }
    }

    public void Draw(string id, float height)
    {
        ImGui.PushID(id);
        if (ImGui.Button("New folder"))
        {
            CreatingFolder = true;
            NewFolderName = "";
            FocusFolderName = true;
            Error = null;
        }
        if (CreatingFolder)
        {
            if (FocusFolderName)
            {
                ImGui.SetKeyboardFocusHere();
                FocusFolderName = false;
            }
            ImGui.SetNextItemWidth(Math.Max(100, ImGui.GetContentRegionAvail().X - 160));
            var confirm = ImGui.InputText("##newFolderName", ref NewFolderName, 256, ImGuiInputTextFlags.EnterReturnsTrue);
            ImGui.SameLine();
            if ((ImGui.Button("Create folder") || confirm) && CreateFolder(NewFolderName))
                CreatingFolder = false;
            ImGui.SameLine();
            if (ImGui.Button("Cancel"))
            {
                CreatingFolder = false;
                Error = null;
            }
        }
        if (ImGui.Button("Up") && Directory.GetParent(Current) is { } parent)
            Go(parent.FullName);
        ImGui.SameLine();
        ImGui.SetNextItemWidth(-1);
        if (ImGui.InputText("##path", ref Typed, 1024, ImGuiInputTextFlags.EnterReturnsTrue))
            Go(Typed);
        if (ImGui.IsItemHovered())
            ImGui.SetTooltip(Extension == null ? "Type a folder and press Enter." : "Type a folder or a file and press Enter.");

        if (OperatingSystem.IsWindows())
        {
            foreach (var drive in DriveInfo.GetDrives().Where(d => d.IsReady))
            {
                if (ImGui.SmallButton(drive.Name))
                    Go(drive.RootDirectory.FullName);
                ImGui.SameLine();
            }

            ImGui.NewLine();
        }

        if (ImGui.BeginChild("##folders", new Vector2(0, height), ImGuiChildFlags.Borders))
        {
            if (Folders.Length == 0 && Files.Length == 0)
                ImGui.TextDisabled(Extension == null ? "No folders here." : $"No folders or {Extension} files here.");
            foreach (var (path, name, project) in Folders)
            {
                if (project)
                    ImGui.PushStyleColor(ImGuiCol.Text, ProjectColor);
                var clicked = ImGui.Selectable(project ? $"{name}    (project)##{path}" : $"{name}##{path}");
                if (project)
                    ImGui.PopStyleColor();
                if (clicked)
                {
                    Go(path);
                    break;
                }
            }

            foreach (var (path, name) in Files)
            {
                ImGui.PushStyleColor(ImGuiCol.Text, FileColor);
                if (ImGui.Selectable($"{name}##{path}"))
                    ChosenFile = path;
                ImGui.PopStyleColor();
            }
        }

        ImGui.EndChild();
        if (Error != null)
            ImGui.TextColored(ErrorColor, Error);
        ImGui.PopID();
    }
}
