using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using ModelContextProtocol.Protocol;
using PixieMcp;
using System.Text.Json.Nodes;

var options = new Dictionary<string, string>(StringComparer.Ordinal);
for (var i = 0; i < args.Length; i += 2)
{
    if (i + 1 >= args.Length || !new[] { "--host", "--project", "--captures", "--channel", "--kind" }.Contains(args[i]))
        throw new ArgumentException("usage: PixieMcp --host <exe> --project <folder> --captures <folder> [--channel <name>] [--kind player|editor]");
    options.Add(args[i], args[i + 1]);
}
foreach (var required in new[] { "--host", "--project", "--captures" })
    if (!options.ContainsKey(required)) throw new ArgumentException($"Missing {required}");
var name = options.GetValueOrDefault("--channel", $"mcp-{Guid.NewGuid():N}");
if (name.Length is < 1 or > 64 || name.Any(c => !char.IsAsciiLetterOrDigit(c) && c != '-' && c != '_')) throw new ArgumentException("Invalid channel name.");
var kind = options.GetValueOrDefault("--kind", "player");
if (kind is not ("player" or "editor")) throw new ArgumentException("kind must be player or editor");
await using var connection = await EngineConnection.OpenAsync(options["--host"], options["--project"], options["--captures"], name, kind == "editor", CancellationToken.None);
var builder = Host.CreateApplicationBuilder();
builder.Logging.AddConsole(logging => logging.LogToStandardErrorThreshold = LogLevel.Trace);
builder.Services.AddMcpServer().WithStdioServerTransport()
    .WithListToolsHandler((_, _) => ValueTask.FromResult(new ListToolsResult { Tools = [.. ToolCatalog.Tools] }))
    .WithCallToolHandler(async (request, cancellationToken) =>
    {
        var tool = request.Params?.Name;
        if (tool is null || !ToolCatalog.Tools.Any(t => t.Name == tool))
            return new CallToolResult { IsError = true, Content = [new TextContentBlock { Text = "Unknown or disabled development tool." }] };
        var arguments = request.Params?.Arguments;
        if (arguments is null || !arguments.TryGetValue("parameters", out var parameters) || parameters.ValueKind != System.Text.Json.JsonValueKind.Object)
            return new CallToolResult { IsError = true, Content = [new TextContentBlock { Text = "parameters must be a JSON object." }] };
        return await connection.CallAsync(tool, JsonNode.Parse(parameters.GetRawText())!, cancellationToken);
    });
await builder.Build().RunAsync();
