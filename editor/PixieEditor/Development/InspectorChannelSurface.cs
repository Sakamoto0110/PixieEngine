using System.Globalization;
using System.Text.Json.Nodes;
using PixieInspector;
using PixieInspector.Diagnostics;
using PixieLib;
using PixieEditor.Native;

namespace PixieEditor.Ui;

// A bounded observation adapter over the registered inspectors. All writes still call Set,
// the same handler used by --set; this file contains no native/property setter.
internal sealed partial class InspectorPanel
{
    private Inspector[] ChannelInstances = [];
    private uint[] ChannelEntities = [];
    private long ChannelEpoch;
    private Action<Inspector>? ChannelNewInspector;
    public long ChannelGeneration { get; private set; }

    public void SynchronizeChannel(IReadOnlyList<uint> selected, long epoch)
    {
        var types = selected.Count == 0 ? [] : Enumerable.Range(0, session.TypeCount)
            .Where(t => selected.All(id => session.Has(id, t))).ToArray();
        if (!selected.SequenceEqual(Entities) || !types.SequenceEqual(Types)) Rebuild([.. selected], types);
        if (epoch != ChannelEpoch) ReleaseRecords();
        var instances = Sections.Select(s => s.Inspector).Concat(RecordSections.Values.Select(s => s.Inspector)).ToArray();
        if (!instances.SequenceEqual(ChannelInstances) || !selected.SequenceEqual(ChannelEntities) || epoch != ChannelEpoch)
        {
            ++ChannelGeneration;
            ChannelInstances = instances;
            ChannelEntities = [.. selected];
            ChannelEpoch = epoch;
        }
        foreach (var instance in instances) instance.Refresh(); // Never Reload or Apply.
    }

    private static JsonNode? SafeValue(object? value) => value switch
    {
        null => null,
        bool b => JsonValue.Create(b),
        string text => JsonValue.Create(text.Length <= 65536 ? text : throw new InvalidOperationException("Inspector text exceeds observation bound.")),
        byte b => JsonValue.Create((int)b),
        short n => JsonValue.Create((int)n),
        int n => JsonValue.Create(n),
        uint n => JsonValue.Create(n),
        long n => JsonValue.Create(n),
        ulong n => JsonValue.Create(n),
        float n when float.IsFinite(n) => JsonValue.Create((double)n),
        double n when double.IsFinite(n) => JsonValue.Create(n),
        decimal n => JsonValue.Create(n),
        PxColorRgba color => new JsonArray(color.R, color.G, color.B, color.A),
        Enum e => JsonValue.Create(e.ToString()),
        _ => new JsonObject { ["unsupported_value_type"] = value.GetType().FullName }
    };
    private static JsonArray Values(IEnumerable<object?> values) => new(values.Select(SafeValue).ToArray());
    private static (JsonArray Values, JsonArray Failures, string? Exception) ReadLive(InspectorNode node)
    {
        var failures = new JsonArray();
        void Failed(object? _, InspectorFailureEventArgs failure) => failures.Add(Failure(failure));
        node.BindFailed += Failed;
        try { return (Values(node.GetValues()), failures, null); }
        catch (Exception e) { return (new JsonArray(), failures, e.Message); }
        finally { node.BindFailed -= Failed; }
    }
    private static EngineException? NativeFailure(Exception? error)
    {
        for (var current = error; current != null; current = current.InnerException)
            if (current is EngineException native) return native;
        return null;
    }
    private static JsonObject? Failure(InspectorFailureEventArgs? failure) => failure is null ? null : new JsonObject
    {
        ["path"] = failure.Path, ["severity"] = failure.Severity.ToString(), ["message"] = failure.Message,
        ["reason"] = failure.Reason, ["handled"] = failure.Handled, ["suggestion"] = failure.Suggestion,
        ["pixie_result"] = NativeFailure(failure.Exception)?.Result is { } code ? (int)code : null,
        ["pixie_last_error"] = NativeFailure(failure.Exception)?.NativeError
    };
    private JsonObject Observe(string component, InspectorNode node, bool live = false)
    {
        var capturedFailure = Failure(node.Failure); // Immutable before optional live getters.
        var read = live && node.ValueType != null ? ReadLive(node) : (new JsonArray(), new JsonArray(), (string?)null);
        var values = node.ValueType is null ? new JsonArray() : live ? read.Item1 : Values(node.ViewValues);
        return new JsonObject
        {
            ["path"] = component + "." + node.Path, ["node_path"] = node.Path, ["component"] = component,
            ["label"] = node.Label, ["editor"] = node.Editor.ToString(),
            ["value_type"] = node.ValueType?.FullName, ["view_type"] = node.ViewType?.FullName,
            ["values"] = values,
            ["domain"] = live ? "live-storage" : "held-view", ["refresh_policy"] = "Refresh; honors BinderControl; preserves pending edits",
            ["binder_control"] = node.Inspector.Options.BinderControl.ToString(),
            ["mixed"] = node.IsMixed, ["read_only"] = node.ReadOnly, ["pending"] = node.HasPendingValue,
            ["compromised"] = node.IsCompromised, ["failure"] = capturedFailure,
            ["read_failures"] = live ? read.Item2 : null, ["read_exception"] = read.Item3,
            ["read_succeeded"] = live ? read.Item2.Count == 0 && read.Item3 == null : null,
            ["group"] = node.IsGroup, ["visible"] = node.Visible, ["ignored"] = node.Ignored,
            ["range"] = node.Range is { } range ? new JsonObject { ["min"] = range.Min, ["max"] = range.Max, ["step"] = range.Step } : null,
            ["scrub_multiplier"] = node.ScrubMultiplier,
            ["unit"] = node.Unit is { } unit ? new JsonObject { ["suffix"] = unit.Suffix, ["factor"] = unit.Factor, ["offset"] = unit.Offset } : null
        };
    }
    public JsonObject ChannelTree()
    {
        var rows = new JsonArray();
        var omitted = new JsonArray();
        foreach (var section in Sections)
        {
            foreach (var node in section.Inspector.Rows)
            {
                if (rows.Count >= 1024) throw new InvalidOperationException("Inspector tree exceeds 1024 visible rows.");
                rows.Add(Observe(section.Name, node));
            }
            foreach (var field in section.Texts)
                omitted.Add(ObserveCustom(section.Name + "." + field.Name));
            for (var f = 0; f < session.FieldCount(section.Type); ++f)
                if (session.FieldInfo(section.Type, f).Editor == PixieEditorKind.LayerMask)
                    omitted.Add(ObserveCustom(section.Name + "." + session.FieldName(section.Type, f)));
        }
        var recordRows = new JsonArray();
        foreach (var (address, section) in RecordSections)
        {
            var prefix = session.TypeName(address.Type) + "." + session.FieldName(address.Type, address.List) + "[" + address.Index + "]";
            foreach (var node in section.Inspector.Rows)
            {
                if (recordRows.Count >= 1024) throw new InvalidOperationException("Record Inspector tree exceeds1024 rows.");
                recordRows.Add(Observe(prefix, node));
            }
        }
        return new JsonObject { ["generation"] = ChannelGeneration, ["traversal"] = "public visible Rows", ["rows"] = rows, ["custom_rows"] = omitted,
            ["record_rows"] = recordRows, ["record_traversal"] = "registered record Inspectors; UI group openness is not observed" };
    }
    private (Section Section, InspectorNode Node) FindChannelNode(string path)
    {
        foreach (var (address, record) in RecordSections)
        {
            var prefix = session.TypeName(address.Type) + "." + session.FieldName(address.Type, address.List) + "[" + address.Index + "]";
            if (!path.StartsWith(prefix + ".", StringComparison.Ordinal)) continue;
            var found = record.Inspector.FirstOrDefault(n => n.Path == path[(prefix.Length + 1)..]);
            if (found != null) return (new Section(address.Type, prefix, record.Inspector, record.View, record.Texts), found);
        }
        var dot = path.IndexOf('.');
        if (dot <= 0) throw new ArgumentException("Inspector path must include its component prefix.");
        var section = Sections.FirstOrDefault(s => s.Name == path[..dot]) ?? throw new ArgumentException("Component is not bound in the selected Inspector.");
        var node = section.Inspector.FirstOrDefault(n => n.Path == path[(dot + 1)..])
            ?? throw new ArgumentException("Path is not a registered Inspector node; custom rows use their existing editor gestures.");
        return (section, node);
    }
    public JsonObject ChannelGet(string path, bool live = false)
    {
        if (CustomRow(path) != null) return ObserveCustom(path);
        var (section, node) = FindChannelNode(path);
        return Observe(section.Name, node, live);
    }
    public JsonObject ChannelReport()
    {
        var reports = new JsonArray();
        foreach (var section in Sections)
            reports.Add(new JsonObject { ["component"] = section.Name, ["creation_error_count"] = section.Inspector.Report.ErrorCount,
                ["creation_unhandled_count"] = section.Inspector.Report.UnhandledErrorCount,
                ["creation_critical_count"] = section.Inspector.Report.CriticalErrorCount,
                ["creation_failures"] = new JsonArray(section.Inspector.Report.Failures.Select(f => (JsonNode?)Failure(f)).ToArray()),
                ["current_nodes"] = new JsonArray(section.Inspector.Where(n => n.Failure != null || n.HasPendingValue || n.IsCompromised)
                    .Take(1024).Select(n => (JsonNode?)Observe(section.Name, n)).ToArray()) });
        return new JsonObject { ["generation"] = ChannelGeneration, ["reports"] = reports };
    }
    public JsonObject ChannelGesture(Func<bool> handler)
    {
        var failures = new JsonArray();
        var nodes = new HashSet<InspectorNode>();
        void Failed(object? _, InspectorFailureEventArgs failure) => failures.Add(Failure(failure));
        void Attach(Inspector instance)
        {
            foreach (var node in instance)
                if (nodes.Add(node)) node.BindFailed += Failed;
        }
        foreach (var instance in Sections.Select(s => s.Inspector).Concat(RecordSections.Values.Select(s => s.Inspector))) Attach(instance);
        ChannelNewInspector = Attach;
        bool returned = false;
        Exception? error = null;
        try { returned = handler(); }
        catch (Exception e) { error = e; }
        finally { ChannelNewInspector = null; foreach (var node in nodes) node.BindFailed -= Failed; }
        var native = NativeFailure(error);
        var eventNative = failures.OfType<JsonObject>().LastOrDefault(f => f["pixie_result"] != null);
        return new JsonObject { ["handler_returned"] = returned, ["bind_failures"] = failures,
            ["exception"] = error?.Message, ["pixie_result"] = native?.Result is { } code ? JsonValue.Create((int)code) : eventNative?["pixie_result"]?.DeepClone(),
            ["pixie_last_error"] = native?.NativeError is { } nativeText ? JsonValue.Create(nativeText) : eventNative?["pixie_last_error"]?.DeepClone(),
            ["outcome"] = error != null || failures.Count > 0 ? "failed_possibly_partial" : returned ? "handler_executed" : "refused_or_partial" };
    }
    public JsonObject ChannelSet(string path, JsonNode? value, long expectedGeneration)
    {
        if (expectedGeneration != ChannelGeneration) throw new InvalidOperationException("Stale Inspector generation; read inspector.tree again.");
        if (CustomRow(path) != null) return SetCustom(path, value);
        if (value is JsonValue nulScalar && nulScalar.TryGetValue<string>(out var nulText) && nulText.Contains('\0')) throw new ArgumentException("Embedded NUL is not allowed.");
        var (section, node) = FindChannelNode(path);
        var before = Observe(section.Name, node);
        var events = new JsonArray();
        var subscribed = Sections.SelectMany(s => s.Inspector).Take(1024).Append(node).Distinct().ToArray();
        void Failed(object? _, InspectorFailureEventArgs failure) => events.Add(Failure(failure));
        foreach (var observed in subscribed) observed.BindFailed += Failed;
        var historyBefore = authoring.History.UndoName;
        var historyCountBefore = authoring.History.UndoCount;
        var returned = false;
        var invoked = false;
        Exception? exception = null;
        try
        {
            var text = value is JsonValue scalar && scalar.TryGetValue<string>(out var literal) ? literal
                : value is JsonArray array ? string.Join(',', array.Select(v => v?.ToJsonString() ?? "null"))
                : value?.ToJsonString() ?? "null";
            var field = Enumerable.Range(0, session.FieldCount(section.Type)).FirstOrDefault(f => session.FieldName(section.Type, f) == node.Path, -1);
            if (field >= 0 && session.FieldInfo(section.Type, field).Kind == PixieFieldKind.Entity)
            {
                ulong? reference = null;
                if (value != null)
                {
                    var id = value.GetValue<string>();
                    if (id.Length != 16 || !ulong.TryParse(id, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out var found) || found == 0)
                        throw new ArgumentException("Entity-reference writes require a16-digit hexadecimal scene ID or null.");
                    reference = found;
                    var required = session.TypeIndex(session.FieldText(section.Type, field, PixieFieldText.References));
                    var target = session.Find(found) ?? throw new ArgumentException("Referenced scene entity is gone.");
                    if (required >= 0 && !session.Has(target, required)) throw new ArgumentException("Referenced entity does not satisfy the native References descriptor.");
                }
                text = Inspection.InspectorFactory.ChannelReferenceLabel(session, reference);
            }
            if (section.Name.StartsWith("CompoundCollider2D.shapes[", StringComparison.Ordinal))
                returned = SetShape(int.Parse(section.Name[section.Name.IndexOf('[')..].Trim('[', ']'), CultureInfo.InvariantCulture), node.Path, text, () => invoked = true);
            else returned = Set(section.Name, node.Path, text, () => invoked = true); // Existing --set/--do shape handler, once.
        }
        catch (Exception e) { exception = e; }
        finally
        {
            foreach (var observed in subscribed) observed.BindFailed -= Failed;
            try { recorder.Close(); } // The same gesture boundary used by scripted/UI edits.
            catch (Exception e) { exception ??= e; }
        }
        // Capture diagnostics before Refresh/live readback can change them.
        var writeFailure = Failure(node.Failure);
        var compromised = node.IsCompromised;
        var pending = node.HasPendingValue;
        var heldAfter = Observe(section.Name, node);
        var readback = node.ValueType != null && !compromised ? ReadLive(node) : (new JsonArray(), new JsonArray(), (string?)null);
        section.Inspector.Refresh();
        var after = Observe(section.Name, node);
        var changed = !JsonNode.DeepEquals(before["values"], after["values"]);
        var failed = exception != null || events.Count > 0 || readback.Item2.Count > 0 || readback.Item3 != null ||
            writeFailure != null && !JsonNode.DeepEquals(writeFailure, before["failure"]);
        var outcome = compromised ? "stale_target" : node.ReadOnly || failed && !invoked ? "refused" : failed ? "failed_possibly_partial"
            : pending || node is ButtonNode && RebuildRequested ? "accepted_pending" : changed ? "applied" : "unchanged";
        var eventNative = events.OfType<JsonObject>().LastOrDefault(f => f["pixie_result"] != null)
            ?? readback.Item2.OfType<JsonObject>().LastOrDefault(f => f["pixie_result"] != null);
        return new JsonObject { ["outcome"] = outcome, ["handler_returned"] = returned,
            ["requested_view_value"] = value?.DeepClone(), ["before"] = before, ["after_handler"] = heldAfter, ["after_refresh"] = after,
            ["live_storage_values"] = readback.Item1, ["bind_failures"] = events, ["write_failure"] = writeFailure,
            ["readback_failures"] = readback.Item2, ["readback_exception"] = readback.Item3,
            ["exception"] = exception?.Message, ["possibly_applied"] = failed && invoked && !node.ReadOnly,
            ["pixie_result"] = NativeFailure(exception)?.Result is { } code ? JsonValue.Create((int)code) : eventNative?["pixie_result"]?.DeepClone(),
            ["pixie_last_error"] = NativeFailure(exception)?.NativeError is { } error ? JsonValue.Create(error) : eventNative?["pixie_last_error"]?.DeepClone(),
            ["history_before"] = historyBefore, ["history_after"] = authoring.History.UndoName,
            ["history_count_before"] = historyCountBefore, ["history_count_after"] = authoring.History.UndoCount,
            ["generation"] = ChannelGeneration };
    }
}
