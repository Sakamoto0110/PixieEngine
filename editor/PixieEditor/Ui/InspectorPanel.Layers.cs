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
            var scheme = LayerScheme.Of(session, type, field);
            var held = handles.Select(h => (int)h.Get(field).X).Distinct().ToArray();
            var selected = Enumerable.Range(1, scheme.Slots).Where(s => (held[0] & (1 << (s - 1))) != 0).ToArray();
            var summary = held.Length > 1 ? "mixed" : selected.Length == scheme.Slots ? "all layers" : selected.Length switch
            {
                0 => "none",
                <= 2 => string.Join(", ", selected.Select(s => scheme.Name(session, s))),
                _ => $"{selected.Length} layers"
            };
            ImGui.PushID(field);
#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
            var editLocked = DrawCustomEditControls(type, field, handles[0].Record);
#endif
            if (ImGui.TreeNodeEx($"{label}: {summary}##filter", ImGuiTreeNodeFlags.DefaultOpen))
            {
                if (ImGui.IsItemHovered()) ImGui.SetTooltip(session.FieldText(type, field, PixieFieldText.Tooltip));
                if (ImGui.BeginChild("layers", new Vector2(0, 150), ImGuiChildFlags.Borders))
                {
                    for (var slot = 1; slot <= scheme.Slots; ++slot)
                    {
                        var bit = 1 << (slot - 1);
                        var values = handles.Select(h => (int)h.Get(field).X).ToArray();
                        var active = values.All(v => (v & bit) != 0);
                        var mixed = !active && values.Any(v => (v & bit) != 0);
                        // Keep the last membership bit: zero membership is invalid natively.
                        var lastMembership = info.RangeMin > 0 && active && values.Any(v => v == bit);
                        ImGui.BeginDisabled(info.Flags.HasFlag(PixieFieldFlags.ReadOnly) || lastMembership
#if PIXIE_INSPECTOR_FIELD_INTERACTIONS
                            || editLocked
#endif
                        );
                        if (ImGui.Checkbox($"{slot}: {scheme.Name(session, slot)}{(mixed ? " (mixed)" : "")}##{slot}", ref active))
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
        var schemes = Enumerable.Range(0, session.FieldCount(type))
            .Where(f => session.FieldInfo(type, f).Editor == PixieEditorKind.LayerMask)
            .Select(f => LayerScheme.Of(session, type, f)).Distinct().ToArray();
        foreach (var scheme in schemes)
        {
            var popup = $"{scheme.NamesType}-names";
            if (ImGui.SmallButton(schemes.Length > 1 || scheme != LayerScheme.Collision ? $"Edit {scheme.Noun.ToLowerInvariant()} layer names..." : "Edit layer names..."))
                ImGui.OpenPopup(popup);
            if (ImGui.BeginPopup(popup))
            {
                ImGui.TextUnformatted($"Scene {scheme.Noun.ToLowerInvariant()} layer names (fixed slots)");
                ImGui.TextDisabled("Clearing a name preserves its bits.");
                var namesType = session.TypeIndex(scheme.NamesType);
                var owners = session.Entities().Where(e => session.Has(e, namesType)).ToArray();
                if (owners.Length > 1) ImGui.TextUnformatted("Remove the duplicate configuration in Entities first.");
                else DrawLayerNameInputs(owners.Length == 0 ? null : owners[0], namesType);
                ImGui.EndPopup();
            }
        }
    }

    private void DrawLayerNames(int type, int list)
    {
        if (Entities.Length != 1) { ImGui.TextDisabled("Select one configuration to edit its layer names."); return; }
        DrawLayerNameInputs(Entities[0], type);
        var scheme = LayerScheme.OfNamesType(session, type);
        var (itemType, count) = session.RecordInfo(Entities[0], type, list);
        var slotField = session.FieldIndex(itemType, "slot");
        var slots = Enumerable.Range(0, count).Select(i => (int)session.GetRecord(Entities[0], type, list, i, slotField).X).ToArray();
        int? remove = null;
        for (var i = 0; i < slots.Length; ++i)
            if (slots[i] < 1 || slots[i] > scheme.Slots || slots.Count(s => s == slots[i]) > 1)
                if (ImGui.SmallButton($"Remove invalid/duplicate record {i + 1} (slot {slots[i]})")) remove = i;
        if (remove is { } index)
        {
            recorder.Close();
            authoring.Change(Entities, $"removal of invalid {scheme.Noun.ToLowerInvariant()} layer name", e => session.EditRecord(e, type, list, index, remove: true));
        }
    }

    private void DrawLayerNameInputs(uint? owner, int type)
    {
        var scheme = LayerScheme.OfNamesType(session, type);
        if (ImGui.BeginChild("name-slots", new Vector2(300, 300), ImGuiChildFlags.Borders))
        {
            for (var slot = 1; slot <= scheme.Slots; ++slot)
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
                    RenameLayer(scheme, slot, text);
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

    public bool RenameLayer(int slot, string name) => RenameLayer(LayerScheme.Collision, slot, name);

    public bool RenameLayer(LayerScheme scheme, int slot, string name)
    {
        recorder.Close();
        return authoring.RenameLayer(scheme, slot, name);
    }

    // The script exercises the same checkbox operation, including mixed selections and the
    // last-membership guard; it never substitutes a precomputed whole filter value.
    public bool SetLayer(string component, string property, int slot, bool enabled, int? shape = null)
    {
        if (slot < 1 || Entities.Length == 0) return false;
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
        if (slot > LayerScheme.Of(session, type, field).Slots) return false;
        if (!enabled && info.RangeMin > 0 && handles.Any(h => (int)h.Get(field).X == (1 << (slot - 1)))) return false;
        WriteLayerBit(handles, field, slot, enabled);
        return true;
    }
}
