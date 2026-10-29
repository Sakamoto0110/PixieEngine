using ModelContextProtocol.Protocol;
using System.Text.Json;

namespace PixieMcp;

internal static class ToolCatalog
{
    // These tools expose only the accepted in-memory development surface. Save/build/export
    // and arbitrary editor command aliases are intentionally absent.
    public static readonly string[] WorldCommands = ["hello", "stats", "play", "stop", "pause", "resume", "step", "advance",
        "time_scale", "reset", "types", "type", "entities", "entity", "create", "destroy", "rename", "add_component",
        "remove_component", "get", "set", "record_insert", "record_remove", "snapshot", "restore", "release_snapshot",
        "key", "mouse_button", "cursor", "scroll", "pick", "pick_box", "view_to_world", "world_to_view", "capture"];
    public static readonly IReadOnlyList<Tool> Tools = WorldCommands.Select(command => new Tool
    {
        Name = $"world.{command}",
        Description = $"Native World {command}. Use world.type to discover named fields and metadata. Entities use 16-digit hexadecimal scene IDs. Save is disabled. See world.hello for host capabilities.",
        InputSchema = JsonSerializer.SerializeToElement(new
        {
            type = "object", properties = new { parameters = new { type = "object", description = "Named command parameters, e.g. {id,field,value}, {count}, {seconds}, or {name} for capture. Empty for no-argument commands." } },
            required = new[] { "parameters" }, additionalProperties = false
        })
    }).ToArray();
}
