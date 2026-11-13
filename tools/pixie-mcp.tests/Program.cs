using System.Diagnostics;
using System.IO.Pipes;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;

if (args.Contains("--dev-channel")) { await BridgeContractQualification.Host(args); return; }
if (args.Length > 0 && args[0] == "--bridge-contract") { await BridgeContractQualification.Run(args[1..]); return; }
if (args.Length > 0 && args[0] == "--test-build") { await TestBuildQualification.Run(args[1..]); return; }
if (args.Length > 0 && args[0] == "--editor") { await EditorQualification.Run(args[1..]); return; }
if (args.Length > 0 && args[0] == "--editor-code") { await EditorCodeQualification.Run(args[1..]); return; }
if (args.Length > 0 && args[0] == "--editor-physics") { await EditorPhysicsQualification.Run(args[1..]); return; }
if (args.Length > 0 && args[0] == "--editor-lighting") { await EditorLightingQualification.Run(args[1..]); return; }

if (args.Length < 5 || (args.Length - 5) % 2 != 0) throw new ArgumentException("usage: tests <player> <project> <mcp.dll> <capture-root> <receipt.json> [explicit bridge SDK options]");
var sdkArguments = args[5..];
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
        var nativeHello = (await Raw("world.hello"))["result"]?["value"];
        Check(nativeHello?["capabilities"]?["capture"]?.GetValue<bool>() == true, "native-capture-capability");
        Check(nativeHello?["engine"]?["schemaVersion"]?.GetValue<int>() == NativeApiVersion.IdentitySchema &&
            nativeHello?["engine"]?["capabilities"]?["nativeAutomation"]?.GetValue<bool>() == true, "native-C1-private-identity-present");
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
        Arguments = [mcp, "--host", player, "--project", project, "--captures", Path.Combine(captures, "mcp"), .. sdkArguments]
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
        Arguments = [mcp, "--host", player, "--project", project, "--captures", Path.Combine(captures, "legacy"), .. sdkArguments]
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

// An explicitly synthetic package/pipe boundary fixture. It never claims an Engine
// binary, Editor behavior, installed SDK independence or authored-state acceptance.
internal static class BridgeContractQualification
{
    private static readonly JsonArray Records = [];
    private static void Check(bool passed, string name, JsonNode? detail = null)
    {
        Records.Add(new JsonObject { ["case"] = name, ["passed"] = passed, ["detail"] = detail?.DeepClone() });
        if (!passed) throw new InvalidOperationException(name);
    }
    private static string Hash(string path) { using var file = File.OpenRead(path); return Convert.ToHexStringLower(SHA256.HashData(file)); }
    private static JsonObject Native() => new()
    {
        ["schemaVersion"] = NativeApiVersion.IdentitySchema, ["sdkId"] = new string('a', 64), ["sdkBuildId"] = new string('b', 64),
        ["cApiVersion"] = NativeApiVersion.Current, ["moduleIdentityContract"] = 1, ["platform"] = "windows", ["architecture"] = "x64",
        ["compilerId"] = "MSVC", ["compilerVersion"] = "19.51.36260.0", ["msvcFullVersion"] = 195136260, ["stlVersion"] = 145,
        ["stlUpdate"] = 202604, ["iteratorDebugLevel"] = 0, ["cppStandard"] = 20, ["configuration"] = "Release", ["crt"] = "MD",
        ["cpuFeatures"] = new JsonArray("avx2", "fma", "f16c"), ["profile"] = "local-development", ["distributionClass"] = "local-private",
        ["exceptions"] = true, ["rtti"] = true, ["windowsSdkVersion"] = "10.0.26100.0", ["dependencyInventorySha256"] = new string('c', 64),
        ["capabilityBits"] = 7, ["capabilities"] = new JsonObject { ["moduleLoading"] = true, ["nativeAutomation"] = true, ["candidateAuthoring"] = true }
    };
    private static string Package(string root, JsonObject native)
    {
        var bin = Path.Combine(root, "profiles/local-development/Release/bin"); Directory.CreateDirectory(bin);
        foreach (var file in Directory.EnumerateFiles(AppContext.BaseDirectory)) File.Copy(file, Path.Combine(bin, Path.GetFileName(file)));
        File.WriteAllText(Path.Combine(bin, "pixie.dll"), "Synthetic runtime placeholder; never loaded.");
        var player = "profiles/local-development/Release/bin/PixieMcp.Tests.exe";
        if (!File.Exists(Path.Combine(root, player))) throw new IOException("Build the qualification client's explicit Windows apphost first.");
        var profile = native.DeepClone().AsObject();
        profile["runtime"] = new JsonObject { ["path"] = "profiles/local-development/Release/bin/pixie.dll", ["sha256"] = Hash(Path.Combine(bin, "pixie.dll")) };
        profile["player"] = new JsonObject { ["path"] = player, ["sha256"] = Hash(Path.Combine(root, player)) };
        const string profilePath = "profiles/local-development/Release/profile.json";
        File.WriteAllText(Path.Combine(root, profilePath), profile.ToJsonString());
        var files = new JsonArray();
        foreach (var file in Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories))
            files.Add(new JsonObject { ["path"] = Path.GetRelativePath(root, file).Replace('\\', '/'), ["size"] = new FileInfo(file).Length, ["sha256"] = Hash(file), ["role"] = "synthetic-fixture" });
        var manifest = new JsonObject { ["schemaVersion"] = 1, ["sdkId"] = native["sdkId"]!.DeepClone(), ["productVersion"] = "0.3.0.0", ["distributionClass"] = "local-private",
            ["profiles"] = new JsonArray(new JsonObject { ["profile"] = "local-development", ["configuration"] = "Release", ["sdkBuildId"] = native["sdkBuildId"]!.DeepClone(), ["manifest"] = profilePath, ["manifestSha256"] = Hash(Path.Combine(root, profilePath)) }), ["files"] = files };
        File.WriteAllText(Path.Combine(root, "sdk.json"), manifest.ToJsonString());
        return Path.Combine(root, player);
    }
    public static async Task Host(string[] args)
    {
        var options = new Dictionary<string, string>(StringComparer.Ordinal);
        for (var index = 0; index + 1 < args.Length; index += 2) options.Add(args[index], args[index + 1]);
        var folder = options["--project"];
        var control = JsonNode.Parse(await File.ReadAllTextAsync(Path.Combine(folder, "synthetic-host.json")))!.AsObject();
        await File.WriteAllTextAsync(Path.Combine(folder, "host-arguments.json"), JsonSerializer.Serialize(new { pid = Environment.ProcessId, args }));
        using var pipe = new NamedPipeServerStream("pixie-" + options["--dev-channel"], PipeDirection.InOut, 1,
            PipeTransmissionMode.Byte, PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
        await pipe.WaitForConnectionAsync();
        using var reader = new StreamReader(pipe, new UTF8Encoding(false, true), leaveOpen: true);
        var helloCount = 0;
        while (await reader.ReadLineAsync() is { } text)
        {
            var request = JsonNode.Parse(text)!; var method = request["method"]!.GetValue<string>();
            await File.AppendAllTextAsync(Path.Combine(folder, "host-methods.jsonl"), JsonSerializer.Serialize(new { method }) + "\n");
            if (method == "world.hello" && control["delayHello"]?.GetValue<int>() is { } delay) await Task.Delay(delay);
            var editor = control["editor"]?.GetValue<bool>() == true;
            JsonNode value = new JsonObject { ["ok"] = true };
            if (method == "world.hello")
            {
                ++helloCount;
                value = new JsonObject { ["protocol_version"] = 1, ["pixie_api_version"] = NativeApiVersion.Current,
                    ["capture_directory"] = options["--dev-capture-dir"], ["engine"] = control["engine"]?.DeepClone(),
                    ["capabilities"] = new JsonObject { ["world"] = true, ["transport"] = true, ["capture"] = true,
                        ["inspector"] = editor, ["editor"] = editor, ["save"] = false } };
                if (control["operations"] is JsonObject operations) value["capabilities"] = operations.DeepClone();
                if (control["protocol"] is { } protocol) value["protocol_version"] = protocol.DeepClone();
                if (control["api"] is { } api) value["pixie_api_version"] = api.DeepClone();
                if (control["missingEngine"]?.GetValue<bool>() == true) value.AsObject().Remove("engine");
                if (control["wrongCaptureRoot"]?.GetValue<bool>() == true) value["capture_directory"] = folder;
                if (helloCount > 1 && control["wrongRepeatIdentity"]?.GetValue<bool>() == true) value["engine"]!["sdkBuildId"] = new string('f', 64);
            }
            var response = new JsonObject { ["jsonrpc"] = "2.0", ["id"] = request["id"]!.DeepClone(), ["result"] = new JsonObject { ["value"] = value } };
            if (method == "world.hello" && control["ambiguousOutcome"]?.GetValue<bool>() == true)
                response["error"] = new JsonObject { ["code"] = -32000, ["message"] = "Synthetic dual outcome" };
            if (method == "world.create" && control["wrongReplyId"]?.GetValue<bool>() == true) response["id"] = -1;
            var output = response.ToJsonString();
            if (method == "world.hello" && control["duplicateIdentity"]?.GetValue<bool>() == true) output = output.Replace("\"schemaVersion\":1", "\"schemaVersion\":1,\"schemaVersion\":1", StringComparison.Ordinal);
            if (method == "world.hello" && control["floatingSchema"]?.GetValue<bool>() == true) output = output.Replace("\"schemaVersion\":1", "\"schemaVersion\":1.0", StringComparison.Ordinal);
            if (method == "world.create" && control["oversizeReply"]?.GetValue<bool>() == true) output = new string(' ', 4 * 1024 * 1024);
            var bytes = Encoding.UTF8.GetBytes(output + "\n");
            if (method == "world.create" && control["invalidUtf8Reply"]?.GetValue<bool>() == true)
            {
                // Keep JSON syntax valid: corrupt a string value, not a delimiter/key.
                bytes = Encoding.UTF8.GetBytes($"{{\"jsonrpc\":\"2.0\",\"id\":{request["id"]},\"result\":{{\"value\":\"invalid-utf8-value\"}}}}\n");
                bytes[bytes.AsSpan().IndexOf("invalid-utf8-value"u8)] = 0xff;
            }
            await pipe.WriteAsync(bytes); await pipe.FlushAsync();
        }
    }

    public static async Task Run(string[] args)
    {
        if (args.Length != 4 || !Path.IsPathFullyQualified(args[0]) || !Path.IsPathFullyQualified(args[1]))
            throw new ArgumentException("--bridge-contract <explicit bridge apphost> <explicit absolute dotnet.exe> <work> <receipt.json>");
        var mcp = Path.GetFullPath(args[0]); var dotnet = args[1]; var work = Path.GetFullPath(args[2]); Directory.CreateDirectory(work);
        var native = Native(); var sdk = Path.Combine(work, "explicit private SDK ação"); var player = Package(sdk, native);
        var ordinal = 0;
        async Task Test(string name, Action<JsonObject>? change = null, bool editor = false, bool accepted = false,
            string[]? extra = null, string? root = null, string? environmentRoot = null, string? method = null, JsonObject? parameters = null,
            bool uncertain = false, bool useDotnet = false, string? startupRefusal = null)
        {
            var folder = Path.Combine(work, (++ordinal).ToString("D3") + "-" + name); Directory.CreateDirectory(folder);
            var control = new JsonObject { ["engine"] = native.DeepClone(), ["editor"] = editor }; change?.Invoke(control);
            await File.WriteAllTextAsync(Path.Combine(folder, "synthetic-host.json"), control.ToJsonString());
            var host = useDotnet ? dotnet :
                root is not (null or "missing-context") && Directory.Exists(root)
                    ? Path.Combine(root, "profiles/local-development/Release/bin/PixieMcp.Tests.exe") : player;
            var launch = new List<string> { "--host", host, "--project", folder, "--captures", Path.Combine(folder, "captures"), "--startup-timeout", "3" };
            if (root != "missing-context") { launch.Add("--sdk"); launch.Add(root ?? sdk); }
            if (editor) { launch.Add("--kind"); launch.Add("editor"); }
            if (extra != null) launch.AddRange(extra);
            var errors = new List<string>();
            await using (var client = await McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions
            {
                Name = "synthetic-" + name, Command = mcp, Arguments = launch,
                EnvironmentVariables = new Dictionary<string, string?> { ["PIXIE_SDK_ROOT"] = environmentRoot },
                StandardErrorLines = line => { lock (errors) errors.Add(line); }
            })))
            {
                Check((await client.ListToolsAsync()).Any(tool => tool.Name == "world.hello"), name + ":immediate-tools");
                var result = await client.CallToolAsync(method ?? "world.create", new Dictionary<string, object?> { ["parameters"] = parameters ?? new JsonObject() });
                var text = result.Content.OfType<TextContentBlock>().First().Text;
                var receipt = JsonNode.Parse(text)!;
                Check((result.IsError != true) == accepted, name + ":expected-admission", receipt);
                if (!accepted) Check(receipt["possibly_applied"]?.GetValue<bool>() == uncertain && receipt["retry_automatically"]?.GetValue<bool>() == false,
                    name + ":nonretrying-uncertainty", receipt);
                if (startupRefusal is not null)
                    Check(receipt["error"]?.GetValue<string>().Contains(startupRefusal, StringComparison.Ordinal) == true, name + ":startup-diagnostic", receipt);
                if (uncertain)
                {
                    var retry = await client.CallToolAsync("world.create", new Dictionary<string, object?> { ["parameters"] = new JsonObject() });
                    Check(retry.IsError == true, name + ":poisoned-connection-refuses-next-call");
                }
            }
            var methodsPath = Path.Combine(folder, "host-methods.jsonl");
            var methods = File.Exists(methodsPath) ? (await File.ReadAllLinesAsync(methodsPath)).Select(line => JsonNode.Parse(line)!["method"]!.GetValue<string>()).ToArray() : [];
            var expectedCreates = method == null || method == "world.create" ? (accepted || uncertain ? 1 : 0) : 0;
            Check(methods.Count(item => item == "world.create") == expectedCreates, name + ":mutation-count");
            if (!accepted && !uncertain && method == "editor.do") Check(!methods.Contains("editor.do"), name + ":unsupported-operation-not-sent");
            var argumentsPath = Path.Combine(folder, "host-arguments.json");
            await File.WriteAllTextAsync(Path.Combine(folder, "bridge.stderr.log"), string.Join('\n', errors));
            if (startupRefusal is not null)
                Check(!File.Exists(argumentsPath) && !Directory.Exists(Path.Combine(folder, "captures")), name + ":no-host-start");
            if (accepted)
            {
                var arguments = JsonNode.Parse(await File.ReadAllTextAsync(argumentsPath))!["args"]!.AsArray().Select(item => item!.GetValue<string>()).ToArray();
                if (!editor) Check(arguments.Where((_, index) => index % 2 == 0).SequenceEqual(["--project", "--dev-channel", "--dev-capture-dir", "--dev-hidden"]), name + ":player-only-supported-flags");
                else
                {
                    Check(arguments.Contains("--sdk") && arguments.Contains("--sdk-id") && arguments.Contains("--sdk-build-id") && arguments.Contains("--sdk-manifest-sha256") &&
                        arguments.Contains("--sdk-profile") && arguments.Contains("--native-configuration") && arguments.Contains("--recent"), name + ":single-editor-selection-forwarded");
                    Check(arguments[Array.IndexOf(arguments, "--sdk") + 1] == Path.GetFullPath(root ?? sdk) && arguments[Array.IndexOf(arguments, "--sdk-build-id") + 1] == new string('b', 64), name + ":exact-editor-selection-values");
                }
            }
            if (File.Exists(argumentsPath))
            {
                var pid = JsonNode.Parse(await File.ReadAllTextAsync(argumentsPath))!["pid"]!.GetValue<int>();
                bool exited = false;
                var deadline = Stopwatch.StartNew();
                while (!exited && deadline.Elapsed < TimeSpan.FromSeconds(2))
                {
                    try { using var process = Process.GetProcessById(pid); exited = process.HasExited; } catch (ArgumentException) { exited = true; }
                    if (!exited) await Task.Delay(20);
                }
                Check(exited, name + ":host-cleaned-up");
            }
        }
        try
        {
            await Test("matching-player", accepted: true);
            await Test("matching-editor-apphost", editor: true, accepted: true);
            var editorAssembly = Path.Combine(Path.GetDirectoryName(player)!, "PixieMcp.Tests.dll");
            await Test("matching-editor-explicit-dotnet", editor: true, accepted: true, useDotnet: true, extra: ["--editor-assembly", editorAssembly]);
            await Test("editor-apphost-rejects-assembly", editor: true, extra: ["--editor-assembly", editorAssembly],
                startupRefusal: "--editor-assembly requires an explicit absolute dotnet.exe host.");
            await Test("process-local-explicit-root", accepted: true, root: "missing-context", environmentRoot: sdk);
            await Test("missing-selection", root: "missing-context");
            await Test("invalid-launch-does-not-fall-back-to-environment", root: Path.Combine(work, "absent"), environmentRoot: sdk);
            await Test("player-rejects-editor-assembly", extra: ["--editor-assembly", editorAssembly]);
            await Test("editor-rejects-relative-assembly", editor: true, useDotnet: true, extra: ["--editor-assembly", "PixieMcp.Tests.dll"]);
            await Test("wrong-expected-sdk", extra: ["--sdk-id", new string('d', 64)]);
            await Test("wrong-expected-build", extra: ["--sdk-build-id", new string('d', 64)]);
            await Test("wrong-expected-manifest", extra: ["--sdk-manifest-sha256", new string('d', 64)]);
            await Test("public-profile-not-automation", extra: ["--sdk-profile", "authoring"]);
            await Test("unavailable-debug-no-remap", extra: ["--native-configuration", "Debug"]);
            await Test("missing-engine", control => control["missingEngine"] = true);
            await Test("wrong-protocol", control => control["protocol"] = 2);
            await Test("wrong-api", control => control["api"] = NativeApiVersion.Current + 1);
            await Test("wrong-capture-root", control => control["wrongCaptureRoot"] = true);
            await Test("dual-hello-outcome-refused", control => control["ambiguousOutcome"] = true);
            await Test("duplicate-identity", control => control["duplicateIdentity"] = true);
            await Test("noninteger-identity", control => control["floatingSchema"] = true);
            await Test("hello-deadline", control => control["delayHello"] = 5000);
            foreach (var field in new[] { "schemaVersion", "sdkId", "sdkBuildId", "cApiVersion", "moduleIdentityContract", "platform", "architecture", "compilerId", "compilerVersion",
                "msvcFullVersion", "stlVersion", "stlUpdate", "iteratorDebugLevel", "cppStandard", "configuration", "crt", "cpuFeatures", "profile", "distributionClass", "exceptions", "rtti", "windowsSdkVersion", "dependencyInventorySha256" })
            {
                await Test("missing-" + field, control => control["engine"]!.AsObject().Remove(field));
                await Test("wrong-" + field, control =>
                {
                    var engine = control["engine"]!.AsObject(); var value = engine[field]!;
                    if (field is "sdkId" or "sdkBuildId" or "dependencyInventorySha256") engine[field] = new string('f', 64);
                    else if (value is JsonArray) engine[field] = new JsonArray("avx2");
                    else if (value.GetValueKind() == JsonValueKind.String) engine[field] = "mismatched";
                    else if (value.GetValueKind() is JsonValueKind.True or JsonValueKind.False) engine[field] = !value.GetValue<bool>();
                    else engine[field] = value.GetValue<int>() + 1;
                });
            }
            await Test("wrong-raw-bits", control => control["engine"]!["capabilityBits"] = 5);
            await Test("missing-raw-bits", control => control["engine"]!.AsObject().Remove("capabilityBits"));
            await Test("native-automation-absent", control => control["engine"]!["capabilities"]!["nativeAutomation"] = false);
            await Test("missing-capture-operation", control => control["operations"] = new JsonObject { ["world"] = true, ["transport"] = true, ["capture"] = false, ["inspector"] = false, ["save"] = false });
            await Test("player-cannot-advertise-save", control => control["operations"] = new JsonObject { ["world"] = true, ["transport"] = true, ["capture"] = true, ["inspector"] = false, ["save"] = true });
            await Test("save-off-refused-before-send", editor: true, method: "editor.do", parameters: new() { ["command"] = "save" });
            await Test("export-gesture-never-admitted", editor: true, method: "editor.do", parameters: new() { ["command"] = "export=forbidden" });
            await Test("wrong-response-id-poisons", control => control["wrongReplyId"] = true, uncertain: true);
            await Test("oversized-response-poisons", control => control["oversizeReply"] = true, uncertain: true);
            await Test("invalid-utf8-response-poisons", control => control["invalidUtf8Reply"] = true, uncertain: true);
            await Test("repeat-hello-identity-change-poisons", control => control["wrongRepeatIdentity"] = true, method: "world.hello", uncertain: true);
            await Test("extra-provenance-is-not-tuple", control => control["engine"]!["additionalProvenance"] = "unchanged", accepted: true);
            string CopyPackage(string name)
            {
                var copy = Path.Combine(work, "bad-packages", name);
                foreach (var file in Directory.EnumerateFiles(sdk, "*", SearchOption.AllDirectories))
                {
                    var target = Path.Combine(copy, Path.GetRelativePath(sdk, file)); Directory.CreateDirectory(Path.GetDirectoryName(target)!); File.Copy(file, target);
                }
                return copy;
            }
            var corrupt = CopyPackage("changed-runtime"); File.AppendAllText(Path.Combine(corrupt, "profiles/local-development/Release/bin/pixie.dll"), "changed");
            await Test("changed-runtime-inventory-refused", root: corrupt);
            var undeclared = CopyPackage("undeclared"); File.WriteAllText(Path.Combine(undeclared, "unlisted.txt"), "not declared");
            await Test("undeclared-payload-refused", root: undeclared);
            foreach (var name in new[] { "public", "profile-hash", "traversal", "case-collision", "duplicate-json" })
            {
                var bad = CopyPackage(name); var path = Path.Combine(bad, "sdk.json"); var manifest = JsonNode.Parse(File.ReadAllText(path))!.AsObject();
                if (name == "public") manifest["distributionClass"] = "public";
                if (name == "profile-hash") manifest["profiles"]![0]!["manifestSha256"] = new string('f', 64);
                if (name == "traversal") manifest["profiles"]![0]!["manifest"] = "../escape.json";
                if (name == "case-collision")
                {
                    var repeated = manifest["files"]![0]!.DeepClone(); repeated["path"] = repeated["path"]!.GetValue<string>().ToUpperInvariant();
                    manifest["files"]!.AsArray().Add(repeated);
                }
                var text = manifest.ToJsonString();
                if (name == "duplicate-json") text = text.Replace("\"schemaVersion\":1", "\"schemaVersion\":1,\"schemaVersion\":1", StringComparison.Ordinal);
                File.WriteAllText(path, text); await Test("bad-package-" + name, root: bad);
            }
        }
        finally
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[3]))!);
            await File.WriteAllTextAsync(args[3], Records.ToJsonString(new() { WriteIndented = true }));
        }
        Console.WriteLine($"PASS: {Records.Count} synthetic bridge boundary assertions; not installed SDK/Editor/G5 acceptance.");
    }
}
