using ModelContextProtocol.Protocol;
using System.Text.Json;

namespace PixieMcp;

internal static class ToolCatalog
{
    // These tools expose only the accepted in-memory development surface. Build/export/import
    // and arbitrary editor command aliases are intentionally absent; the editor's save gesture
    // exists only in builds that keep it (world.hello reports capabilities.save).
    public static readonly string[] WorldCommands = ["hello", "stats", "play", "stop", "pause", "resume", "step", "advance",
        "time_scale", "reset", "types", "type", "entities", "entity", "create", "destroy", "rename", "add_component",
        "remove_component", "get", "set", "record_insert", "record_remove", "snapshot", "restore", "release_snapshot",
        "key", "mouse_button", "cursor", "scroll", "pick", "pick_box", "view_to_world", "world_to_view", "capture"];
    public static readonly IReadOnlyList<Tool> Tools = WorldCommands.Select(command => new Tool
    {
        Name = $"world.{command}",
        Description = $"Native World {command}. Use world.type to discover named fields and metadata. Entities use 16-digit hexadecimal scene IDs. World commands never save. See world.hello for host capabilities.",
        InputSchema = JsonSerializer.SerializeToElement(new
        {
            type = "object", properties = new { parameters = new { type = "object", description = "Named command parameters, e.g. {id,field,value}, {count}, {seconds}, or {name} for capture. Empty for no-argument commands." } },
            required = new[] { "parameters" }, additionalProperties = false
        })
    }).ToArray();
    public static IReadOnlyList<Tool> ForHost(bool editor) => editor ? [.. Tools, .. new[]
    {
        "inspector.tree", "inspector.get", "inspector.report", "inspector.set", "editor.do", "editor.history", "editor.selection"
    }.Select(name => new Tool
    {
        Name = name,
        Description = "Editor/Inspector observation or existing gesture. Read inspector.tree first; writes require its generation. " +
            "inspector.set takes {path,value,generation}; editor.do takes {command,generation}. Selection commands use select=<16-digit scene ID>. " +
            "Default Inspector values are held/view units; inspector.get domain live-storage explicitly reads storage. editor.do save writes the scene when world.hello reports capabilities.save; build/export/import are disabled.",
        InputSchema = JsonSerializer.SerializeToElement(new
        {
            type = "object", properties = new { parameters = new { type = "object" } }, required = new[] { "parameters" }, additionalProperties = false
        })
    })] : Tools;
}
