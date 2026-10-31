using System.Diagnostics;
using System.IO.Pipes;
using System.Text;
using System.Text.Json.Nodes;
using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;

if (args.Length > 0 && args[0] == "--editor") { await EditorQualification.Run(args[1..]); return; }
if (args.Length > 0 && args[0] == "--editor-code") { await EditorCodeQualification.Run(args[1..]); return; }
if (args.Length > 0 && args[0] == "--editor-physics") { await EditorPhysicsQualification.Run(args[1..]); return; }

if (args.Length != 5) throw new ArgumentException("usage: tests <player> <project> <mcp.dll> <capture-root> <receipt.json>");
var player = Path.GetFullPath(args[0]);
var project = Path.GetFullPath(args[1]);
var mcp = Path.GetFullPath(args[2]);
var captures = Path.Combine(Path.GetFullPath(args[3]), "capturas com espaço-ação");
Directory.CreateDirectory(captures);
var receipt = new JsonArray();
void Check(bool condition, string name)
{
    receipt.Add(new JsonObject { ["case"] = name, ["passed"] = condition });
    if (!condition) throw new InvalidOperationException(name);
}

async Task<CallToolResult> Call(McpClient client, string method, object parameters)
    => await client.CallToolAsync(method, new Dictionary<string, object?> { ["parameters"] = parameters });
try
{
    var channel = $"qualification-{Guid.NewGuid():N}";
    var rawRoot = Path.Combine(captures, "raw");
    var start = new ProcessStartInfo(player) { UseShellExecute = false, CreateNoWindow = true, WorkingDirectory = project,
        RedirectStandardOutput = true, RedirectStandardError = true };
    foreach (var argument in new[] { "--project", project, "--dev-channel", channel, "--dev-capture-dir", rawRoot,
        "--dev-hidden", "on", "--size", "640x360", "--fps", "60", "--vsync", "off", "--quit-after", "25" }) start.ArgumentList.Add(argument);
    using var host = Process.Start(start) ?? throw new IOException("native host did not launch");
    var stdout = host.StandardOutput.ReadToEndAsync();
    var stderr = host.StandardError.ReadToEndAsync();
    try
    {
        using var pipe = new NamedPipeClientStream(".", $"pixie-{channel}", PipeDirection.InOut, PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
        await pipe.ConnectAsync(10000);
        using var writer = new StreamWriter(pipe, new UTF8Encoding(false), leaveOpen: true) { AutoFlush = true };
        using var reader = new StreamReader(pipe, new UTF8Encoding(false, true), leaveOpen: true);
        int id = 0;
        var frames = new List<long>();
        async Task<JsonNode> Raw(string method, JsonObject? parameters = null)
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(10));
            await writer.WriteLineAsync(new JsonObject { ["jsonrpc"] = "2.0", ["id"] = ++id, ["method"] = method, ["params"] = parameters ?? new() }.ToJsonString().AsMemory(), timeout.Token);
            var response = JsonNode.Parse(await reader.ReadLineAsync(timeout.Token) ?? throw new IOException("missing response"))!;
            var frame = response["result"]?["frame"] ?? response["error"]?["data"]?["frame"];
            if (frame is not null) frames.Add(frame.GetValue<long>());
            return response;
        }
        Check((await Raw("world.stats"))["error"] is not null, "hello-first-over-real-pipe");
        Check((await Raw("world.hello"))["result"]?["value"]?["capabilities"]?["capture"]?.GetValue<bool>() == true, "native-capture-capability");
        using (var second = new NamedPipeClientStream(".", $"pixie-{channel}", PipeDirection.InOut, PipeOptions.Asynchronous))
        {
            bool refused = false;
            try { await second.ConnectAsync(100); } catch (TimeoutException) { refused = true; }
            Check(refused, "second-client-refused");
        }
        Check((await Raw("world.pause"))["result"] is not null, "paused-queue-drains");
        var step = await Raw("world.step", new() { ["count"] = 3 });
        Check(step["result"]?["value"]?["steps_run"]?.GetValue<int>() == 3, "paused-step-completes-before-response");
        Check((await Raw("world.capture", new() { ["name"] = "../escape" }))["error"] is not null, "capture-traversal-rejected");
        Check((await Raw("world.capture", new() { ["name"] = "CON" }))["error"] is not null, "capture-device-basename-rejected");
        var capture = await Raw("world.capture", new() { ["name"] = "frame" });
        Check(capture["result"]?["value"]?["alpha"]?.GetValue<double>() == 1, "native-capture-alpha-one");
        var png = await File.ReadAllBytesAsync(Path.Combine(rawRoot, "frame.png"));
        Check(png.Length > 8 && png[0] == 137 && png[1] == 80, "native-capture-png-created");
        Check((await Raw("world.capture", new() { ["name"] = "frame" }))["error"] is not null, "capture-existing-file-preserved");
        var preserved = await File.ReadAllBytesAsync(Path.Combine(rawRoot, "frame.png"));
        Check(png.SequenceEqual(preserved), "capture-existing-bytes-unchanged");
        Check((await Raw("world.save"))["error"] is not null, "channel-save-disabled");
        Check((await Raw("inspector.tree"))["error"] is not null, "player-has-no-inspector-namespace");
        Check(frames.Count == id && frames.Zip(frames.Skip(1)).All(pair => pair.Second > pair.First), "commands-drained-on-distinct-owner-frames");
    }
    finally
    {
        if (!host.HasExited) host.Kill(true);
        await host.WaitForExitAsync();
        await File.WriteAllTextAsync(Path.Combine(captures, "raw.stdout.log"), await stdout);
        await File.WriteAllTextAsync(Path.Combine(captures, "raw.stderr.log"), await stderr);
    }

    var transport = new StdioClientTransport(new StdioClientTransportOptions
    {
        Name = "Pixie qualification", Command = "dotnet", StandardErrorLines = Console.Error.WriteLine,
        Arguments = [mcp, "--host", player, "--project", project, "--captures", Path.Combine(captures, "mcp")]
    });
    await using var client = await McpClient.CreateAsync(transport);
    Check(client.NegotiatedProtocolVersion is not null, "official-mcp-client-negotiated-protocol");
    receipt[^1]!["protocol_version"] = client.NegotiatedProtocolVersion;
    var tools = await client.ListToolsAsync();
    Check(tools.Any(t => t.Name == "world.capture") && !tools.Any(t => t.Name == "world.save"), "official-mcp-client-discovery-without-save");
    Check((await Call(client, "world.pause", new { })).IsError != true, "official-mcp-client-pause");
    Check((await Call(client, "world.step", new { count = 3 })).IsError != true, "official-mcp-client-step");
    var image = await Call(client, "world.capture", new { name = "image-for-model" });
    Check(image.IsError != true && image.Content.OfType<ImageContentBlock>().Count() == 1, "official-mcp-client-image-content");
    var imageBytes = image.Content.OfType<ImageContentBlock>().Single().DecodedData.ToArray();
    Check(imageBytes.Length > 8 && imageBytes[0] == 137 && imageBytes[1] == 80, "official-mcp-client-valid-png-payload");
    Check((await Call(client, "world.capture", new { name = "../escape" })).IsError == true, "mcp-path-rejection-is-tool-error");
    Check((await Call(client, "world.capture", new { name = "image-for-model" })).IsError == true, "mcp-overwrite-rejection-is-tool-error");
    await using var legacy = await McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions
    {
        Name = "Pixie legacy protocol qualification", Command = "dotnet", StandardErrorLines = Console.Error.WriteLine,
        Arguments = [mcp, "--host", player, "--project", project, "--captures", Path.Combine(captures, "legacy")]
    }), new McpClientOptions { ProtocolVersion = "2025-11-25" });
    Check(legacy.NegotiatedProtocolVersion == "2025-11-25", "official-legacy-mcp-client-handshake");
    Check((await legacy.ListToolsAsync()).Any(t => t.Name == "world.step"), "official-legacy-mcp-client-discovery");
    Check((await Call(legacy, "world.pause", new { })).IsError != true, "official-legacy-mcp-client-pause");
    var legacyImage = await Call(legacy, "world.capture", new { name = "legacy-image" });
    Check(legacyImage.IsError != true && legacyImage.Content.OfType<ImageContentBlock>().Count() == 1, "official-legacy-mcp-client-image");
}
finally
{
    Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[4]))!);
    await File.WriteAllTextAsync(args[4], receipt.ToJsonString(new() { WriteIndented = true }));
}
Console.WriteLine($"PASS: {receipt.Count} real-pipe and official MCP-client cases.");
