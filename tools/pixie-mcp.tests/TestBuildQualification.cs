using System.Diagnostics;
using System.Text.Json.Nodes;
using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;

internal static class TestBuildQualification
{
    public static async Task Run(string[] args)
    {
        if (args.Length != 4 && (args.Length != 5 || args[4] is not ("--game-commands" or "--full-game-commands")))
            throw new ArgumentException("usage: tests --test-build <manifest.json> <private SDK> <PixieMcp.dll> <new evidence folder> [--game-commands | --full-game-commands]");
        var gameCommands = args.Length == 5;
        var fullCommands = gameCommands && args[4] == "--full-game-commands";

        var manifest = Path.GetFullPath(args[0]);
        var sdk = Path.GetFullPath(args[1]);
        var bridge = Path.GetFullPath(args[2]);
        var evidence = Path.GetFullPath(args[3]);
        if (Directory.Exists(evidence) || File.Exists(evidence)) throw new IOException("Select a new evidence folder.");
        Directory.CreateDirectory(evidence);
        var bundle = Path.GetDirectoryName(manifest)!;
        var inventory = JsonNode.Parse(await File.ReadAllTextAsync(manifest))!;
        var player = Path.Combine(bundle, inventory["player"]!.GetValue<string>());
        var receipt = new JsonObject();

        try
        {
            var withoutOptIn = new ProcessStartInfo(player) { WorkingDirectory = bundle, UseShellExecute = false, CreateNoWindow = true };
            withoutOptIn.ArgumentList.Add("--quit-after");
            withoutOptIn.ArgumentList.Add("2");
            var pipesBefore = PixiePipes();
            using (var process = Process.Start(withoutOptIn) ?? throw new IOException("Could not launch bundled player."))
            {
                try
                {
                    using var deadline = new CancellationTokenSource(TimeSpan.FromSeconds(15));
                    while (!process.HasExited)
                    {
                        Require(PixiePipes().All(pipesBefore.Contains), "Standalone launch opens no Pixie pipe");
                        await Task.Delay(50, deadline.Token);
                    }
                    Require(process.ExitCode == 0, "Bundled player runs without channel opt-in");
                }
                finally
                {
                    if (!process.HasExited) process.Kill(entireProcessTree: true);
                }
            }
            receipt["standaloneWithoutOptIn"] = true;

            await using (var client = await Open(bridge, manifest, sdk, Path.Combine(evidence, "captures")))
            {
                var tools = await client.ListToolsAsync();
                Require(tools.Any(tool => tool.Name == "world.hello") && tools.Any(tool => tool.Name == "world.capture") &&
                    tools.All(tool => tool.Name != "world.save") && tools.Any(tool => tool.Name.StartsWith("game.", StringComparison.Ordinal)) == gameCommands,
                    "Private bundle exposes the existing player tools");
                receipt["tools"] = tools.Count;
                var hello = Value(await Call(client, "world.hello", new { }));
                Require(hello["engine"]?["sdkBuildId"]?.GetValue<string>() == inventory["sdkBuildId"]?.GetValue<string>(), "Exact SDK tuple at runtime");
                receipt["hello"] = hello.DeepClone();
                Value(await Call(client, "world.pause", new { }));
                var step = Value(await Call(client, "world.step", new { count = 3 }));
                Require(step["steps_run"]?.GetValue<int>() == 3, "Paused simulation advances by three ticks");
                receipt["step"] = step.DeepClone();
                var capture = await Call(client, "world.capture", new { name = "weapons-lab" });
                var image = Value(capture);
                Require(capture.Content.OfType<ImageContentBlock>().Single().DecodedData.Span.StartsWith(new byte[] { 137, 80, 78, 71 }) &&
                    image["steps"]?.GetValue<long>() == step["stats"]?["steps"]?.GetValue<long>(), "Capture returns the matching simulation tick and PNG");
                receipt["capture"] = image.DeepClone();
                if (gameCommands) receipt["gameCommands"] = await CheckGameCommands(client, tools, fullCommands);
                if (fullCommands) receipt["remainingCommands"] = await CheckRemainingCommands(client, tools);
            }

            // A stale or damaged bundle must fail before spawning a host. Work on a disposable copy.
            var changed = Path.Combine(evidence, "changed");
            foreach (var file in Directory.EnumerateFiles(bundle, "*", SearchOption.AllDirectories))
            {
                var destination = Path.Combine(changed, Path.GetRelativePath(bundle, file));
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                File.Copy(file, destination);
            }
            var changedManifest = Path.Combine(changed, Path.GetFileName(manifest));
            var changedScene = inventory["files"]!.AsArray().First(item => item!["role"]!.GetValue<string>() == "scene")!["path"]!.GetValue<string>();
            await File.AppendAllTextAsync(Path.Combine(changed, changedScene), "\n");
            var captures = Path.Combine(evidence, "refused-captures");
            await using (var client = await Open(bridge, changedManifest, sdk, captures))
            {
                var refused = await Call(client, "world.hello", new { });
                Require(refused.IsError == true && !Directory.Exists(captures) &&
                    refused.Content.OfType<TextContentBlock>().Single().Text.Contains("differs from its inventory", StringComparison.Ordinal),
                    "Modified payload is refused before startup");
            }
            receipt["modifiedPayloadRefused"] = true;

            var profile = Path.Combine(sdk, "profiles", "local-development", "Release", "bin", "pixie-player.exe");
            var sdkCaptures = Path.Combine(evidence, "sdk-captures");
            await using (var client = await OpenBridge(bridge, "--host", profile, "--project", bundle,
                "--sdk", sdk, "--captures", sdkCaptures, "--startup-timeout", "20"))
            {
                var hello = Value(await Call(client, "world.hello", new { }));
                Require(hello["engine"]?["sdkBuildId"]?.GetValue<string>() == inventory["sdkBuildId"]?.GetValue<string>(),
                    "The explicit SDK player launch route still works");
            }
            receipt["sdkPlayerLaunch"] = true;
        }
        finally
        {
            await File.WriteAllTextAsync(Path.Combine(evidence, "receipt.json"), receipt.ToJsonString(new() { WriteIndented = true }));
        }
        Console.WriteLine("PASS: private test build launches through MCP, steps, captures and rejects a modified payload.");
    }

    private static Task<McpClient> Open(string bridge, string manifest, string sdk, string captures) =>
        OpenBridge(bridge, "--test-build", manifest, "--sdk", sdk, "--captures", captures, "--startup-timeout", "20");

    private static Task<McpClient> OpenBridge(string bridge, params string[] arguments) =>
        McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions
        {
            Name = "Private test-build qualification", Command = Environment.ProcessPath!, StandardErrorLines = Console.Error.WriteLine,
            Arguments = [bridge, .. arguments]
        }));

    private static HashSet<string> PixiePipes() => Directory.EnumerateFiles(@"\\.\pipe\", "pixie-*")
        .ToHashSet(StringComparer.OrdinalIgnoreCase);

    private static async Task<JsonObject> CheckGameCommands(McpClient client, IList<McpClientTool> tools, bool fullCommands)
    {
        string[] expected = ["game.dev.describe", "game.dev.read_events", "game.player.fire", "game.player.get_status",
            "game.player.release_controls", "game.player.rotate", "game.player.set_motor_power", "game.player.set_requested_rpm"];
        var registered = tools.Where(tool => tool.Name.StartsWith("game.", StringComparison.Ordinal)).Select(tool => tool.Name).ToArray();
        Require(expected.All(registered.Contains) && registered.Length == (fullCommands ? 42 : expected.Length),
            "MCP advertises the expected game bindings");
        var fireTool = tools.Single(tool => tool.Name == "game.player.fire").ProtocolTool;
        Require(fireTool.InputSchema.GetProperty("properties").GetProperty("parameters").GetProperty("properties").GetProperty("input").GetProperty("enum").GetArrayLength() == 3 &&
            fireTool.OutputSchema is not null, "MCP advertises input and output schemas");

        async Task<JsonNode> Game(string name, object parameters)
        {
            var result = await Call(client, name, parameters);
            var body = Value(result);
            Require(result.StructuredContent is not null && body["tick"] is not null, "Game result includes structured content and tick");
            return body;
        }
        async Task<JsonNode> Status() => (await Game("game.player.get_status", new { }))["value"]!;
        async Task Step(int count) => Require(Value(await Call(client, "world.step", new { count }))["steps_run"]!.GetValue<int>() == count, "Stepped requested ticks");
        async Task<JsonNode> Fire()
        {
            var accepted = await Game("game.player.fire", new { input = "tap" });
            Require(accepted["status"]!.GetValue<string>() == "pending", "A tap queues the regular fire input");
            await Step(1);
            var actionId = accepted["action_id"]!.GetValue<string>();
            var events = await Game("game.dev.read_events", new { filters = new { action_id = actionId } });
            var records = events["value"]!["events"]!.AsArray();
            if (fullCommands) Require(records.Any(record => record!["status"]!.GetValue<string>() == "pending"), "Fire acceptance is retained in the event log");
            var outcome = records.Single(record => record!["status"]!.GetValue<string>() == "completed")!;
            Require(outcome["action_id"]!.GetValue<string>() == actionId && outcome["tick"]!.GetValue<long>() > accepted["tick"]!.GetValue<long>() &&
                outcome["status"]!.GetValue<string>() == "completed", "Terminal fire event is correlated to its later tick");
            return outcome.DeepClone();
        }
        var catalog = await Game("game.dev.describe", new { command = "game.player.rotate" });
        Require(catalog["value"]!["commands"]!.AsArray().Single()!["name"]!.GetValue<string>() == "game.player.rotate", "Game discovery filters by command");
        var baseline = await Status();
        var shot = await Fire();
        Require(shot["value"]!["firing_events"]!.GetValue<int>() == 1 && shot["value"]!["projectiles_emitted"]!.GetValue<int>() > 0,
            "Normal firing emits projectiles");
        var cooldown = await Fire();
        Require(cooldown["value"]!["projectiles_emitted"]!.GetValue<int>() == 0 && !string.IsNullOrWhiteSpace(cooldown["value"]!["limiting_reason"]!.GetValue<string>()),
            "An immediate repeat follows the existing firing limit");

        var rpm = (await Game("game.player.set_requested_rpm", new { rpm = 10 }))["value"]!;
        Require(rpm["requested_rpm"]!.GetValue<double>() == 10, "RPM dial applies its requested setting");
        var power = (await Game("game.player.set_motor_power", new { fraction = 0.85 }))["value"]!;
        Require(power["motor_power"]!.GetValue<double>() == 0.85 && power["available_power"]!.GetValue<double>() < 0.85,
            "Power dial changes the request while delivery ramps");
        Require((await Call(client, "game.player.set_requested_rpm", new { rpm = -1 })).IsError == true &&
            (await Status())["requested_rpm"]!.GetValue<double>() == 10, "Invalid dial input preserves the applied setting");
        Require((await Call(client, "game.player.unregistered", new { })).IsError == true, "Unregistered game command is refused");
        await Game("game.player.rotate", new { direction = "right" });
        await Step(100);
        var right = await Status();
        Require(right["actual_rpm"]!.GetValue<double>() is > 0.05 and <= 10.2 &&
            right["available_power"]!.GetValue<double>() > power["available_power"]!.GetValue<double>(), "Manual right turn follows the requested RPM and power ramp");
        await Game("game.player.release_controls", new { });
        await Step(100);
        var stopped = await Status();
        Require(Math.Abs(stopped["actual_rpm"]!.GetValue<double>()) < 0.05, "Release brakes the assembly");
        await Game("game.player.rotate", new { direction = "left" });
        await Step(100);
        var left = await Status();
        Require(left["actual_rpm"]!.GetValue<double>() < -0.05, "Left turns in the opposite direction");
        await Game("game.player.rotate", new { direction = "stop" });

        Value(await Call(client, "world.stop", new { }));
        Require((await client.ListToolsAsync()).Any(tool => tool.Name == "game.player.rotate"), "Discovery refreshes after a World rebuild");
        var refused = await Game("game.player.rotate", new { direction = "left" });
        Require(refused["status"]!.GetValue<string>() == "refused", "Well-formed rotation is refused outside Play");
        return new JsonObject { ["registered"] = registered.Length, ["baseline"] = baseline.DeepClone(), ["shot"] = shot,
            ["cooldown"] = cooldown, ["rpmDial"] = rpm.DeepClone(), ["powerDial"] = power.DeepClone(), ["right"] = right.DeepClone(),
            ["stopped"] = stopped.DeepClone(), ["left"] = left.DeepClone(), ["outsidePlay"] = refused.DeepClone() };
    }

    private static async Task<JsonObject> CheckRemainingCommands(McpClient client, IList<McpClientTool> tools)
    {
        var exercised = new HashSet<string>(StringComparer.Ordinal);
        async Task<JsonNode> Game(string name, object args, string? status = null)
        {
            var response = Value(await Call(client, name, args));
            exercised.Add(name);
            Require(response["tick"] is not null, $"{name} returns a simulation tick");
            if (status is not null) Require(response["status"]?.GetValue<string>() == status, $"{name} returns {status}: {response}");
            return response;
        }
        async Task<JsonNode> Player(string action, object args, string status = "completed") =>
            await Game($"game.player.{action}", args, status);
        async Task<JsonNode> Dev(string action, object args, string status = "completed") =>
            await Game($"game.dev.{action}", args, status);
        async Task Step(int count) => Require(Value(await Call(client, "world.step", new { count }))["steps_run"]!.GetValue<int>() == count, "fixed steps execute");
        async Task<JsonNode> Inspect(string id) => (await Game("game.dev.inspect", new { id }))["value"]!;
        async Task<JsonNode> Status(string subsystem) => (await Game("game.player.get_status", new { subsystem }))["value"]!;
        async Task<JsonArray> Contacts(string filter = "all") => (await Game("game.player.list_contacts", new { filter }))["value"]!["contacts"]!.AsArray();
        async Task<JsonNode> Spawn(double x, double y, double vx, double vy) =>
            (await Dev("spawn_target", new { position = new { x, y }, movement = new { x = vx, y = vy }, size = 8, health = 3 }))["value"]!;

        var fixtures = (await Game("game.dev.list_fixtures", new { }))["value"]!["fixtures"]!.AsArray();
        Require(fixtures.Any(f => f!["id"]!.GetValue<string>() == "baseline"), "baseline is discoverable");
        var reset = await Dev("reset_fixture", new { fixture_id = "baseline", seed = 37 });
        Require(reset["tick"]!.GetValue<long>() == 0 && reset["value"]!["paused"]!.GetValue<bool>() &&
            reset["value"]!["seed"]!.GetValue<int>() == 37, "reset restores a paused, seeded baseline");
        var resetId = reset["action_id"]!.GetValue<string>();
        var resetEvents = await Game("game.dev.read_events", new { filters = new { action_id = resetId } });
        Require(resetEvents["value"]!["events"]!.AsArray().Count == 1, "reset action is retained in the new fixture history");
        await Dev("set_spawning", new { enabled = false });
        await Dev("configure_spawner", new { patch = new { max_targets = 7, band = new { index = 0, inner = 0.2, outer = 1, interval = 0.5 } } });
        Require((await Inspect("spawner"))["maxTargets"]!.GetValue<int>() == 7 &&
            !(await Inspect("spawner"))["enabled"]!.GetValue<bool>(), "spawner configuration is reflected");
        Require((await Call(client, "game.dev.configure_spawner", new { patch = new { max_targets = 8, band = new { index = 0, inner = 3, outer = 2 } } })).IsError == true &&
            (await Inspect("spawner"))["maxTargets"]!.GetValue<int>() == 7, "invalid spawn-band patch is atomic");

        await Player("set_auto_target", new { mode = "off" });
        var fallback = await Player("set_movement_mode", new { mode = "automatic" });
        Require(fallback["value"]!["movement"]!.GetValue<string>() == "manual", "automatic mode falls back with targeting off");
        await Player("set_movement_mode", new { mode = "semi_automatic" });
        var aim = await Player("aim", new { point = new { x = 90, y = -50 } });
        Require(aim["value"]!["alignment_error_radians"] is not null, "semi-auto aim reports its alignment error");
        await Step(1);
        await Player("set_movement_mode", new { mode = "manual" });

        var hidden = (await Spawn(0, -430, 0, 160))["id"]!.GetValue<string>();
        Require((await Inspect(hidden))["position"]!["y"]!.GetValue<double>() == -430, "developer reads hidden live position");
        Require((await Contacts()).All(contact => contact!["id"]!.GetValue<string>() != hidden), "unobserved target is hidden from the player");
        await Player("lock_target", new { target_id = hidden }, "refused");
        await Player("set_sweep_interval", new { seconds = 0.1 });
        await Step(6);
        var observed = (await Contacts("radar")).Single(c => c!["id"]!.GetValue<string>() == hidden)!;
        Require(observed["source"]!.GetValue<string>() == "radar", "radar publishes an observation");
        await Player("set_sweep_interval", new { seconds = 8 });
        await Step(1);
        var last = (await Contacts("radar")).Single(c => c!["id"]!.GetValue<string>() == hidden)!;
        Require(last["position"]!["y"]!.GetValue<double>() != (await Inspect(hidden))["position"]!["y"]!.GetValue<double>(),
            "radar-only reads retain an observed position instead of the live transform");
        await Player("select_target", new { contact_id = hidden });
        var visible = (await Spawn(60, -40, 0, 0))["id"]!.GetValue<string>();
        Require((await Contacts("local")).Any(c => c!["id"]!.GetValue<string>() == visible), "direct observation exposes the local target");
        Require((await Player("lock_target", new { target_id = visible }))["value"]!["locked_id"]!.GetValue<string>() == visible, "normal lock chooses visible target");
        await Dev("remove_target", new { target_id = visible });
        Require((await Status("firing"))["locked_id"]!.GetValue<string>() == "0", "target removal clears lock immediately");
        Require((await Dev("clear_targets", new { }))["value"]!["remaining"]!.GetValue<int>() == 0 && (await Contacts()).Count == 0,
            "clearing targets clears all observations");

        var radarBefore = await Status("radar");
        var range = (await Player("set_radar_range", new { range = "R5" }))["value"]!;
        Require(range["detection_range"]!.GetValue<double>() == radarBefore["detection_range"]!.GetValue<double>() &&
            range["automatic_range"]!.GetValue<double>() == radarBefore["automatic_range"]!.GetValue<double>(),
            "display zoom leaves detection and acquisition unchanged");
        Require((await Player("set_radar_expanded", new { expanded = true }))["value"]!["expanded"]!.GetValue<bool>(), "radar view expands");
        await Player("set_radar_expanded", new { expanded = false });
        await Player("set_auto_target", new { mode = "radar" });
        await Player("set_auto_fire", new { mode = "local_and_radar" });
        await Player("set_auto_fire", new { mode = "off" });
        await Player("set_salvo_mode", new { mode = "alt" });
        await Player("set_cycle_mode", new { mode = "overload" });
        Require((await Player("set_cadence", new { events_per_minute = 0 }))["value"]!["requested_events_per_minute"]!.GetValue<double>() == 0,
            "zero cadence pauses firing events");
        await Player("set_cadence", new { events_per_minute = 150 });
        await Player("set_cycle_mode", new { mode = "balanced" });

        var drive = await Inspect("drive");
        var gear = drive["gears"]!.AsArray().First()!["id"]!.GetValue<string>();
        await Player("set_regulation", new { mode = "off" });
        await Player("set_shift_policy", new { policy = "economy" });
        await Dev("set_gear_health", new { gear_id = gear, fraction = 0.15 });
        await Player("set_gear_enabled", new { gear_id = gear, enabled = false });
        var enabled = (await Player("set_gear_enabled", new { gear_id = gear, enabled = true }))["value"]!["gears"]!.AsArray().First()!;
        Require(enabled["enabled"]!.GetValue<bool>() && enabled["health"]!.GetValue<double>() == 0.15, "gear permission preserves damage");
        await Player("disconnect_transmission", new { });
        var repair = await Player("start_repair", new { });
        Require(repair["value"]!["repairing"]!.GetValue<bool>() && repair["value"]!["repair_progress"]!.GetValue<double>() == 0,
            "repair starts without repairing instantly");
        await Player("reconnect_transmission", new { }, "refused");
        await Step(1);
        Require((await Inspect("drive"))["repair_progress"]!.GetValue<double>() > 0, "repair advances in simulation");

        var energy = await Dev("set_energy", new { stored_energy = 250000 });
        Require(energy["value"]!["stored"]!.GetValue<double>() == 250000 && energy["value"]!["capacity"]!.GetValue<double>() >= 250000, "energy injection preserves capacity");
        await Dev("set_temperature", new { reservoir_id = "absorber", temperature = 200 });
        Require((await Inspect("thermal"))["absorber"]!.GetValue<double>() == 200, "thermal injection updates the selected reservoir");
        await Player("set_cooling_route", new { route_id = "Shared absorber" });
        Require((await Call(client, "game.player.set_cooling_route", new { route_id = "missing" })).IsError == true &&
            (await Status("thermal"))["route"]!.GetValue<string>() == "Shared absorber", "invalid cooling route preserves the prior route");
        await Step(1);
        var thermal = await Status("thermal");
        Require(!string.IsNullOrEmpty(thermal["branch_flow"]!.GetValue<string>()), "cooling publishes branch flow after a step");
        await Player("set_cooling_route", new { route_id = "Auto" });
        await Dev("inject_fault", new { type = "barrel_jam", component_id = "cannon:1" });
        Require((await Inspect("cannon:1"))["jammed"]!.GetValue<bool>(), "test barrel jam is visible");
        await Dev("clear_fault", new { type = "barrel_jam", component_id = "cannon:1" });
        Require(!(await Inspect("cannon:1"))["jammed"]!.GetValue<bool>(), "test barrel jam clears");
        await Dev("inject_fault", new { type = "transmission_jam", component_id = "drive" });
        Require((await Inspect("drive"))["transmission_jammed"]!.GetValue<bool>(), "transmission jam is visible");
        await Dev("clear_fault", new { type = "transmission_jam", component_id = "drive" });
        Require(!(await Inspect("drive"))["transmission_jammed"]!.GetValue<bool>(), "transmission jam clears");
        var events = await Game("game.dev.read_events", new { filters = new { method = "game.player.reconnect_transmission" } });
        Require(events["value"]!["events"]!.AsArray().Any(item => item!["status"]!.GetValue<string>() == "refused"),
            "gameplay refusal can be read from the event log");
        var oldCursor = events["value"]!["cursor"]!.GetValue<string>();

        Value(await Call(client, "world.stop", new { }));
        var original = (await Inspect("turret"))["configuration"]!["projectileSpeed"]!.GetValue<double>();
        Require((await Call(client, "game.dev.configure_fixture", new { id = "turret", patch = new { projectileSpeed = 600, actualRpm = 99 } })).IsError == true &&
            (await Inspect("turret"))["configuration"]!["projectileSpeed"]!.GetValue<double>() == original, "invalid authored patch is atomic");
        await Dev("configure_fixture", new { id = "turret", patch = new { projectileSpeed = 600 } });
        Require((await Inspect("turret"))["configuration"]!["projectileSpeed"]!.GetValue<double>() == 600, "valid authored patch applies in memory");
        await Dev("reset_fixture", new { fixture_id = "baseline", seed = 37 });
        Require((await Inspect("turret"))["configuration"]!["projectileSpeed"]!.GetValue<double>() == original, "fixture reset restores authored baseline");
        Require((await Call(client, "game.dev.read_events", new { cursor = oldCursor })).IsError == true, "fixture reset rejects an old event cursor");
        var freshTarget = (await Spawn(0, -430, 0, 160))["id"]!.GetValue<string>();
        Require(freshTarget != hidden && (await Call(client, "game.dev.inspect", new { id = hidden })).IsError == true,
            "a new fixture never aliases an earlier target ID");
        Value(await Call(client, "world.resume", new { }));
        var scale = Value(await Call(client, "world.time_scale", new { value = 0.5 }));
        Require(scale["stats"]!["time_scale"]!.GetValue<double>() == 0.5, "the existing time-scale command applies");
        Value(await Call(client, "world.pause", new { }));
        Value(await Call(client, "world.stop", new { }));
        var play = Value(await Call(client, "world.play", new { }));
        Require(play["stats"]!["mode"]!.GetValue<string>() == "play", "the existing Play command rebuilds the active world");
        Value(await Call(client, "world.pause", new { }));
        var commands = tools.Where(t => t.Name.StartsWith("game.", StringComparison.Ordinal)).Select(t => t.Name).ToArray();
        string[] earlier = ["game.dev.describe", "game.dev.read_events", "game.player.fire", "game.player.get_status",
            "game.player.release_controls", "game.player.rotate", "game.player.set_motor_power", "game.player.set_requested_rpm"];
        Require(commands.All(name => exercised.Contains(name) || earlier.Contains(name)), "every advertised game binding was exercised");
        return new JsonObject { ["methods"] = new JsonArray(commands.Order().Select(name => (JsonNode?)JsonValue.Create(name)).ToArray()),
            ["reset"] = reset.DeepClone(), ["radarObservation"] = observed.DeepClone(), ["thermal"] = thermal.DeepClone(), ["repair"] = repair.DeepClone() };
    }

    private static async Task<CallToolResult> Call(McpClient client, string method, object parameters)
    {
        using var deadline = new CancellationTokenSource(TimeSpan.FromSeconds(30));
        return await client.CallToolAsync(method, new Dictionary<string, object?> { ["parameters"] = parameters }, cancellationToken: deadline.Token);
    }

    private static JsonNode Value(CallToolResult result)
    {
        var text = result.Content.OfType<TextContentBlock>().Single().Text;
        if (result.IsError == true) throw new IOException(text);
        return JsonNode.Parse(text)?["result"]?["value"] ?? throw new IOException("Missing MCP response value.");
    }

    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}
