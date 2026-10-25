using System.Globalization;
using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieInspector;
using PixieInspector.ImGui;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private sealed record RecordSection(Inspector Inspector, ImGuiInspectorView View, TextField[] Texts);
    private readonly Dictionary<(int Type, int List, int Index), RecordSection> RecordSections = [];
    private readonly Dictionary<(int Type, int List), int> RecordCounts = [];
    private readonly Dictionary<(int Type, int List, int Index, int Field, int Point), Vector2> RecordPoints = [];
    private readonly Dictionary<(int Type, int List, int Index, int Field), string> RecordText = [];

    private RecordSection RecordInspector(int type, int list, int index)
    {
        var key = (type, list, index);
        if (RecordSections.TryGetValue(key, out var found)) return found;
        var (itemType, _) = session.RecordInfo(Entities[0], type, list);
        var handle = new ComponentHandle(session, Entities[0], itemType, recorder, new RecordAddress(type, list, index));
        var inspector = InspectorFactory.Build(session, itemType, [handle], () => { });
        var texts = Enumerable.Range(0, session.FieldCount(itemType))
            .Select(f => (Field: f, Kind: session.FieldInfo(itemType, f).Kind))
            .Where(f => f.Kind is PixieFieldKind.Text or PixieFieldKind.PointList or PixieFieldKind.RecordList)
            .Select(f => new TextField(f.Field, session.FieldName(itemType, f.Field), session.FieldText(itemType, f.Field, PixieFieldText.Label),
                session.FieldText(itemType, f.Field, PixieFieldText.Tooltip), "", f.Kind)).ToArray();
        return RecordSections[key] = new RecordSection(inspector, inspector.CreateImGuiView(), texts);
    }

    private void DrawRecords(int type, TextField field)
    {
        ImGui.PushID(field.Field);
        if (Entities.Length != 1)
        {
            ImGui.TextDisabled("Select one entity to edit its shapes.");
            ImGui.PopID();
            return;
        }
        var (_, count) = session.RecordInfo(Entities[0], type, field.Field);
        if (RecordCounts.TryGetValue((type, field.Field), out var previous) && previous != count) ReleaseRecords();
        RecordCounts[(type, field.Field)] = count;
        ImGui.TextUnformatted($"{field.Label} ({count})");
        if (ImGui.IsItemHovered()) ImGui.SetTooltip(field.Tooltip);
        int? removed = null;
        for (var i = 0; i < count; ++i)
        {
            ImGui.PushID(i);
            var keep = true;
            if (ImGui.CollapsingHeader($"Shape {i + 1}", ref keep, i == 0 ? ImGuiTreeNodeFlags.DefaultOpen : ImGuiTreeNodeFlags.None))
            {
                var section = RecordInspector(type, field.Field, i);
                foreach (var text in section.Texts)
                {
                    if (text.Kind == PixieFieldKind.PointList) DrawRecordPoints(type, field.Field, i, text);
                    else if (text.Kind == PixieFieldKind.Text) DrawRecordText(type, field.Field, i, text);
                    else ImGui.TextDisabled("Nested record lists need a deeper editor.");
                }
                var (itemType, _) = session.RecordInfo(Entities[0], type, field.Field);
                DrawLayerMasks(itemType, [new ComponentHandle(session, Entities[0], itemType, recorder, new RecordAddress(type, field.Field, i))]);
                section.View.Draw();
            }
            if (!keep) removed = i;
            ImGui.PopID();
        }
        if (removed is { } index)
        {
            recorder.Close();
            authoring.EditRecord(Entities[0], type, field.Field, index, remove: true);
        }
        ImGui.BeginDisabled(count >= 256 || removed != null);
        if (ImGui.Button("Add shape"))
        {
            recorder.Close();
            authoring.EditRecord(Entities[0], type, field.Field, count, remove: false);
        }
        ImGui.EndDisabled();
        ImGui.PopID();
    }

    private void DrawRecordPoints(int type, int list, int index, TextField field)
    {
        ImGui.PushID(field.Field);
        var points = session.GetRecordPoints(Entities[0], type, list, index, field.Field);
        if (ImGui.TreeNode($"{field.Label} ({points.Length})"))
        {
            int? removed = null;
            for (var i = 0; i < points.Length; ++i)
            {
                ImGui.PushID(i);
                var key = (type, list, index, field.Field, i);
                if (!RecordPoints.TryGetValue(key, out var value)) value = new Vector2((float)points[i].X, (float)points[i].Y);
                ImGui.SetNextItemWidth(-40);
                ImGui.InputFloat2("##point", ref value);
                if (ImGui.IsItemActive()) RecordPoints[key] = value; else RecordPoints.Remove(key);
                if (ImGui.IsItemDeactivatedAfterEdit())
                {
                    var changed = points.ToArray();
                    changed[i] = (value.X, value.Y);
                    recorder.Close();
                    authoring.SetRecordPoints(Entities[0], type, list, index, field.Field, changed);
                }
                ImGui.SameLine();
                if (ImGui.SmallButton("-")) removed = i;
                ImGui.PopID();
            }
            if (removed is { } at)
            {
                recorder.Close();
                authoring.SetRecordPoints(Entities[0], type, list, index, field.Field, points.Where((_, i) => i != at).ToArray());
            }
            if (ImGui.SmallButton("Add point"))
            {
                recorder.Close();
                authoring.SetRecordPoints(Entities[0], type, list, index, field.Field, [.. points, (32, 0)]);
            }
            ImGui.TreePop();
        }
        ImGui.PopID();
    }

    private void DrawRecordText(int type, int list, int index, TextField field)
    {
        var key = (type, list, index, field.Field);
        if (!RecordText.TryGetValue(key, out var text)) text = session.GetRecordText(Entities[0], type, list, index, field.Field);
        ImGui.InputText(field.Label, ref text, 4096);
        if (ImGui.IsItemActive()) RecordText[key] = text; else RecordText.Remove(key);
        if (ImGui.IsItemDeactivatedAfterEdit())
        {
            recorder.Close();
            authoring.SetRecordText(Entities[0], type, list, index, field.Field, text);
        }
    }

    // Scripted edits use the same descriptor-built node as the inspector, including display units.
    public bool SetShape(int index, string property, string text)
    {
        if (Entities.Length != 1 || session.TypeIndex("CompoundCollider2D") is not (var type and >= 0) || !session.Has(Entities[0], type)) return false;
        var list = session.FieldIndex(type, "shapes");
        if (index < 0 || index >= session.RecordInfo(Entities[0], type, list).Count) return false;
        var section = RecordInspector(type, list, index);
        var (itemType, _) = session.RecordInfo(Entities[0], type, list);
        var maskField = Enumerable.Range(0, session.FieldCount(itemType)).FirstOrDefault(f => session.FieldName(itemType, f) == property, -1);
        if (maskField >= 0 && session.FieldInfo(itemType, maskField).Editor == PixieEditorKind.LayerMask)
        {
            new ComponentHandle(session, Entities[0], itemType, recorder, new RecordAddress(type, list, index))
                .Set(maskField, new PixieValue { X = int.Parse(text, CultureInfo.InvariantCulture) });
            return true;
        }
        if (section.Texts.FirstOrDefault(t => t.Name == property) is { Kind: PixieFieldKind.PointList } points)
        {
            recorder.Close();
            authoring.SetRecordPoints(Entities[0], type, list, index, points.Field, ParsePoints(text));
            return true;
        }
        var node = section.Inspector[property];
        object value = node.ValueType switch
        {
            var t when t == typeof(bool) => bool.Parse(text),
            var t when t == typeof(int) => int.Parse(text, CultureInfo.InvariantCulture),
            var t when t == typeof(double) => double.Parse(text, CultureInfo.InvariantCulture),
            _ => text
        };
        node.SetValue(value);
        return true;
    }

    private void ReleaseRecords()
    {
        foreach (var section in RecordSections.Values) { section.View.Dispose(); section.Inspector.Dispose(); }
        RecordSections.Clear();
        RecordCounts.Clear();
        RecordPoints.Clear();
        RecordText.Clear();
    }
}
