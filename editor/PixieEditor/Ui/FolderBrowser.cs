using System.Numerics;
using Hexa.NET.ImGui;

namespace PixieEditor.Ui;

// A folder picker drawn with ImGui, so it works on every editor host: the current folder as text that
// can be typed over, Up, the drives on Windows, and the folders inside, where a click goes in.
// Folders that hold a project are marked.
internal sealed class FolderBrowser
{
    private static readonly Vector4 ProjectColor = new(0.55f, 0.8f, 1.0f, 1.0f);
    private static readonly Vector4 ErrorColor = new(1.0f, 0.45f, 0.4f, 1.0f);

    private string Typed = "";
    private (string Path, string Name, bool Project)[] Folders = [];
    private string? Error;

    public FolderBrowser(string start)
    {
        Current = "";
        if (!Go(start))
            Go(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile));
    }

    public string Current { get; private set; }

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
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or ArgumentException or NotSupportedException)
        {
            Error = e.Message;
            Folders = [];
            Typed = Current;
            return false;
        }
    }

    public void Draw(string id, float height)
    {
        ImGui.PushID(id);
        if (ImGui.Button("Up") && Directory.GetParent(Current) is { } parent)
            Go(parent.FullName);
        ImGui.SameLine();
        ImGui.SetNextItemWidth(-1);
        if (ImGui.InputText("##path", ref Typed, 1024, ImGuiInputTextFlags.EnterReturnsTrue))
            Go(Typed);
        if (ImGui.IsItemHovered())
            ImGui.SetTooltip("Type a folder and press Enter.");

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
            if (Folders.Length == 0)
                ImGui.TextDisabled("No folders here.");
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
        }

        ImGui.EndChild();
        if (Error != null)
            ImGui.TextColored(ErrorColor, Error);
        ImGui.PopID();
    }
}
