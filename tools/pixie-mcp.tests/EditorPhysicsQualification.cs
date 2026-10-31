using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;
using System.Text.Json.Nodes;

// The no-code physics authoring workflow, driven through the editor's existing gestures over MCP:
// presets, Inspector units and writes, collider handles, named layers, compound shapes, refused
// motion owners, debug views, Play/Stop, invalid settings and recovery, save/reopen, a second
// session and a legacy migration. Programmatic gestures prove native state and history, not
// physical-pointer usability or the owner's review.
internal static class EditorPhysicsQualification
{
    public static async Task Run(string[] args)
    {
        if (args.Length != 7)
            throw new ArgumentException("usage: --editor-physics <editor.exe> <empty project> <second empty project> <legacy project> <mcp.dll> <captures> <receipt.json>");
        var (editor, project, second, legacy, mcp, captures) = (Path.GetFullPath(args[0]), Path.GetFullPath(args[1]),
            Path.GetFullPath(args[2]), Path.GetFullPath(args[3]), Path.GetFullPath(args[4]), Path.GetFullPath(args[5]));
        var records = new JsonArray();
        void Check(bool condition, string name, JsonNode? detail = null)
        {
            var record = new JsonObject { ["case"] = name, ["passed"] = condition };
            if (detail != null) record["detail"] = detail.DeepClone();
            records.Add(record);
            if (!condition) throw new InvalidOperationException(name + (detail == null ? "" : ": " + detail.ToJsonString()));
        }
        Task<McpClient> Connect(string folder, string captureFolder) => McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions
        {
            Name = "Pixie physics authoring qualification", Command = "dotnet", StandardErrorLines = Console.Error.WriteLine,
            Arguments = [mcp, "--host", editor, "--project", folder, "--captures", Path.Combine(captures, captureFolder), "--kind", "editor"]
        }));
        static async Task<(JsonNode Receipt, CallToolResult Tool)> Call(McpClient client, string name, JsonObject? p = null)
        {
            var result = await client.CallToolAsync(name, new Dictionary<string, object?> { ["parameters"] = p ?? new JsonObject() });
            var text = result.Content.OfType<TextContentBlock>().FirstOrDefault()?.Text ?? throw new IOException("Missing tool receipt.");
            return (JsonNode.Parse(text) ?? throw new IOException(text), result);
        }
        static async Task<JsonNode> Good(McpClient client, string name, JsonObject? p = null)
        {
            var (receipt, result) = await Call(client, name, p);
            if (result.IsError == true) throw new IOException(name + ": " + receipt.ToJsonString());
            return receipt["result"]!["value"]!;
        }
        static async Task<JsonNode> Gesture(McpClient client, string command) => await Good(client, "editor.do", new()
            { ["generation"] = (await Good(client, "inspector.tree"))["generation"]!.GetValue<long>(), ["command"] = command });
        // A gesture the editor may refuse: the receipt either way, never an exception.
        static async Task<(JsonNode Receipt, CallToolResult Tool)> TryGesture(McpClient client, string command) => await Call(client, "editor.do", new()
            { ["generation"] = (await Good(client, "inspector.tree"))["generation"]!.GetValue<long>(), ["command"] = command });
        static async Task<string> Create(McpClient client, string preset) =>
            (await Gesture(client, preset))["selection"]![0]!.GetValue<string>();
        static async Task<JsonNode?> Get(McpClient client, string id, string field)
        {
            var (receipt, result) = await Call(client, "world.get", new() { ["id"] = id, ["field"] = field });
            return result.IsError == true ? null : receipt["result"]!["value"]!["value"];
        }
        static async Task<double> Number(McpClient client, string id, string field) => (await Get(client, id, field))!.GetValue<double>();
        static async Task<(double X, double Y)> Pair(McpClient client, string id, string field)
        {
            var value = (await Get(client, id, field))!.AsArray();
            return (value[0]!.GetValue<double>(), value[1]!.GetValue<double>());
        }
        static async Task<int> Undos(McpClient client) => (await Good(client, "editor.history"))["undo_count"]!.GetValue<int>();
        static async Task<JsonNode> Set(McpClient client, string path, JsonNode value) => await Good(client, "inspector.set", new()
            { ["generation"] = (await Good(client, "inspector.tree"))["generation"]!.GetValue<long>(), ["path"] = path, ["value"] = value });
        static async Task Select(McpClient client, string id) => await Gesture(client, "select=" + id);
        static async Task<int> Entities(McpClient client) => (await Good(client, "world.entities"))["entities"]!.AsArray().Count;
        async Task<double> FallAfterPlay(McpClient client, string id, int steps)
        {
            var before = await Pair(client, id, "Transform2D.position");
            await Gesture(client, "play");
            await Good(client, "world.pause");
            await Good(client, "world.step", new() { ["count"] = steps });
            var during = await Pair(client, id, "Transform2D.position");
            await Gesture(client, "stop");
            return during.Y - before.Y;
        }
        string box, platform, compound, sensor, obstacle, settings;
        double density = 4, mass = 3;
        try
        {
            await using (var client = await Connect(project, "authoring"))
            {
                var hello = await Good(client, "world.hello");
                Check(hello["capabilities"]?["editor"]?.GetValue<bool>() == true && hello["stats"]?["entities"]?.GetValue<int>() == 0,
                    "empty-no-code-project-opens-in-editor");

                // Presets: one history step each; the world preset reuses the scene's single settings entity.
                settings = await Create(client, "world");
                var gravity = await Pair(client, settings, "PhysicsSettings2D.gravity");
                Check(gravity == (0, 1100) && await Number(client, settings, "PhysicsSettings2D.unitsPerMeter") == 32
                    && await Number(client, settings, "PhysicsSettings2D.substeps") == 4, "world-preset-has-documented-defaults");
                var count = await Entities(client);
                Check(await Create(client, "world") == settings && await Entities(client) == count, "second-world-preset-selects-the-existing-settings");
                var undos = await Undos(client);
                obstacle = await Create(client, "collider");
                box = await Create(client, "dynamic");
                platform = await Create(client, "kinematic");
                compound = await Create(client, "compoundbox");
                sensor = await Create(client, "sensor");
                Check(await Undos(client) == undos + 5, "each-preset-is-one-history-step");
                Check(await Get(client, obstacle, "RigidBody2D.type") == null && await Number(client, box, "RigidBody2D.type") == 2
                    && await Number(client, platform, "RigidBody2D.type") == 1 && await Get(client, compound, "CompoundCollider2D.enabled") != null
                    && (await Get(client, sensor, "Collider2D.sensor"))!.GetValue<bool>(), "presets-create-static-dynamic-kinematic-compound-and-sensor",
                    new JsonObject { ["dynamic"] = 2, ["kinematic"] = 1 });

                // Lay the room out with the editor's move gesture: a wide static floor under the bodies.
                foreach (var (id, x, y) in new[] { (obstacle, 0.0, 150.0), (box, 0.0, -40.0), (platform, -260.0, 60.0), (compound, 160.0, -40.0), (sensor, -140.0, 110.0) })
                {
                    await Select(client, id);
                    var at = await Pair(client, id, "Transform2D.position");
                    await Gesture(client, FormattableString.Invariant($"move={x - at.X},{y - at.Y}"));
                    Check(await Pair(client, id, "Transform2D.position") == (x, y), "move-gesture-places-" + id);
                }
                await Select(client, obstacle);
                var floorWrite = await Set(client, "Collider2D.halfSizeX", 800);
                var floorHalf = await Pair(client, obstacle, "Collider2D.halfSize");
                var floorView = (await Good(client, "inspector.get", new() { ["path"] = "Collider2D.halfSizeX" }))["values"]![0]!.GetValue<double>();
                Check(floorWrite["outcome"]?.GetValue<string>() == "applied" && floorHalf.X == 400 && floorView == 800,
                    "inspector-size-is-the-full-width-over-native-half-extents", new JsonObject { ["half"] = floorHalf.X, ["shown"] = floorView });

                // Inspector units and mass: density in kg/m^2 and a mass override in kg, each one undoable step.
                await Select(client, box);
                undos = await Undos(client);
                await Set(client, "Collider2D.density", density);
                await Set(client, "RigidBody2D.massOverride", mass);
                Check(await Number(client, box, "Collider2D.density") == density && await Number(client, box, "RigidBody2D.massOverride") == mass
                    && await Undos(client) == undos + 2, "density-and-mass-override-edit-native-fields-as-history-steps");
                await Gesture(client, "undo");
                Check(await Number(client, box, "RigidBody2D.massOverride") == 0, "mass-override-undo-restores-computed-mass");
                await Gesture(client, "redo");
                Check(await Number(client, box, "RigidBody2D.massOverride") == mass, "mass-override-redo");
                // Invalid input: the Inspector clamps a negative density to its range minimum, zero. A
                // dynamic body without positive-density geometry is disabled rather than given a unit
                // mass, and undo recovers it.
                var clamped = await Set(client, "Collider2D.density", -5);
                Check(clamped["outcome"]?.GetValue<string>() == "applied" && await Number(client, box, "Collider2D.density") == 0,
                    "negative-density-input-is-clamped-to-the-range-minimum");
                Check(await FallAfterPlay(client, box, 30) == 0, "zero-density-dynamic-body-is-disabled-not-given-a-unit-mass");
                await Gesture(client, "undo");
                Check(await Number(client, box, "Collider2D.density") == density && await FallAfterPlay(client, box, 30) > 20,
                    "undo-recovers-the-disabled-body");

                // Collider handles (shape -1 is the root collider): resize and rotate are undoable; cancel writes nothing.
                var half = await Pair(client, box, "Collider2D.halfSize");
                undos = await Undos(client);
                await Gesture(client, "colliderdrag=-1:resize:40,-6");
                var resized = await Pair(client, box, "Collider2D.halfSize");
                Check(resized != half && await Undos(client) == undos + 1, "collider-resize-handle-changes-native-geometry",
                    new JsonObject { ["before"] = new JsonArray(half.X, half.Y), ["after"] = new JsonArray(resized.X, resized.Y) });
                await Gesture(client, "undo");
                Check(await Pair(client, box, "Collider2D.halfSize") == half, "collider-resize-undo-restores-geometry");
                await Gesture(client, "redo");
                Check(await Pair(client, box, "Collider2D.halfSize") == resized, "collider-resize-redo");
                await Gesture(client, "colliderdrag=-1:rotate:60,-80");
                var rotation = await Number(client, box, "Collider2D.rotation");
                Check(rotation != 0, "collider-rotate-handle-changes-local-rotation", JsonValue.Create(rotation));
                undos = await Undos(client);
                await Gesture(client, "colliderdrag=-1:cancel:90,10");
                Check(await Pair(client, box, "Collider2D.halfSize") == resized && await Undos(client) == undos, "cancelled-collider-drag-writes-nothing");

                // Named layers: checkboxes edit native bits; the last membership cannot be cleared.
                await Gesture(client, "layername=2:Player");
                Check(await Number(client, box, "Collider2D.category") == 1, "naming-a-layer-does-not-rewrite-filters");
                await Gesture(client, "layer=category:2:true");
                Check(await Number(client, box, "Collider2D.category") == 3, "layer-checkbox-sets-the-named-bit");
                await Gesture(client, "layer=category:1:false");
                Check(await Number(client, box, "Collider2D.category") == 2, "layer-checkbox-clears-the-unnamed-bit");
                undos = await Undos(client);
                await TryGesture(client, "layer=category:2:false");
                Check(await Number(client, box, "Collider2D.category") == 2 && await Undos(client) == undos, "last-layer-membership-is-kept");
                await Gesture(client, "undo");
                await Gesture(client, "undo");
                Check(await Number(client, box, "Collider2D.category") == 1, "layer-edits-undo-to-the-original-bits");

                // Compound shapes: add an item, edit it through the record row, undo. The custom row reads
                // the native record list itself.
                await Select(client, compound);
                async Task<JsonArray> Shapes() => (await Good(client, "inspector.get", new() { ["path"] = "CompoundCollider2D.shapes" }))["values"]![0]!.AsArray();
                var items = (await Shapes()).Count;
                await Gesture(client, "addshape");
                Check((await Shapes()).Count == items + 1, "compound-addshape-appends-an-item", JsonValue.Create(items));
                await Gesture(client, FormattableString.Invariant($"shape={items}:density:2"));
                Check((await Shapes())[items]!["density"]!.GetValue<double>() == 2, "compound-item-field-edit");
                await Gesture(client, "undo");
                await Gesture(client, "undo");
                Check((await Shapes()).Count == items, "compound-item-undo-removes-it");

                // One motion owner: a motor cannot be added to a dynamic body.
                await Select(client, box);
                undos = await Undos(client);
                await TryGesture(client, "add=CharacterMotor2D");
                Check(await Get(client, box, "CharacterMotor2D.radius") == null && await Undos(client) == undos, "motor-on-a-dynamic-body-is-refused");

                // Debug views and Play/Stop: bodies fall, the scene's poses come back on Stop.
                await Gesture(client, "physicsdebug=127");
                var debug = await Call(client, "world.capture", new() { ["name"] = "authoring-debug" });
                Check(debug.Tool.IsError != true && debug.Tool.Content.OfType<ImageContentBlock>().Count() == 1, "debug-view-capture-in-edit");
                count = await Entities(client);
                var authored = await Pair(client, box, "Transform2D.position");
                var fall = await FallAfterPlay(client, box, 45);
                Check(fall > 50 && await Pair(client, box, "Transform2D.position") == authored && await Entities(client) == count,
                    "play-simulates-and-stop-restores-authored-poses", JsonValue.Create(fall));
                await Gesture(client, "play");
                await Good(client, "world.pause");
                await Good(client, "world.step", new() { ["count"] = 70 });
                var played = await Call(client, "world.capture", new() { ["name"] = "play-debug" });
                var settled = await Pair(client, box, "Transform2D.position");
                await Gesture(client, "stop");
                Check(played.Tool.IsError != true && settled.Y > 60 && settled.Y < 150, "dynamic-box-lands-on-the-authored-floor",
                    new JsonArray(settled.X, settled.Y));

                // Duplicate world settings disable the service; undoing the duplicate recovers it.
                await Select(client, settings);
                await Gesture(client, "duplicate");
                Check(await FallAfterPlay(client, box, 30) == 0, "duplicate-world-settings-disable-simulation");
                await Gesture(client, "undo");
                Check(await FallAfterPlay(client, box, 30) > 20, "undoing-the-duplicate-settings-recovers-simulation");

                await Gesture(client, "save");
                var saved = await File.ReadAllTextAsync(Path.Combine(project, "scenes", "main.scene"));
                Check(saved.Contains(box) && saved.Contains("Player") && saved.Contains("CompoundCollider2D")
                    && !saved.Contains("\"PreviousTransform2D\""), "save-writes-authored-physics-only");
            }

            // Reopen: the saved authored data comes back, and a second editor session stays isolated.
            await using (var client = await Connect(project, "reopened"))
            await using (var other = await Connect(second, "second session"))
            {
                Check(await Number(client, box, "Collider2D.density") == density && await Number(client, box, "RigidBody2D.massOverride") == mass
                    && await Number(client, platform, "RigidBody2D.type") == 1 && await Get(client, compound, "CompoundCollider2D.enabled") != null,
                    "reopen-restores-bodies-materials-and-compounds");
                var otherBox = await Create(other, "dynamic");
                await Gesture(other, "play");
                await Good(other, "world.pause");
                await Good(other, "world.step", new() { ["count"] = 30 });
                var stats = await Good(client, "world.stats");
                Check((await Pair(other, otherBox, "Transform2D.position")).Y > 20 && stats["mode"]!.GetValue<string>() == "edit"
                    && stats["steps"]!.GetValue<int>() == 0, "second-editor-session-steps-without-touching-the-first");
                await Gesture(other, "stop");
                Check(await FallAfterPlay(client, box, 30) > 20, "reopened-scene-plays");
            }

            // Version 1 motors keep their fall: gravityScale = legacy gravity / 1100, differing and zero
            // values stay distinct, and Collider2D-only entities stay static.
            await using (var client = await Connect(legacy, "legacy"))
            {
                var scene = JsonNode.Parse(await File.ReadAllTextAsync(Path.Combine(legacy, "scenes", "main.scene")))!;
                var observed = new JsonArray();
                var converted = 0;
                foreach (var motor in scene["entities"]!.AsArray().Where(e => e!["components"]!["CharacterMotor2D"]?["v"]?.GetValue<int>() == 1))
                {
                    var legacyGravity = motor!["components"]!["CharacterMotor2D"]!["gravity"]!.GetValue<double>();
                    var scale = await Number(client, motor["id"]!.GetValue<string>(), "CharacterMotor2D.gravityScale");
                    observed.Add(new JsonObject { ["gravity"] = legacyGravity, ["scale"] = scale });
                    converted += Math.Abs(scale - legacyGravity / 1100) < 1e-12 ? 1 : 0;
                }
                var colliders = scene["entities"]!.AsArray().Where(e => e!["components"]!["Collider2D"] != null).Select(e => e!["id"]!.GetValue<string>()).ToArray();
                var statics = 0;
                foreach (var collider in colliders) statics += await Get(client, collider, "RigidBody2D.type") == null ? 1 : 0;
                Check(observed.Count >= 2 && converted == observed.Count, "legacy-v1-motor-gravities-convert-to-distinct-scales", observed);
                Check(colliders.Length > 0 && statics == colliders.Length, "legacy-collider-only-entities-stay-static");
            }
        }
        finally
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[6]))!);
            await File.WriteAllTextAsync(args[6], records.ToJsonString(new() { WriteIndented = true }));
        }
        Console.WriteLine($"PASS: {records.Count} no-code physics authoring cases using the official MCP client.");
    }
}
