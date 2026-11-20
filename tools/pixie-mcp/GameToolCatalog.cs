using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using ModelContextProtocol.Protocol;

namespace PixieMcp;

internal sealed partial class EngineConnection
{
    internal async Task<IReadOnlyList<Tool>> ReadGameTools(CancellationToken cancellationToken)
    {
        if (!gameTools) return [];
        // The pipe is serialized. Refresh after World replacement instead of keeping stale names or schemas.
        var hello = await SendAsync("world.hello", new JsonObject(), cancellationToken, admitted: true);
        if (hello["error"] is not null) throw new IOException("The host could not refresh game commands.");
        if (hello["result"]?["value"]?["game_commands"] is not JsonArray commands) return [];
        if (commands.Count > 129) throw new IOException("The host game command catalog exceeds its bound.");

        var names = new HashSet<string>(StringComparer.Ordinal);
        var tools = new List<Tool>(commands.Count);
        foreach (var item in commands)
        {
            var name = item?["name"]?.GetValue<string>() ?? "";
            var description = item?["description"]?.GetValue<string>() ?? "";
            if (!Regex.IsMatch(name, @"\Agame\.(player|dev)\.[a-z][a-z0-9_]{0,47}\z") || !names.Add(name) ||
                description.Length is 0 or > 4096 || item?["inputSchema"] is not JsonObject input ||
                input["type"]?.GetValue<string>() != "object" || item["outputSchema"] is not JsonObject output ||
                output["type"]?.GetValue<string>() != "object")
                throw new IOException("The host game command catalog contains invalid metadata.");
            tools.Add(new Tool
            {
                Name = name,
                Description = description,
                InputSchema = JsonSerializer.SerializeToElement(new
                {
                    type = "object", properties = new { parameters = input }, required = new[] { "parameters" }, additionalProperties = false
                }),
                OutputSchema = JsonSerializer.SerializeToElement(output)
            });
        }
        return tools;
    }
}
