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
        "key", "mouse_button", "cursor", "scroll", "pick", "pick_box", "view_to_world", "world_to_view", "capture", "lighting_debug",
        "triggers", "trigger_post"];
    public static readonly IReadOnlyList<Tool> Tools = WorldCommands.Select(command => command switch
    {
        "triggers" => TriggerTool(command,
            "List input trigger bindings and whether each action is registered. Set action_catalog to discover selectable action IDs and labels. Use the ID in world.set for InputTriggers.bindings[index].action.",
            new { id = new { type = "string", pattern = "^[0-9a-fA-F]{16}$" }, action_catalog = new { type = "boolean" },
                offset = Integer(0, int.MaxValue), limit = Integer(1, 256) },
            [], readOnly: true),
        "trigger_post" => TriggerTool(command,
            "Queue one typed event for trigger evaluation in Play. It runs on a later fixed step, after host input. Use world.step to consume it while paused. The event does not change held input state.",
            new
            {
                kind = new { type = "string", @enum = new[] { "key", "mouse_button", "mouse_move", "scroll", "text", "focus", "framebuffer_resize", "joystick_connection" } },
                edge = new { type = "string", @enum = new[] { "press", "release", "repeat" }, description = "Required for key and mouse_button. Repeat is key-only." },
                code = new { type = "integer", minimum = 0, maximum = 0x10FFFF, description = "Required for key (GLFW code), mouse_button, text (Unicode scalar), and joystick_connection (slot 0..15)." },
                modifiers = Integer(0, 63), scan_code = Integer(0, int.MaxValue),
                x = new { type = "number", description = "Framebuffer X in pixels. Required for mouse_button, mouse_move, and scroll." },
                y = new { type = "number", description = "Framebuffer Y in pixels. Required for mouse_button, mouse_move, and scroll." },
                dx = new { type = "number", description = "Horizontal delta. Required for mouse_move and scroll." },
                dy = new { type = "number", description = "Vertical delta. Required for mouse_move and scroll." },
                focused = new { type = "boolean", description = "Required for focus." },
                connected = new { type = "boolean", description = "Required for joystick_connection." },
                width = Integer(0, 16384), height = Integer(0, 16384)
            }, ["kind"], readOnly: false),
        _ => WorldTool(command)
    }).ToArray();

    private static object Integer(int min, int max) => new { type = "integer", minimum = min, maximum = max };

    private static Tool TriggerTool(string command, string description, object fields, string[] required, bool readOnly) => new()
    {
        Name = $"world.{command}",
        Description = description,
        Annotations = new ToolAnnotations { ReadOnlyHint = readOnly, OpenWorldHint = false },
        InputSchema = JsonSerializer.SerializeToElement(new
        {
            type = "object", properties = new { parameters = new { type = "object", properties = fields, required, additionalProperties = false } },
            required = new[] { "parameters" }, additionalProperties = false
        })
    };

    private static Tool WorldTool(string command) => new()
    {
        Name = $"world.{command}",
        Description = $"Native World {command}. Use world.type to discover named fields and metadata. Entities use 16-digit hexadecimal scene IDs. World commands never save. See world.hello for host capabilities.",
        InputSchema = JsonSerializer.SerializeToElement(new
        {
            type = "object", properties = new { parameters = new { type = "object", description = "Named command parameters, e.g. {id,field,value}, {count}, {seconds}, or {name} for capture. Empty for no-argument commands." } },
            required = new[] { "parameters" }, additionalProperties = false
        })
    };
    public static IReadOnlyList<Tool> ForHost(bool editor) => editor ? [.. Tools, .. new[]
    {
        "inspector.tree", "inspector.get", "inspector.report", "inspector.set", "editor.do", "editor.history", "editor.selection", "editor.scenes", "editor.presentation"
    }.Select(name => new Tool
    {
        Name = name,
        Description = "Editor/Inspector observation or existing gesture. Read inspector.tree first; writes require its generation. " +
            "inspector.set takes {path,value,generation}; editor.do takes {command,generation}. Selection commands use select=<16-digit scene ID>. " +
            "editor.scenes lists scope IDs. Scene gestures: scenecreate=name, sceneselect=scopeID/world, scenerename=scopeID:name, sceneremove=scopeID, scenemove=scopeID/world. " +
            "editor.presentation reads native presentation, Scene overrides/gravity and camera catalog. " +
            "In Edit, worldpresentation=JSON and scenepresentation=scopeID:JSON publish one validated candidate with history. " +
            "presentationmigrate=confirm explicitly migrates the in-memory legacy lighting authority; retained legacy effects are read-only. " +
            "sceneconvert=confirm explicitly saves current v1 authorship as v2 with .v1-backup, only when capabilities.save is enabled. " +
            "Default Inspector values are held/view units; inspector.get domain live-storage explicitly reads storage. editor.do save writes the scene when world.hello reports capabilities.save; build/export/import are disabled.",
        InputSchema = JsonSerializer.SerializeToElement(new
        {
            type = "object", properties = new { parameters = new { type = "object" } }, required = new[] { "parameters" }, additionalProperties = false
        })
    })] : Tools;
}
