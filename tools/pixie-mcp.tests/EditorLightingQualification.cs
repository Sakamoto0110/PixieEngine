using System.Security.Cryptography;
using System.Diagnostics;
using System.Text.Json.Nodes;
using ModelContextProtocol.Client;
using ModelContextProtocol.Protocol;

// A real authored lighting room: Inspector edit, native readback, image change and undo.
internal static class EditorLightingQualification
{
    public static async Task Run(string[] args)
    {
        if (args.Length != 5)
            throw new ArgumentException("usage: --editor-lighting <editor.exe> <light-lab project> <mcp.dll> <captures> <receipt.json>");
        var project = Path.GetFullPath(args[1]);
        if (File.Exists(Path.Combine(project, "imgui.ini")))
            throw new ArgumentException("Use a fresh disposable room copy without imgui.ini; this fixture requires the default layout.");
        var captures = Path.GetFullPath(args[3]);
        var scene = Path.Combine(project, "scenes", "main.scene");
        var authored = await File.ReadAllBytesAsync(scene);
        var records = new JsonArray();
        void Check(bool passed, string name, JsonNode? evidence = null)
        {
            records.Add(new JsonObject { ["case"] = name, ["passed"] = passed, ["evidence"] = evidence?.DeepClone() });
            if (!passed) throw new InvalidOperationException(name);
        }
        try
        {
            await using var client = await McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions
            {
                Name = "Pixie lighting room qualification", Command = "dotnet", StandardErrorLines = Console.Error.WriteLine,
                Arguments = [Path.GetFullPath(args[2]), "--host", Path.GetFullPath(args[0]), "--project", project,
                    "--captures", captures, "--kind", "editor"]
            }));
            async Task<JsonNode> Good(string name, JsonObject? parameters = null)
            {
                var result = await client.CallToolAsync(name, new Dictionary<string, object?> { ["parameters"] = parameters ?? new JsonObject() });
                var text = result.Content.OfType<TextContentBlock>().FirstOrDefault()?.Text ?? throw new IOException("Missing receipt");
                var receipt = JsonNode.Parse(text)!;
                if (result.IsError == true) throw new IOException(receipt.ToJsonString());
                return receipt["result"]!["value"]!;
            }
            async Task<long> Generation() => (await Good("inspector.tree"))["generation"]!.GetValue<long>();
            async Task Gesture(string command) => await Good("editor.do", new() { ["generation"] = await Generation(), ["command"] = command });
            async Task<byte[]> Capture(string name)
            {
                var result = await client.CallToolAsync("world.capture", new Dictionary<string, object?>
                    { ["parameters"] = new JsonObject { ["name"] = name } });
                Check(result.IsError != true && result.Content.OfType<ImageContentBlock>().Count() == 1, "image-content-" + name);
                var captureReceipt = JsonNode.Parse(result.Content.OfType<TextContentBlock>().First().Text)!;
                Check(captureReceipt["result"]?["value"]?["viewport_visible"]?.GetValue<bool>() == true,
                    "viewport-visible-" + name);
                return await File.ReadAllBytesAsync(Path.Combine(captures, name + ".png"));
            }
            async Task<bool> CompareViewport(string left, string right, string mode)
            {
                var start = new ProcessStartInfo("python") { UseShellExecute = false, CreateNoWindow = true,
                    RedirectStandardOutput = true, RedirectStandardError = true };
                foreach (var argument in new[] { "tests/mcp/compare_lighting_capture.py", Path.Combine(captures, left + ".png"),
                    Path.Combine(captures, right + ".png"), mode }) start.ArgumentList.Add(argument);
                using var process = Process.Start(start) ?? throw new IOException("Comparison did not start");
                var output = process.StandardOutput.ReadToEndAsync();
                var error = process.StandardError.ReadToEndAsync();
                await process.WaitForExitAsync();
                records.Add(new JsonObject { ["case"] = "viewport-comparison-" + mode, ["passed"] = process.ExitCode == 0,
                    ["detail"] = await output, ["stderr"] = await error });
                return process.ExitCode == 0;
            }
            var hello = await Good("world.hello");
            Check(hello["capabilities"]?["inspector"]?.GetValue<bool>() == true, "real-editor-inspector-channel", hello);
            var entities = (await Good("world.entities"))["entities"]!.AsArray();
            var lamp = entities.Single(e => e?["name"]?.GetValue<string>() == "cyan light")!["id"]!.GetValue<string>();
            await Gesture("select=" + lamp);
            async Task<double> Intensity() => (await Good("world.get", new() { ["id"] = lamp, ["field"] = "Light2D.intensity" }))["value"]!.GetValue<double>();
            var original = await Intensity();
            Check(original > 0, "authored-room-lamp-is-lit", JsonValue.Create(original));
            // Editor captures include the window's unsaved marker. Establish the same
            // dirty-but-restored UI state on both sides of the actual visual comparison.
            await Good("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "Light2D.intensity", ["value"] = 0 });
            await Gesture("undo");
            Check(Math.Abs(await Intensity() - original) < 1e-6, "comparison-baseline-restores-light-with-unsaved-marker");
            await Capture("lamp-original");
            var history = (await Good("editor.history"))["undo_count"]!.GetValue<int>();
            await Good("inspector.set", new() { ["generation"] = await Generation(), ["path"] = "Light2D.intensity", ["value"] = 0 });
            Check(await Intensity() == 0, "inspector-switch-off-reaches-native-light");
            Check((await Good("editor.history"))["undo_count"]!.GetValue<int>() == history + 1, "light-edit-is-one-history-step");
            await Capture("lamp-off");
            Check(await Intensity() == 0, "native-light-stays-edited-after-capture");
            Check(await CompareViewport("lamp-original", "lamp-off", "different"), "light-edit-changes-rendered-viewport");
            await Gesture("undo");
            Check(Math.Abs(await Intensity() - original) < 1e-6, "undo-restores-native-intensity");
            var restored = await Capture("lamp-restored");
            Check(await CompareViewport("lamp-original", "lamp-restored", "equal"), "undo-restores-viewport-within-bounded-raster-rounding", JsonValue.Create(Convert.ToHexString(SHA256.HashData(restored))));
            await Gesture("redo");
            Check(await Intensity() == 0, "redo-reapplies-native-intensity");
            await Gesture("undo");
            await Gesture("play");
            await Good("world.pause");
            var step = await Good("world.step", new() { ["count"] = 120 });
            Check(step["steps_run"]?.GetValue<int>() == 120, "lighting-room-runs-120-fixed-steps", step);
            await Capture("room-play-120");
            await Gesture("stop");
            Check(Math.Abs(await Intensity() - original) < 1e-6, "stop-restores-authored-light");
            var finalScene = await File.ReadAllBytesAsync(scene);
            Check(authored.SequenceEqual(finalScene), "authored-scene-file-preserved");
        }
        finally
        {
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(args[4]))!);
            await File.WriteAllTextAsync(args[4], records.ToJsonString(new() { WriteIndented = true }));
        }
        Console.WriteLine($"PASS: {records.Count} lighting-room cases using the official MCP client.");
    }
}


