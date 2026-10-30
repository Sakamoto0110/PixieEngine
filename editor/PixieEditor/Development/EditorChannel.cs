using System.Globalization;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using PixieEditor.Native;
using PixieEditor.Ui;

namespace PixieEditor.Development;

// Owner-thread adapter. The transport worker never reads this session or its inspectors.
internal sealed class EditorChannel : IDisposable
{
    private readonly EngineSession Session;
    private readonly Selection Selection;
    private readonly InspectorPanel Inspector;
    private readonly Editing.History History;
    private readonly Func<string, bool> RunCommand;
    private readonly EditorPipe Pipe;
    private readonly CaptureDirectory Captures;
    private readonly string SessionId = "editor-" + Guid.NewGuid().ToString("N");
    private long Client;
    private long Sequence;
    private long InternalId;
    private bool Hello;
    private EngineException? RenderFailure;
    private sealed record PendingCapture(EditorPipe.Command Command, JsonNode? Id, bool Notification, string Path, long ReadyFrame);
    private PendingCapture? Pending;
    private sealed class WireException(int code, string message) : Exception(message) { public int Code { get; } = code; }
    public long Frame { get; private set; }
    private static readonly HashSet<string> Gestures = new(StringComparer.Ordinal)
    {
        "select", "create", "collider", "character", "compound", "addshape", "removeshape", "shape", "layername",
        "layer", "shapelayer", "dynamic", "compoundbox", "kinematic", "sensor", "world", "duplicate", "delete",
        "undo", "redo", "add", "remove", "rename", "addpoint", "removepoint", "move", "colliderhandles", "physicsdebug", "colliderdrag", "play", "stop",
#if PIXIE_CHANNEL_SAVE
        "save",
#endif
    };
#if PIXIE_CHANNEL_SAVE
    private const bool Saves = true;
#else
    private const bool Saves = false;
#endif
    public EditorChannel(string name, string root, EngineSession session, Selection selection, InspectorPanel inspector,
        Editing.History history, Func<string, bool> runCommand)
    {
        Session = session; Selection = selection; Inspector = inspector; History = history; RunCommand = runCommand;
        Captures = new CaptureDirectory(root);
        Pipe = new EditorPipe(name);
    }
    private JsonArray Ids() => new(Selection.Items.Select(Session.SceneId).OfType<ulong>()
        .Select(id => (JsonNode?)JsonValue.Create(id.ToString("x16", CultureInfo.InvariantCulture))).ToArray());
    private void Synchronize()
    {
        Selection.Prune(Session);
        Inspector.SynchronizeChannel(Selection.Items, Session.DevelopmentGeneration);
    }
    private JsonObject Context(JsonObject body)
    {
        body["editor_session"] = SessionId;
        body["frame"] = Frame;
        body["command_sequence"] = Sequence;
        body["selection_generation"] = Inspector.ChannelGeneration;
        body["scene_ids"] = Ids();
        return body;
    }
    private JsonObject Result(JsonNode? id, JsonNode? value) => new()
    {
        ["jsonrpc"] = "2.0", ["id"] = id?.DeepClone(), ["result"] = Context(new() { ["value"] = value })
    };
    private JsonObject Error(JsonNode? id, string message, int code = -32000, PixieResult result = PixieResult.ErrorArgument,
        string? nativeError = null, bool possiblyApplied = false, JsonNode? receipt = null) => new()
    {
        ["jsonrpc"] = "2.0", ["id"] = id?.DeepClone(), ["error"] = new JsonObject
        {
            ["code"] = code, ["message"] = message, ["data"] = Context(new()
            {
                ["pixie_result"] = (int)result, ["pixie_last_error"] = nativeError ?? "",
                ["native_error"] = nativeError != null, ["possibly_applied"] = possiblyApplied, ["receipt"] = receipt
            })
        }
    };
    private static string Text(JsonObject p, string key)
    {
        var text = p[key]?.GetValue<string>() ?? throw new ArgumentException($"Missing {key}.");
        return text.Contains('\0') ? throw new ArgumentException("Embedded NUL is not allowed.") : text;
    }
    private static long Generation(JsonObject p) => p["generation"]?.GetValue<long>() ?? throw new ArgumentException("A current generation from inspector.tree is required.");
    private void RequireGeneration(JsonObject p)
    {
        if (Generation(p) != Inspector.ChannelGeneration) throw new InvalidOperationException("Stale selection/Inspector generation; read inspector.tree again.");
    }
    private JsonObject Native(string method, JsonObject? parameters = null)
    {
        var request = new JsonObject { ["jsonrpc"] = "2.0", ["id"] = --InternalId, ["method"] = method, ["params"] = parameters ?? new() };
        return JsonNode.Parse(Session.DispatchDevelopment(request.ToJsonString()))!.AsObject();
    }
    public void Drain()
    {
        ++Frame;
        RenderFailure = null;
        Synchronize();
        if (Pending is { } capture)
        {
            if (Frame >= capture.ReadyFrame)
            {
                var prepared = Native("world.prepare_capture");
                if (prepared["error"] != null)
                {
                    Pending = null;
                    capture.Command.Reply.TrySetResult(capture.Notification ? "" : Error(capture.Id, "Native render preparation failed.",
                        result: PixieResult.ErrorGl, receipt: prepared).ToJsonString());
                }
            }
            return;
        }
        if (!Pipe.TryTake(out var command) || command == null) return;
        JsonNode? id = null;
        bool notification = false;
        bool possiblyApplied = false;
        try
        {
            var input = JsonNode.Parse(command.Request, documentOptions: new() { MaxDepth = 32 }) as JsonObject
                ?? throw new WireException(-32600, "JSON-RPC request must be an object.");
            if (input["jsonrpc"] is not JsonValue version || !version.TryGetValue<string>(out var versionText) || versionText != "2.0")
                throw new WireException(-32600, "Expected JSON-RPC2.0.");
            id = input["id"];
            if (id != null && !(id is JsonValue v && (v.TryGetValue<string>(out _) || v.TryGetValue<long>(out _) || v.TryGetValue<ulong>(out _))))
            { id = null; throw new WireException(-32600, "Invalid JSON-RPC request id."); }
            if (input["method"] is not JsonValue methodValue || !methodValue.TryGetValue<string>(out _))
                throw new WireException(-32600, "Missing JSON-RPC method.");
            var method = Text(input, "method");
            notification = !input.ContainsKey("id");
            var p = input.ContainsKey("params") ? input["params"] as JsonObject ?? throw new ArgumentException("params must be an object.") : new JsonObject();
            if (Client != command.Client) { Client = command.Client; Hello = false; }
            ++Sequence;
            if (!Hello && method != "world.hello") throw new InvalidOperationException("world.hello must be the first call.");
            JsonObject response;
            if (method == "world.capture")
            {
                var path = Captures.Resolve(Text(p, "name"));
                Pending = new(command, id?.DeepClone(), notification, path, Frame + 1);
                return; // Complete only after a full subsequent UI frame has rendered.
            }
            if (method.StartsWith("world.", StringComparison.Ordinal))
            {
                possiblyApplied = method is not ("world.hello" or "world.stats" or "world.types" or "world.type" or "world.entities" or "world.entity" or
                    "world.get" or "world.pick" or "world.pick_box" or "world.view_to_world" or "world.world_to_view");
                string output = "";
                var structural = method is "world.play" or "world.stop" or "world.reset" or "world.restore" or "world.destroy" or "world.add_component" or "world.remove_component" or "world.record_insert" or "world.record_remove";
                if (structural)
                {
                    Selection.PreserveThrough(Session, () => output = Session.DispatchDevelopment(command.Request));
                    Session.InvalidateDevelopmentBindings();
                }
                else output = Session.DispatchDevelopment(command.Request);
                Synchronize();
                if (notification) { if (method == "world.hello") Hello = true; command.Reply.TrySetResult(""); return; }
                response = JsonNode.Parse(output)!.AsObject();
                if (response["result"] is JsonObject result)
                {
                    Context(result);
                    if (method == "world.hello")
                    {
                        Hello = true;
                        var value = result["value"]!.AsObject();
                        value["capture_directory"] = Captures.Root;
                        value["capabilities"] = new JsonObject { ["world"] = true, ["inspector"] = true, ["editor"] = true,
                            ["transport"] = true, ["capture"] = true, ["save"] = Saves };
                    }
                }
                else if (response["error"]?["data"] is JsonObject data) Context(data);
            }
            else if (method == "inspector.tree") response = Result(id, Inspector.ChannelTree());
            else if (method == "inspector.get")
            {
                if (p.ContainsKey("generation")) RequireGeneration(p);
                var domain = p["domain"]?.GetValue<string>() ?? "held-view";
                if (domain is not ("held-view" or "live-storage")) throw new ArgumentException("Unsupported value domain.");
                response = Result(id, Inspector.ChannelGet(Text(p, "path"), domain == "live-storage"));
            }
            else if (method == "inspector.report") response = Result(id, Inspector.ChannelReport());
            else if (method == "editor.selection") response = Result(id, new JsonObject { ["ids"] = Ids(), ["generation"] = Inspector.ChannelGeneration });
            else if (method == "editor.history") response = Result(id, HistoryState());
            else if (method == "inspector.set")
            {
                RequireGeneration(p);
                if (!p.ContainsKey("value")) throw new ArgumentException("Missing value.");
                possiblyApplied = true;
                var receipt = Inspector.ChannelSet(Text(p, "path"), p["value"], Generation(p));
                var failed = receipt["outcome"]?.GetValue<string>() is "refused" or "stale_target" or "failed_possibly_partial";
                response = failed ? Error(id, "Inspector write was not fully applied.", result: (PixieResult)(receipt["pixie_result"]?.GetValue<int>() ?? 1),
                    nativeError: receipt["pixie_last_error"]?.GetValue<string>(), possiblyApplied: receipt["possibly_applied"]?.GetValue<bool>() == true, receipt: receipt)
                    : Result(id, receipt);
            }
            else if (method == "editor.do")
            {
                RequireGeneration(p);
                var gesture = Text(p, "command");
                var equals = gesture.IndexOf('=');
                var verb = equals < 0 ? gesture : gesture[..equals].TrimEnd('+');
                if (!Gestures.Contains(verb)) throw new ArgumentException(Saves
                    ? "Disabled editor gesture; export/build/import/drop/font/image are not available."
                    : "Disabled editor gesture; save (left out of this build)/export/build/import/drop/font/image are not available.");
                if (verb == "select")
                {
                    var target = equals < 0 ? "" : gesture[(equals + 1)..];
                    if (target.Length != 16 || !ulong.TryParse(target, NumberStyles.HexNumber, CultureInfo.InvariantCulture, out var sceneId) || sceneId == 0)
                        throw new ArgumentException("Channel selection requires a16-digit hexadecimal scene ID.");
                }
                var before = HistoryState();
                possiblyApplied = true;
                var diagnostics = Inspector.ChannelGesture(() => RunCommand(gesture)); // The same --do handler, once.
                var ran = diagnostics["handler_returned"]?.GetValue<bool>() == true;
                Synchronize();
                var receipt = new JsonObject { ["handler_returned"] = ran, ["history_before"] = before, ["history_after"] = HistoryState(),
                    ["selection"] = Ids(), ["outcome"] = diagnostics["outcome"]?.DeepClone(), ["diagnostics"] = diagnostics, ["unsaved"] = Session.Unsaved };
                var failed = !ran || diagnostics["outcome"]?.GetValue<string>() == "failed_possibly_partial";
                response = !failed ? Result(id, receipt) : Error(id, "Editor gesture refused; inspect history/readback before retrying.",
                    result: (PixieResult)(diagnostics["pixie_result"]?.GetValue<int>() ?? 1), nativeError: diagnostics["pixie_last_error"]?.GetValue<string>(), possiblyApplied: true, receipt: receipt);
            }
            else response = Error(id, "Unknown or disabled development method.", -32601, PixieResult.ErrorNotFound);
            Complete(command, notification ? "" : response.ToJsonString(), id, possiblyApplied);
        }
        catch (Exception e)
        {
            var native = e as EngineException;
            var response = Error(id, e.Message, e is WireException wire ? wire.Code : e is JsonException ? -32700 :
                e is ArgumentException or FormatException ? -32602 : -32000,
                native?.Result ?? PixieResult.ErrorArgument, native?.NativeError, possiblyApplied);
            Complete(command, notification ? "" : response.ToJsonString(), id, possiblyApplied);
        }
    }
    private JsonObject HistoryState() => new() { ["undo_count"] = History.UndoCount, ["redo_count"] = History.RedoCount,
        ["undo_name"] = History.UndoName, ["redo_name"] = History.RedoName };
    private void Complete(EditorPipe.Command command, string response, JsonNode? id, bool possiblyApplied)
    {
        if (Encoding.UTF8.GetByteCount(response) >= 4 * 1024 * 1024)
            response = Error(id, "Response exceeds4MiB; do not retry mutations automatically.", result: PixieResult.ErrorBuffer, possiblyApplied: possiblyApplied).ToJsonString();
        command.Reply.TrySetResult(response);
    }
    public void ReportRenderFailure(EngineException error) => RenderFailure = error;
    public void AfterFrame(int width, int height, bool viewportVisible)
    {
        if (Pending is not { } capture || Frame < capture.ReadyFrame) return;
        Pending = null;
        JsonObject response;
        try
        {
            if (RenderFailure != null) throw RenderFailure;
            if (width <= 0 || height <= 0) throw new EngineException("Editor framebuffer is unavailable.", PixieResult.ErrorGl);
            Captures.Check();
            Screenshot.Save(capture.Path, width, height, exclusive: true);
            response = Result(capture.Id, new JsonObject { ["path"] = capture.Path, ["width"] = width, ["height"] = height,
                ["alpha"] = 1.0, ["steps"] = Session.Stats.Steps, ["scope"] = "editor-window", ["viewport_visible"] = viewportVisible,
                ["requested_frame"] = capture.ReadyFrame - 1, ["rendered_frame"] = Frame });
        }
        catch (Exception e)
        {
            var native = e as EngineException;
            response = Error(capture.Id, e.Message, result: native?.Result ?? PixieResult.ErrorIo,
                nativeError: native?.NativeError, possiblyApplied: e is IOException);
        }
        Complete(capture.Command, capture.Notification ? "" : response.ToJsonString(), capture.Id, false);
    }
    public void Dispose()
    {
        Pending?.Command.Reply.TrySetCanceled(); Pending = null;
        Pipe.Dispose();
    }
}
