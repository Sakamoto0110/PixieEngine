using System.Diagnostics;
using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;
using System.Text.Json.Nodes;

// A project with code opened cold through MCP: the editor builds the code, and the engine with it,
// before it opens its channel, so the server must answer its client at once and wait for the host.
internal static class EditorCodeQualification
{
    public static async Task Run(string[] args)
    {
        if (args.Length != 7)
            throw new ArgumentException("usage: --editor-code <editor.exe> <cold code project> <mcp.dll> <captures> <receipt.json> <folder without a project> <second cold code project>");
        var (editor, project, mcp, captures) = (Path.GetFullPath(args[0]), Path.GetFullPath(args[1]), Path.GetFullPath(args[2]), Path.GetFullPath(args[3]));
        var records = new JsonArray();
        void Check(bool condition, string name, JsonObject? detail = null)
        {
            var record = new JsonObject { ["case"] = name, ["passed"] = condition };
            if (detail != null) foreach (var (key, value) in detail) record[key] = value?.DeepClone();
            records.Add(record);
            if (!condition) throw new InvalidOperationException(name);
        }
        Task<McpClient> Connect(string folder, string captureFolder, params string[] extra) => McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions
        {
            Name = "Pixie code project qualification", Command = "dotnet", StandardErrorLines = Console.Error.WriteLine,
            Arguments = [mcp, "--host", editor, "--project", folder, "--captures", Path.Combine(captures, captureFolder), "--kind", "editor", .. extra]
        }));
        static async Task<CallToolResult> Call(McpClient client, string name, JsonObject? p = null)
            => await client.CallToolAsync(name, new Dictionary<string, object?> { ["parameters"] = p ?? new JsonObject() });
        static string Text(CallToolResult result) => result.Content.OfType<TextContentBlock>().FirstOrDefault()?.Text ?? "";
        try
        {
            // A host that exits before its channel opens is a tool error that applied nothing.
            await using (var client = await Connect(Path.GetFullPath(args[5]), "no-project"))
            {
                var failed = await Call(client, "world.hello");
                var receipt = JsonNode.Parse(Text(failed));
                Check(failed.IsError == true && receipt?["possibly_applied"]?.GetValue<bool>() == false && Text(failed).Contains("exited"),
                    "host-exit-before-channel-is-tool-error", new() { ["text"] = Text(failed) });
            }
            // A startup deadline shorter than the build is a tool error too, and closes the host.
            await using (var client = await Connect(Path.GetFullPath(args[6]), "deadline", "--startup-timeout", "3"))
            {
                var failed = await Call(client, "world.hello");
                Check(failed.IsError == true && Text(failed).Contains("within 3 seconds"), "startup-deadline-is-tool-error", new() { ["text"] = Text(failed) });
            }

            var clock = Stopwatch.StartNew();
            await using var cold = await Connect(project, "code", "--startup-timeout", "1800");
            var tools = await cold.ListToolsAsync();
            var listed = clock.Elapsed.TotalSeconds;
            Check(tools.Any(t => t.Name == "inspector.tree") && listed < 15, "tools-listed-while-the-code-builds", new() { ["seconds"] = listed });
            var hello = await Call(cold, "world.hello");
            var opened = clock.Elapsed.TotalSeconds;
            Check(hello.IsError != true && opened > listed, "cold-code-project-opens-its-channel-after-the-build",
                new() { ["seconds"] = opened, ["hello"] = Text(hello) });
            var types = JsonNode.Parse(Text(await Call(cold, "world.types")))!["result"]!["value"]!;
            Check(types.ToJsonString().Contains("\"Spinner\""), "editor-runs-on-the-built-game-module");
            var entities = JsonNode.Parse(Text(await Call(cold, "world.entities")))!["result"]!["value"]!["entities"]!.AsArray();
            var spinner = entities.First(e => e?["name"]?.GetValue<string>() == "spinner")!["id"]!.GetValue<string>();
            var speed = JsonNode.Parse(Text(await Call(cold, "world.get", new() { ["id"] = spinner, ["field"] = "Spinner.speed" })))!["result"]!["value"]!["value"]!;
            Check(Math.Abs(speed.GetValue<double>() - 1.5) < 1e-9, "game-component-read-through-the-channel");
            await Call(cold, "editor.do", new() { ["generation"] = JsonNode.Parse(Text(await Call(cold, "inspector.tree")))!["result"]!["value"]!["generation"]!.GetValue<long>(), ["command"] = "select=" + spinner });
            var tree = JsonNode.Parse(Text(await Call(cold, "inspector.tree")))!["result"]!["value"]!;
            var set = await Call(cold, "inspector.set", new() { ["generation"] = tree["generation"]!.GetValue<long>(), ["path"] = "Spinner.speed", ["value"] = 90 });
            var stored = JsonNode.Parse(Text(await Call(cold, "world.get", new() { ["id"] = spinner, ["field"] = "Spinner.speed" })))!["result"]!["value"]!["value"]!.GetValue<double>();
            Check(set.IsError != true && Math.Abs(stored - Math.PI / 2) < 1e-6, "game-component-inspector-write-in-display-units", new() { ["stored"] = stored });
        }
        finally
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[4]))!);
            await File.WriteAllTextAsync(args[4], records.ToJsonString(new() { WriteIndented = true }));
        }
        Console.WriteLine($"PASS: {records.Count} cold code-project cases using the official MCP client.");
    }
}
