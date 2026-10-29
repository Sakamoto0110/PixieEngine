using Hexa.NET.ImGui;
using PixieEditor.Native;
using PixieEditor.Ui;

namespace PixieEditor.Development;
internal sealed class ExportPanel(EngineSession session) : IDisposable
{
    private readonly ProjectExportJob Job = new();
    private string Output = session.ProjectFolder == null ? "" : Path.Combine(
        Path.GetDirectoryName(session.ProjectFolder) ?? session.ProjectFolder, Path.GetFileName(session.ProjectFolder) + "-export");
    private bool Requested;
    private bool Reported;
    public bool Running => Job.Running;
    public bool Failed => Job.Failed;
    public void Open() { Requested = true; }
    public void Start(string output)
    {
        if (session.ProjectFolder == null || !session.Editing || session.Unsaved)
        {
            Status.Show("Save the project and stop playing before exporting.", error: true);
            return;
        }
        Output = output;
        if (!Job.Start(session.ProjectFolder, output)) { Status.Show("An export is already running.", error: true); return; }
        Reported = false;
        Status.Show("Exporting the saved project...");
    }
    public void Draw()
    {
        if (Job.Finished && !Reported)
        {
            Reported = true;
            Status.Show(Job.Failed ? "Export failed; open Export game for the build log." : $"Exported game to {Output}", Job.Failed);
        }
        if (Requested) { ImGui.OpenPopup("Export game"); Requested = false; }
        if (!ImGui.BeginPopupModal("Export game", ImGuiWindowFlags.AlwaysAutoResize)) return;
        ImGui.TextUnformatted("Exports the saved project to an empty or new folder.");
        ImGui.BeginDisabled(Job.Running);
        ImGui.SetNextItemWidth(600);
        ImGui.InputText("Destination", ref Output, 4096);
        if (ImGui.Button("Export") && !string.IsNullOrWhiteSpace(Output)) Start(Output);
        ImGui.EndDisabled();
        ImGui.SameLine();
        if (ImGui.Button("Close")) ImGui.CloseCurrentPopup();
        if (Job.Running) ImGui.TextUnformatted("Building and exporting...");
        if (ImGui.CollapsingHeader("Build log"))
            foreach (var line in Job.Lines) ImGui.TextUnformatted(line);
        ImGui.EndPopup();
    }
    public void Dispose() => Job.Dispose();
}
