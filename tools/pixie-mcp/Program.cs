using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using ModelContextProtocol.Protocol;
using PixieMcp;
using System.Globalization;
using System.Text.Json.Nodes;

var options = new Dictionary<string, string>(StringComparer.Ordinal);
for (var i = 0; i < args.Length; i += 2)
{
    if (i + 1 >= args.Length || !new[] { "--host", "--project", "--captures", "--channel", "--kind", "--startup-timeout",
        "--sdk", "--sdk-id", "--sdk-build-id", "--sdk-manifest-sha256", "--sdk-profile", "--native-configuration", "--editor-assembly",
        "--test-build" }.Contains(args[i]))
        throw new ArgumentException("usage: PixieMcp --host <explicit exe> --project <folder> --captures <folder> --sdk <private SDK root> " +
            "[--kind player|editor] [--sdk-profile local-development] [--native-configuration Debug|Release] " +
            "[--sdk-id <sha256>] [--sdk-build-id <sha256>] [--sdk-manifest-sha256 <sha256>] " +
            "[--editor-assembly <absolute editor.dll>] [--channel <name>] [--startup-timeout <seconds>] " +
            "or PixieMcp --test-build <absolute pixie-test-build.json> --captures <folder> --sdk <private SDK root>");
    options.Add(args[i], args[i + 1]);
}
var testBuild = options.ContainsKey("--test-build");
if (testBuild && (options.ContainsKey("--host") || options.ContainsKey("--project") || !options.ContainsKey("--sdk")))
    throw new ArgumentException("--test-build requires an explicit --sdk and supplies its own player and project.");
foreach (var required in testBuild ? new[] { "--captures" } : new[] { "--host", "--project", "--captures" })
    if (!options.ContainsKey(required)) throw new ArgumentException($"Missing {required}");
var name = options.GetValueOrDefault("--channel", $"mcp-{Guid.NewGuid():N}");
if (name.Length is < 1 or > 64 || name.Any(c => !char.IsAsciiLetterOrDigit(c) && c != '-' && c != '_')) throw new ArgumentException("Invalid channel name.");
var kind = options.GetValueOrDefault("--kind", "player");
if (kind is not ("player" or "editor")) throw new ArgumentException("kind must be player or editor");
// A cold code project is built with the selected SDK before the editor opens its channel.
var startup = options.TryGetValue("--startup-timeout", out var seconds)
    ? double.TryParse(seconds, NumberStyles.Float, CultureInfo.InvariantCulture, out var value) && value is > 0 and <= 86400
        ? TimeSpan.FromSeconds(value) : throw new ArgumentException("startup-timeout must be 1..86400 seconds")
    : TimeSpan.FromMinutes(10);

// The MCP server answers its client at once; the host opens in the background and the first call waits for it.
using var stopping = new CancellationTokenSource();
var opening = EngineConnection.OpenAsync(options.GetValueOrDefault("--host"), options.GetValueOrDefault("--project"), options["--captures"], name, kind == "editor", startup,
    stopping.Token, options, Environment.GetEnvironmentVariable("PIXIE_SDK_ROOT"));
var availableTools = ToolCatalog.ForHost(kind == "editor");
var builder = Host.CreateApplicationBuilder();
builder.Logging.AddConsole(logging => logging.LogToStandardErrorThreshold = LogLevel.Trace);
builder.Services.AddMcpServer().WithStdioServerTransport()
    .WithListToolsHandler(async (_, cancellationToken) =>
    {
        if (!testBuild) return new ListToolsResult { Tools = [.. availableTools] };
        try
        {
            var connection = await opening.WaitAsync(cancellationToken);
            return new ListToolsResult { Tools = [.. availableTools, .. await connection.ReadGameTools(cancellationToken)] };
        }
        catch (Exception e) when (!cancellationToken.IsCancellationRequested)
        {
            Console.Error.WriteLine($"Game command discovery unavailable: {e.Message}");
            return new ListToolsResult { Tools = [.. availableTools] };
        }
    })
    .WithCallToolHandler(async (request, cancellationToken) =>
    {
        var tool = request.Params?.Name;
        if (tool is null || (!availableTools.Any(t => t.Name == tool) &&
            !(testBuild && (tool.StartsWith("game.player.", StringComparison.Ordinal) || tool.StartsWith("game.dev.", StringComparison.Ordinal)))))
            return new CallToolResult { IsError = true, Content = [new TextContentBlock { Text = "Unknown or disabled development tool." }] };
        var arguments = request.Params?.Arguments;
        if (arguments is null || !arguments.TryGetValue("parameters", out var parameters) || parameters.ValueKind != System.Text.Json.JsonValueKind.Object)
            return new CallToolResult { IsError = true, Content = [new TextContentBlock { Text = "parameters must be a JSON object." }] };
        EngineConnection connection;
        try { connection = await opening.WaitAsync(cancellationToken); }
        catch (Exception e) when (e is not OperationCanceledException || !cancellationToken.IsCancellationRequested)
        {
            // No mutating MCP request is admitted until the startup identity has been verified.
            return new CallToolResult { IsError = true, Content = [new TextContentBlock
            {
                Text = new JsonObject { ["error"] = "Host development startup was refused: " + e.Message,
                    ["possibly_applied"] = false, ["retry_automatically"] = false }.ToJsonString()
            }] };
        }
        return await connection.CallAsync(tool, JsonNode.Parse(parameters.GetRawText())!, cancellationToken);
    });
try { await builder.Build().RunAsync(); }
finally
{
    stopping.Cancel();
    try { await (await opening).DisposeAsync(); }
    catch (Exception) { } // A failed or cancelled opening has already closed its host.
}
