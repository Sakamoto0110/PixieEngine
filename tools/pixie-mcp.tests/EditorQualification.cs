using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;
using System.Text.Json.Nodes;

internal static class EditorQualification
{
    public static async Task Run(string[] args)
    {
        if (args.Length != 5) throw new ArgumentException("usage: --editor <editor.exe> <project> <mcp.dll> <captures> <receipt.json>");
        var records = new JsonArray();
        void Check(bool condition, string name)
        {
            records.Add(new JsonObject { ["case"] = name, ["passed"] = condition });
            if (!condition) throw new InvalidOperationException(name);
        }
        try
        {
            await using var client = await McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions
            {
                Name = "Pixie Inspector qualification", Command = "dotnet",
                Arguments = [Path.GetFullPath(args[2]), "--host", Path.GetFullPath(args[0]), "--project", Path.GetFullPath(args[1]),
                    "--captures", Path.Combine(Path.GetFullPath(args[3]), "editor espaço-ação"), "--kind", "editor"]
            }));
            async Task<(JsonNode Receipt, CallToolResult Tool)> Call(string name, JsonObject? p = null)
            {
                var result = await client.CallToolAsync(name, new Dictionary<string, object?> { ["parameters"] = p ?? new JsonObject() });
                var text = result.Content.OfType<TextContentBlock>().FirstOrDefault()?.Text ?? throw new IOException("Missing tool receipt.");
                return (JsonNode.Parse(text) ?? throw new IOException(text), result);
            }
            async Task<JsonNode> Good(string name, JsonObject? p = null)
            {
                var (receipt, result) = await Call(name, p);
                if (result.IsError == true) throw new IOException(receipt.ToJsonString());
                return receipt["result"]!["value"]!;
            }
            async Task<long> Generation() => (await Good("inspector.tree"))["generation"]!.GetValue<long>();
            async Task<JsonNode> Gesture(string command) => await Good("editor.do", new() { ["generation"] = await Generation(), ["command"] = command });
            async Task<double> X(string id) => (await Good("world.get", new() { ["id"] = id, ["field"] = "Transform2D.position" }))["value"]![0]!.GetValue<double>();
            var tools = await client.ListToolsAsync();
            Check(tools.Any(t => t.Name == "inspector.set") && tools.Any(t => t.Name == "editor.history") && !tools.Any(t => t.Name == "world.save"), "editor-tools-discovered-without-save");
            var hello = await Good("world.hello");
            Check(hello["capabilities"]?["inspector"]?.GetValue<bool>() == true && hello["pixie_api_version"]?.GetValue<int>() == 18, "editor-native-api-and-inspector-capability");
            var entities = (await Good("world.entities"))["entities"]!.AsArray();
            var first = entities[0]!["id"]!.GetValue<string>();
            await Gesture("select=" + first);
            var tree = await Good("inspector.tree");
            var generation = tree["generation"]!.GetValue<long>();
            Check(tree["rows"]!.AsArray().Any(n => n?["path"]?.GetValue<string>() == "Transform2D.positionX"), "registered-inspector-node-tree");
            var before = (await Good("editor.history"))["undo_count"]!.GetValue<int>();
            var written = await Good("inspector.set", new() { ["generation"] = generation, ["path"] = "Transform2D.positionX", ["value"] = 42 });
            Check(written["handler_returned"]?.GetValue<bool>() == true && written["outcome"]?.GetValue<string>() == "applied" && await X(first) == 42, "existing-inspector-handler-writes-native-world");
            Check((await Good("editor.history"))["undo_count"]!.GetValue<int>() == before + 1, "inspector-write-records-one-history-gesture");
            await Good("inspector.set", new() { ["generation"] = generation, ["path"] = "Transform2D.rotation", ["value"] = 90 });
            var storageAngle = (await Good("world.get", new() { ["id"] = first, ["field"] = "Transform2D.rotation" }))["value"]!.GetValue<double>();
            var viewAngle = (await Good("inspector.get", new() { ["path"] = "Transform2D.rotation" }))["values"]![0]!.GetValue<double>();
            var liveAngle = (await Good("inspector.get", new() { ["path"] = "Transform2D.rotation", ["domain"] = "live-storage" }))["values"]![0]!.GetValue<double>();
            Check(Math.Abs(storageAngle - Math.PI / 2) < 1e-8 && Math.Abs(viewAngle - 90) < 1e-8 && Math.Abs(liveAngle - storageAngle) < 1e-8, "held-view-and-live-storage-unit-domains-distinguished");
            await Gesture("undo");
            Check(Math.Abs((await Good("world.get", new() { ["id"] = first, ["field"] = "Transform2D.rotation" }))["value"]!.GetValue<double>()) < 1e-9, "existing-editor-undo-restores-native-value");
            await Gesture("redo");
            Check((await Good("editor.history"))["redo_count"]!.GetValue<int>() == 0, "existing-editor-redo-history-state");
            var historyCount = (await Good("editor.history"))["undo_count"]!.GetValue<int>();
            await Good("world.set", new() { ["id"] = first, ["field"] = "Transform2D.position", ["value"] = new JsonArray(9, 11) });
            Check((await Good("editor.history"))["undo_count"]!.GetValue<int>() == historyCount, "world-write-does-not-enter-editor-history");
            Check((await Good("inspector.get", new() { ["path"] = "Transform2D.positionX" }))["values"]![0]!.GetValue<double>() == 9, "inspector-refresh-observes-world-write");
            var second = (await Good("world.create", new() { ["name"] = "second" }))["id"]!.GetValue<string>();
            await Good("world.add_component", new() { ["id"] = second, ["type"] = "Transform2D" });
            await Gesture("select+=" + second);
            var mixed = await Good("inspector.get", new() { ["path"] = "Transform2D.positionX" });
            Check(mixed["mixed"]?.GetValue<bool>() == true && mixed["values"]!.AsArray().Count == 2, "mixed-inspector-row-holds-each-selected-value");
            var stale = await Call("inspector.set", new() { ["generation"] = generation, ["path"] = "Transform2D.positionX", ["value"] = 999 });
            Check(stale.Tool.IsError == true && await X(first) == 9 && await X(second) == 0, "stale-selection-generation-refused-before-write");
            var multiBefore = (await Good("editor.history"))["undo_count"]!.GetValue<int>();
            await Good("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "Transform2D.positionX", ["value"] = 55 });
            Check(await X(first) == 55 && await X(second) == 55 && (await Good("editor.history"))["undo_count"]!.GetValue<int>() == multiBefore + 1, "multi-object-inspector-write-is-one-real-history-gesture");
            await Gesture("undo");
            Check(await X(first) == 9 && await X(second) == 0, "multi-object-undo-restores-distinct-values");
            await Gesture("redo");
            foreach (var command in new[] { "save", "export=elsewhere", "font=outside.ttf", "image=outside.png", "drop=file@0,0", "build" })
            {
                var rejected = await Call("editor.do", new() { ["generation"] = await Generation(), ["command"] = command });
                Check(rejected.Tool.IsError == true, "filesystem-alias-disabled-" + command.Split('=')[0]);
            }
            Check((await Call("editor.do", new() { ["generation"] = await Generation(), ["command"] = "select=second" })).Tool.IsError == true, "channel-selection-rejects-names");
            var historyAfter = (await Good("editor.history"))["undo_count"]!.GetValue<int>();
            var token = (await Good("world.snapshot", new() { ["ids"] = new JsonArray(first) }))["token"]!.GetValue<string>();
            await Good("world.set", new() { ["id"] = first, ["field"] = "Transform2D.position", ["value"] = new JsonArray(99, 11) });
            await Good("world.restore", new() { ["token"] = token });
            Check(await X(first) == 55 && (await Good("editor.selection"))["ids"]![0]!.GetValue<string>() == first, "world-restore-rebinds-selection-by-scene-id");
            Check((await Good("editor.history"))["undo_count"]!.GetValue<int>() == historyAfter, "world-restore-has-no-parallel-editor-history");
            await Gesture("select=" + first);
            await Gesture("add=Text");
            var originalText = (await Good("world.get", new() { ["id"] = first, ["field"] = "Text.text" }))["value"]!.GetValue<string>();
            await Good("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "Text.text", ["value"] = "café from the existing handler" });
            var textRow = await Good("inspector.get", new() { ["path"] = "Text.text" });
            Check(textRow["source"]!.GetValue<string>() == "editor-custom-row" && textRow["domain"]!.GetValue<string>() == "live-native-storage" &&
                (await Good("world.get", new() { ["id"] = first, ["field"] = "Text.text" }))["value"]!.GetValue<string>().StartsWith("café"), "custom-text-handler-and-storage-domain-identified");
            await Gesture("undo");
            Check((await Good("world.get", new() { ["id"] = first, ["field"] = "Text.text" }))["value"]!.GetValue<string>() == originalText, "custom-text-undo-through-existing-authoring-history");
            await Gesture("redo");
            await Gesture("add=Path");
            var points = new JsonArray(new JsonArray(0, 0), new JsonArray(20, 30));
            await Good("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "Path.points", ["value"] = points });
            Check(JsonNode.DeepEquals((await Good("world.get", new() { ["id"] = first, ["field"] = "Path.points" }))["value"], points), "custom-point-list-reuses-existing-set-handler");
            await Gesture("add=CompoundCollider2D");
            await Gesture("shape=0:radius:17");
            Check((await Good("world.get", new() { ["id"] = first, ["field"] = "CompoundCollider2D.shapes", ["index"] = 0, ["member"] = "radius" }))["value"]!.GetValue<double>() == 17,
                "existing-record-gesture-writes-native-record-field");
            var record = await Good("inspector.get", new() { ["path"] = "CompoundCollider2D.shapes[0].radius" });
            Check(record["domain"]!.GetValue<string>() == "held-view" && record["values"]![0]!.GetValue<double>() == 17, "registered-record-inspector-readback");
            var recordFailure = await Call("editor.do", new() { ["generation"] = await Generation(), ["command"] = "shape=0:radius:invalid" });
            Check(recordFailure.Tool.IsError == true && recordFailure.Receipt["error"]?["data"]?["receipt"]?["diagnostics"]?["exception"] != null,
                "record-gesture-command-local-error-not-hidden-by-handler");
            await Gesture("add=ParticleSpawner");
            var referenced = (await Good("world.create", new() { ["name"] = "reference-target" }))["id"]!.GetValue<string>();
            await Good("world.add_component", new() { ["id"] = referenced, ["type"] = "ParticleSystem" });
            var wrongTarget = await Call("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "ParticleSpawner.target", ["value"] = referenced });
            Check(wrongTarget.Tool.IsError == true && wrongTarget.Receipt["error"]?["data"]?["possibly_applied"]?.GetValue<bool>() == false,
                "native-references-constraint-refused-before-inspector-write");
            await Good("world.add_component", new() { ["id"] = referenced, ["type"] = "Transform2D" });
            await Good("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "ParticleSpawner.target", ["value"] = referenced });
            Check((await Good("world.get", new() { ["id"] = first, ["field"] = "ParticleSpawner.target" }))["value"]!.GetValue<string>() == referenced,
                "inspector-entity-reference-uses-scene-id-through-existing-writer");
            Check((await Call("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "ParticleSpawner.target", ["value"] = "reference-target #1" })).Tool.IsError == true,
                "inspector-entity-reference-rejects-runtime-index-labels");
            await Good("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "ParticleSpawner.target", ["value"] = null });
            await Good("world.play");
            await Good("world.pause");
            Check((await Good("world.step", new() { ["count"] = 3 }))["steps_run"]!.GetValue<int>() == 3, "editor-world-step-drained-while-paused");
            var stats = await Good("world.stats");
            var capture = await Call("world.capture", new() { ["name"] = "inspector-frame" });
            Check(capture.Tool.IsError != true && capture.Tool.Content.OfType<ImageContentBlock>().Count() == 1, "editor-mcp-returns-full-ui-png-image");
            var image = capture.Receipt["result"]!["value"]!;
            Check(image["rendered_frame"]!.GetValue<long>() > image["requested_frame"]!.GetValue<long>() && image["alpha"]!.GetValue<double>() == 1, "editor-capture-completes-on-subsequent-full-ui-frame");
            var afterCapture = await Good("world.stats");
            Check(JsonNode.DeepEquals(stats["paused"], afterCapture["paused"]) && JsonNode.DeepEquals(stats["time_scale"], afterCapture["time_scale"]) && JsonNode.DeepEquals(stats["steps"], afterCapture["steps"]), "editor-capture-does-not-change-pause-scale-or-steps");
            Check((await Call("world.capture", new() { ["name"] = "../escape" })).Tool.IsError == true, "editor-capture-traversal-refused");
            Check((await Call("world.capture", new() { ["name"] = "inspector-frame" })).Tool.IsError == true, "editor-capture-overwrite-refused");
            Check((await Good("inspector.report"))["reports"] is JsonArray, "inspector-creation-and-runtime-report-readable");
        }
        finally
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[4]))!);
            await File.WriteAllTextAsync(args[4], records.ToJsonString(new() { WriteIndented = true }));
        }
        Console.WriteLine($"PASS: {records.Count} editor/Inspector cases using the official MCP client.");
    }
}
