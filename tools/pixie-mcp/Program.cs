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
    if (i + 1 >= args.Length || !new[] { "--host", "--project", "--captures", "--channel", "--kind", "--startup-timeout" }.Contains(args[i]))
        throw new ArgumentException("usage: PixieMcp --host <exe> --project <folder> --captures <folder> [--channel <name>] [--kind player|editor] [--startup-timeout <seconds>]");
    options.Add(args[i], args[i + 1]);
}
foreach (var required in new[] { "--host", "--project", "--captures" })
    if (!options.ContainsKey(required)) throw new ArgumentException($"Missing {required}");
var name = options.GetValueOrDefault("--channel", $"mcp-{Guid.NewGuid():N}");
if (name.Length is < 1 or > 64 || name.Any(c => !char.IsAsciiLetterOrDigit(c) && c != '-' && c != '_')) throw new ArgumentException("Invalid channel name.");
var kind = options.GetValueOrDefault("--kind", "player");
if (kind is not ("player" or "editor")) throw new ArgumentException("kind must be player or editor");
// A project with code is built before the editor opens its channel; the first build compiles the engine too.
var startup = options.TryGetValue("--startup-timeout", out var seconds)
    ? double.TryParse(seconds, NumberStyles.Float, CultureInfo.InvariantCulture, out var value) && value is > 0 and <= 86400
        ? TimeSpan.FromSeconds(value) : throw new ArgumentException("startup-timeout must be 1..86400 seconds")
    : TimeSpan.FromMinutes(10);

// The MCP server answers its client at once; the host opens in the background and the first call waits for it.
using var stopping = new CancellationTokenSource();
var opening = EngineConnection.OpenAsync(options["--host"], options["--project"], options["--captures"], name, kind == "editor", startup, stopping.Token);
var availableTools = ToolCatalog.ForHost(kind == "editor");
var builder = Host.CreateApplicationBuilder();
builder.Logging.AddConsole(logging => logging.LogToStandardErrorThreshold = LogLevel.Trace);
builder.Services.AddMcpServer().WithStdioServerTransport()
    .WithListToolsHandler((_, _) => ValueTask.FromResult(new ListToolsResult { Tools = [.. availableTools] }))
    .WithCallToolHandler(async (request, cancellationToken) =>
    {
        var tool = request.Params?.Name;
        if (tool is null || !availableTools.Any(t => t.Name == tool))
            return new CallToolResult { IsError = true, Content = [new TextContentBlock { Text = "Unknown or disabled development tool." }] };
        var arguments = request.Params?.Arguments;
        if (arguments is null || !arguments.TryGetValue("parameters", out var parameters) || parameters.ValueKind != System.Text.Json.JsonValueKind.Object)
            return new CallToolResult { IsError = true, Content = [new TextContentBlock { Text = "parameters must be a JSON object." }] };
        EngineConnection connection;
        try { connection = await opening.WaitAsync(cancellationToken); }
        catch (Exception e) when (e is not OperationCanceledException || !cancellationToken.IsCancellationRequested)
        {
            // Nothing was sent to a host that never connected, so nothing was applied.
            return new CallToolResult { IsError = true, Content = [new TextContentBlock
            {
                Text = new JsonObject { ["error"] = "Host did not open its development channel: " + e.Message,
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
