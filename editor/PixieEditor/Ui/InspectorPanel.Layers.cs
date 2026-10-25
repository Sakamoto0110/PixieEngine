using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieEditor.Native;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private readonly Dictionary<int, string> LayerTyping = [];

    private void DrawLayerMasks(int type, ComponentHandle[] handles)
    {
        foreach (var field in Enumerable.Range(0, session.FieldCount(type)))
        {
            var info = session.FieldInfo(type, field);
            if (info.Editor != PixieEditorKind.LayerMask) continue;
            var label = session.FieldText(type, field, PixieFieldText.Label);
            var held = handles.Select(h => (int)h.Get(field).X).Distinct().ToArray();
            var selected = Enumerable.Range(1, 31).Where(s => (held[0] & (1 << (s - 1))) != 0).ToArray();
            var summary = held.Length > 1 ? "mixed" : selected.Length switch
            {
                0 => "none", 31 => "all layers",
                <= 2 => string.Join(", ", selected.Select(session.CollisionLayerName)),
                _ => $"{selected.Length} layers"
            };
            ImGui.PushID(field);
            if (ImGui.TreeNodeEx($"{label}: {summary}##filter", ImGuiTreeNodeFlags.DefaultOpen))
            {
                if (ImGui.IsItemHovered()) ImGui.SetTooltip(session.FieldText(type, field, PixieFieldText.Tooltip));
                if (ImGui.BeginChild("layers", new Vector2(0, 150), ImGuiChildFlags.Borders))
                {
                    for (var slot = 1; slot <= 31; ++slot)
                    {
                        var bit = 1 << (slot - 1);
                        var values = handles.Select(h => (int)h.Get(field).X).ToArray();
                        var active = values.All(v => (v & bit) != 0);
                        var mixed = !active && values.Any(v => (v & bit) != 0);
                        // Keep the last membership bit: zero membership is invalid natively.
                        var lastMembership = info.RangeMin > 0 && active && values.Any(v => v == bit);
                        ImGui.BeginDisabled(info.Flags.HasFlag(PixieFieldFlags.ReadOnly) || lastMembership);
                        if (ImGui.Checkbox($"{slot}: {session.CollisionLayerName(slot)}{(mixed ? " (mixed)" : "")}##{slot}", ref active))
                            WriteLayerBit(handles, field, slot, active);
                        ImGui.EndDisabled();
                        if (lastMembership && ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled))
                            ImGui.SetTooltip("Keep at least one membership layer. Enable another before clearing this one.");
                    }
                }
                ImGui.EndChild();
                ImGui.TreePop();
            }
            ImGui.PopID();
        }
        if (!Enumerable.Range(0, session.FieldCount(type)).Any(f => session.FieldInfo(type, f).Editor == PixieEditorKind.LayerMask)) return;
        if (ImGui.SmallButton("Edit layer names...")) ImGui.OpenPopup("layer-names");
        if (ImGui.BeginPopup("layer-names"))
        {
            ImGui.TextUnformatted("Scene layer names (fixed slots)");
            ImGui.TextDisabled("Clearing a name preserves its collision bits.");
            var namesType = session.TypeIndex("CollisionLayers2D");
            var owners = session.Entities().Where(e => session.Has(e, namesType)).ToArray();
            if (owners.Length > 1) ImGui.TextUnformatted("Remove the duplicate configuration in Entities first.");
            else DrawLayerNameInputs(owners.Length == 0 ? null : owners[0], namesType);
            ImGui.EndPopup();
        }
    }

    private void DrawLayerNames(int type, int list)
    {
        if (Entities.Length != 1) { ImGui.TextDisabled("Select one configuration to edit its layer names."); return; }
        DrawLayerNameInputs(Entities[0], type);
        var (itemType, count) = session.RecordInfo(Entities[0], type, list);
        var slotField = session.FieldIndex(itemType, "slot");
        var slots = Enumerable.Range(0, count).Select(i => (int)session.GetRecord(Entities[0], type, list, i, slotField).X).ToArray();
        int? remove = null;
        for (var i = 0; i < slots.Length; ++i)
            if (slots[i] is < 1 or > 31 || slots.Count(s => s == slots[i]) > 1)
                if (ImGui.SmallButton($"Remove invalid/duplicate record {i + 1} (slot {slots[i]})")) remove = i;
        if (remove is { } index)
        {
            recorder.Close();
            authoring.Change(Entities, "removal of invalid collision layer name", e => session.EditRecord(e, type, list, index, remove: true));
        }
    }

    private void DrawLayerNameInputs(uint? owner, int type)
    {
        if (ImGui.BeginChild("name-slots", new Vector2(300, 300), ImGuiChildFlags.Borders))
        {
            for (var slot = 1; slot <= 31; ++slot)
            {
                var text = "";
                if (owner is { } entity)
                {
                    var list = session.FieldIndex(type, "layers");
                    var (item, count) = session.RecordInfo(entity, type, list);
                    var slotField = session.FieldIndex(item, "slot");
                    var index = Enumerable.Range(0, count).FirstOrDefault(i => session.GetRecord(entity, type, list, i, slotField).X == slot, -1);
                    if (index >= 0) text = session.GetRecordText(entity, type, list, index, session.FieldIndex(item, "name"));
                }
                else if (slot == 1) text = "Default";
                if (LayerTyping.TryGetValue(slot, out var typing)) text = typing;
                ImGui.SetNextItemWidth(235);
                ImGui.InputText($"{slot}##name", ref text, 1024);
                if (ImGui.IsItemActive()) LayerTyping[slot] = text; else LayerTyping.Remove(slot);
                if (ImGui.IsItemDeactivatedAfterEdit())
                {
                    RenameLayer(slot, text);
                    // Creation may have changed the owner list. Resume with fresh handles next frame.
                    if (owner == null) break;
                }
            }
        }
        ImGui.EndChild();
    }

    private void WriteLayerBit(ComponentHandle[] handles, int field, int slot, bool enabled)
    {
        var bit = 1 << (slot - 1);
        recorder.Close();
        foreach (var handle in handles)
        {
            var old = (int)handle.Get(field).X;
            handle.Set(field, new PixieValue { X = enabled ? old | bit : old & ~bit });
        }
        recorder.Close();
    }

    public bool RenameLayer(int slot, string name)
    {
        recorder.Close();
        return authoring.RenameCollisionLayer(slot, name);
    }

    // The script exercises the same checkbox operation, including mixed selections and the
    // last-membership guard; it never substitutes a precomputed whole filter value.
    public bool SetLayer(string component, string property, int slot, bool enabled, int? shape = null)
    {
        if (slot is < 1 or > 31 || Entities.Length == 0) return false;
        var type = session.TypeIndex(component);
        if (type < 0) return false;
        ComponentHandle[] handles;
        if (shape is { } index)
        {
            var owner = session.TypeIndex("CompoundCollider2D");
            if (Entities.Length != 1 || owner < 0 || !session.Has(Entities[0], owner)) return false;
            var list = session.FieldIndex(owner, "shapes");
            if (index < 0 || index >= session.RecordInfo(Entities[0], owner, list).Count) return false;
            handles = [new ComponentHandle(session, Entities[0], type, recorder, new RecordAddress(owner, list, index))];
        }
        else
        {
            if (Entities.Any(e => !session.Has(e, type))) return false;
            handles = Entities.Select(e => new ComponentHandle(session, e, type, recorder)).ToArray();
        }
        var field = session.FieldIndex(type, property);
        var info = session.FieldInfo(type, field);
        if (info.Editor != PixieEditorKind.LayerMask || info.Flags.HasFlag(PixieFieldFlags.ReadOnly)) return false;
        if (!enabled && info.RangeMin > 0 && handles.Any(h => (int)h.Get(field).X == (1 << (slot - 1)))) return false;
        WriteLayerBit(handles, field, slot, enabled);
        return true;
    }
}
