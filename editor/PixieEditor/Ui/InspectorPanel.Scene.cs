using Hexa.NET.ImGui;
using PixieEditor.Native;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private ulong? BoundScope;
    private string ScopeName = "";
    private bool ScopeNameActive;

    // Rebuild bindings without drawing a second interactive Inspector inside another panel.
    internal void Rebind(Selection selection)
    {
        if (selection.IsWorld) { BindWorld(); return; }
        if (selection.Scope is { } scope)
        {
            Rebuild([], []);
            BoundScope = scope;
            ScopeName = session.SceneScopes().First(s => s.Id == scope).Name;
            return;
        }
        var entities = selection.Items.ToArray();
        var types = entities.Length == 0 ? [] : Enumerable.Range(0, session.TypeCount)
            .Where(t => entities.All(e => session.Has(e, t))).ToArray();
        Rebuild(entities, types);
    }

    private void DrawScope(ulong id)
    {
        var scope = session.SceneScopes().FirstOrDefault(s => s.Id == id);
        if (scope.Id == 0) return;
        if (BoundScope != id || BoundRevision != session.BindingRevision) Rebind(newScope(id));
        ScopeNameActive = false;
        if (ImGui.Begin("Inspector"))
        {
            ImGui.TextUnformatted("Scene properties");
            ImGui.TextDisabled($"ID {id:x16}");
            ImGui.TextWrapped("Logical membership within World. Inspecting this Scene does not change the viewport Environment.");
            var scenes = authoring.Scenes;
            var problem = scenes?.UnavailableReason;
            ImGui.BeginDisabled(problem != null || scenes == null);
            if (ImGui.InputText("Name", ref ScopeName, 257)) scenes?.Queue("rename", id, ScopeName);
            ScopeNameActive = ImGui.IsItemActive();
            ImGui.BeginDisabled(ScopeNameActive || scenes?.HasPending == true);
            if (ImGui.Button("Remove empty Scene")) scenes?.Queue("remove", id);
            ImGui.EndDisabled();
            ImGui.EndDisabled();
            if (problem != null) ImGui.TextWrapped(problem);
            ImGui.TextWrapped("Reassign members before removal. Environment and camera selection remain independent of this Inspector target.");
            DrawPresentation(id);
        }
        ImGui.End();
        static Selection newScope(ulong scopeId) { var selected = new Selection(); selected.SetScope(scopeId); return selected; }
    }

    private void DrawMembership()
    {
        if (authoring.Scenes is not { } scenes || session.ProjectFolder == null) return;
        var scopes = session.SceneScopes();
        var members = Entities.Select(session.EntityScope).Distinct().ToArray();
        var current = members.Length == 1 ? members[0] : (ulong?)null;
        var label = current == null ? "Mixed" : current == 0 ? "Directly in World" : scopes.FirstOrDefault(s => s.Id == current).Name;
        var ids = Entities.Select(session.SceneId).ToArray();
        var disabled = scenes.UnavailableReason != null || scenes.NeedsConversion || ids.Any(id => id == null) || scenes.HasPending;
        ImGui.BeginDisabled(disabled);
        if (ImGui.BeginCombo("Scene membership", label))
        {
            if (ImGui.Selectable("Directly in World", current == 0) && current != 0) scenes.Queue("transfer", entities: ids.OfType<ulong>().ToArray());
            foreach (var scope in scopes)
                if (ImGui.Selectable($"{scope.Name}##{scope.Id:x16}", current == scope.Id) && current != scope.Id)
                    scenes.Queue("transfer", scope.Id, entities: ids.OfType<ulong>().ToArray());
            ImGui.EndCombo();
        }
        ImGui.EndDisabled();
        if (scenes.NeedsConversion) ImGui.TextDisabled("Convert to v2 in Entities to edit Scene membership.");
    }
}
