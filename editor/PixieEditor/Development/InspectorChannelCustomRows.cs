using System.Globalization;
using System.Text.Json.Nodes;
using PixieEditor.Native;

namespace PixieEditor.Ui;

internal sealed partial class InspectorPanel
{
    private (Section Section, int Field, PixieFieldKind Kind)? CustomRow(string path)
    {
        var dot = path.IndexOf('.');
        if (dot <= 0) return null;
        var section = Sections.FirstOrDefault(s => s.Name == path[..dot]);
        if (section == null) return null;
        var name = path[(dot + 1)..];
        var field = Enumerable.Range(0, session.FieldCount(section.Type)).FirstOrDefault(f => session.FieldName(section.Type, f) == name, -1);
        if (field < 0) return null;
        var info = session.FieldInfo(section.Type, field);
        return info.Kind is PixieFieldKind.Text or PixieFieldKind.PointList or PixieFieldKind.RecordList || info.Editor == PixieEditorKind.LayerMask
            ? (section, field, info.Kind) : null;
    }
    private static JsonArray Points((double X, double Y)[] points) => new(points.Select(p => (JsonNode?)new JsonArray(p.X, p.Y)).ToArray());
    private JsonNode? NativeValue(PixieValue value, PixieFieldKind kind) => kind switch
    {
        PixieFieldKind.Bool => JsonValue.Create(value.X != 0),
        PixieFieldKind.Point or PixieFieldKind.Vector => new JsonArray(value.X, value.Y),
        PixieFieldKind.Rect or PixieFieldKind.Color => new JsonArray(value.X, value.Y, value.Z, value.W),
        PixieFieldKind.Entity => value.X == uint.MaxValue ? null : session.SceneId((uint)value.X) is { } id
            ? JsonValue.Create(id.ToString("x16", CultureInfo.InvariantCulture)) : new JsonObject { ["unaddressable"] = true },
        _ => JsonValue.Create(value.X)
    };
    private JsonNode ReadCustom(uint entity, int type, int field, PixieFieldKind kind)
    {
        if (kind == PixieFieldKind.Text) return JsonValue.Create(session.GetText(entity, type, field))!;
        if (kind == PixieFieldKind.PointList) return Points(session.GetPoints(entity, type, field));
        if (kind != PixieFieldKind.RecordList) return NativeValue(session.Get(entity, type, field), kind)!;
        var (itemType, count) = session.RecordInfo(entity, type, field);
        var records = new JsonArray();
        for (var i = 0; i < count; ++i)
        {
            var record = new JsonObject();
            for (var f = 0; f < session.FieldCount(itemType); ++f)
            {
                var memberKind = session.FieldInfo(itemType, f).Kind;
                record[session.FieldName(itemType, f)] = memberKind switch
                {
                    PixieFieldKind.Text => JsonValue.Create(session.GetRecordText(entity, type, field, i, f)),
                    PixieFieldKind.PointList => Points(session.GetRecordPoints(entity, type, field, i, f)),
                    PixieFieldKind.RecordList => new JsonObject { ["unsupported"] = "nested record list" },
                    _ => NativeValue(session.GetRecord(entity, type, field, i, f), memberKind)
                };
            }
            records.Add(record);
        }
        return records;
    }
    private JsonObject ObserveCustom(string path)
    {
        var row = CustomRow(path) ?? throw new ArgumentException("Unknown custom Inspector row.");
        var info = session.FieldInfo(row.Section.Type, row.Field);
        var values = new JsonArray(Entities.Select(e => (JsonNode?)ReadCustom(e, row.Section.Type, row.Field, row.Kind)).ToArray());
        var pendingText = Typing.GetValueOrDefault((row.Section.Type, row.Field));
        var pendingPoints = PointTyping.Keys.Any(k => k.Type == row.Section.Type && k.Field == row.Field);
        return new JsonObject { ["path"] = path, ["kind"] = row.Kind.ToString(), ["source"] = "editor-custom-row",
            ["domain"] = "live-native-storage", ["inspector_node"] = false, ["values"] = values,
            ["held_text_input"] = pendingText, ["pending"] = pendingText != null || pendingPoints,
            ["mixed"] = values.Skip(1).Any(v => !JsonNode.DeepEquals(v, values[0])),
            ["read_only"] = info.Flags.HasFlag(PixieFieldFlags.ReadOnly), ["generation"] = ChannelGeneration,
            ["note"] = "Custom row storage/input state; not PixieInspector ViewValues or widget-formatted text." };
    }
    private JsonObject SetCustom(string path, JsonNode? value)
    {
        var row = CustomRow(path)!.Value;
        var before = ObserveCustom(path);
        var undoBefore = authoring.History.UndoCount;
        if (before["read_only"]!.GetValue<bool>() || before["pending"]!.GetValue<bool>() || row.Kind == PixieFieldKind.RecordList)
            return new JsonObject { ["outcome"] = "refused", ["possibly_applied"] = false, ["before"] = before,
                ["reason"] = "Read-only, active UI input, or record-list structural edit; use the existing editor gesture for records." };
        string text;
        if (row.Kind == PixieFieldKind.PointList)
        {
            if (value is not JsonArray points || points.Count > 65536) throw new ArgumentException("Expected bounded JSON point array.");
            text = string.Join(';', points.Select(p => p is JsonArray pair && pair.Count == 2
                ? string.Join(',', pair.Select(v => v?.GetValue<double>() is { } n && double.IsFinite(n) ? n.ToString("R", CultureInfo.InvariantCulture) : throw new ArgumentException("Invalid point.")))
                : throw new ArgumentException("Point needs two finite coordinates.")));
        }
        else text = value is JsonValue scalar && scalar.TryGetValue<string>(out var literal) ? literal : value?.ToJsonString() ?? "null";
        if (text.Contains('\0')) throw new ArgumentException("Embedded NUL is not allowed.");
        bool returned = false;
        EngineException? native = null;
        Exception? error = null;
        try { returned = Set(row.Section.Name, session.FieldName(row.Section.Type, row.Field), text); }
        catch (Exception e) { error = e; native = NativeFailure(e); }
        finally { try { recorder.Close(); } catch (Exception e) { error ??= e; native ??= NativeFailure(e); } }
        var after = ObserveCustom(path);
        return new JsonObject { ["outcome"] = error != null ? "failed_possibly_partial" : JsonNode.DeepEquals(before["values"], after["values"]) ? "unchanged" : "applied",
            ["handler_returned"] = returned, ["before"] = before, ["after_refresh"] = after, ["exception"] = error?.Message,
            ["possibly_applied"] = error != null, ["pixie_result"] = native?.Result is { } code ? (int)code : null,
            ["pixie_last_error"] = native?.NativeError, ["history_count_before"] = undoBefore, ["history_count_after"] = authoring.History.UndoCount };
    }
}
