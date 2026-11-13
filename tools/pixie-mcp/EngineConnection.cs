using System.Diagnostics;
using System.IO.Pipes;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Text.RegularExpressions;
using ModelContextProtocol.Protocol;

namespace PixieMcp;

// An owned local host; no retry after a request has been sent.
internal sealed partial class EngineConnection : IAsyncDisposable
{
    private readonly NamedPipeClientStream pipe;
    private readonly Process process;
    private readonly HostJob job;
    private readonly SemaphoreSlim gate = new(1, 1);
    private readonly string captureRoot;
    private readonly SelectionContext selection;
    private readonly bool editor;
    private readonly bool gameTools;
    private JsonObject operations = new();
    private bool poisoned;
    private long nextId;
    private EngineConnection(Process process, HostJob job, NamedPipeClientStream pipe, string captureRoot, SelectionContext selection, bool editor, bool gameTools)
        => (this.process, this.job, this.pipe, this.captureRoot, this.selection, this.editor, this.gameTools) =
            (process, job, pipe, captureRoot, selection, editor, gameTools);

    public static async Task<EngineConnection> OpenAsync(string? executable, string? project, string captureDirectory,
        string channel, bool editor, TimeSpan startup, CancellationToken cancellationToken,
        IReadOnlyDictionary<string, string> options, string? processSdkRoot)
    {
        await Task.Yield(); // Tool discovery stays available while explicit package preflight runs.
        cancellationToken.ThrowIfCancellationRequested();
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("The qualified transport is Windows named pipes.");
        if (!editor && options.ContainsKey("--editor-assembly")) throw new ArgumentException("--editor-assembly is only available for an explicit Editor host.");
        var selection = SelectionContext.Load(options, processSdkRoot);
        var testBuild = options.GetValueOrDefault("--test-build");
        if (testBuild is not null)
        {
            if (editor) throw new ArgumentException("--test-build requires a player host.");
            (executable, project) = OpenTestBuild(testBuild, selection);
        }
        if (executable is null || project is null) throw new ArgumentException("Select a host and project, or a private test-build manifest.");
        var host = Path.GetFullPath(executable);
        CheckPath(host, "Host");
        if (!File.Exists(host)) throw new IOException("The explicit host artifact is absent.");
        if (options.TryGetValue("--editor-assembly", out var assembly))
        {
            if (!Path.IsPathFullyQualified(executable) || !string.Equals(Path.GetFileName(host), "dotnet.exe", StringComparison.OrdinalIgnoreCase))
                throw new ArgumentException("--editor-assembly requires an explicit absolute dotnet.exe host.");
            if (!Path.IsPathFullyQualified(assembly) || !File.Exists(assembly))
                throw new ArgumentException("--editor-assembly requires an explicit absolute existing assembly path.");
            CheckPath(assembly, "Editor assembly");
        }
        if (!editor && testBuild is null && (!string.Equals(host, selection.PlayerPath, StringComparison.OrdinalIgnoreCase) || SelectionContext.HashFile(host) != selection.PlayerHash))
            throw new IOException("The explicit player is not the selected private SDK player artifact.");
        if (captureDirectory.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar).Contains(".."))
            throw new ArgumentException("Capture startup root cannot contain parent traversal.");
        var root = Path.GetFullPath(captureDirectory);
        if (testBuild is not null)
        {
            var relativeCapture = Path.GetRelativePath(project, root);
            if (!Path.IsPathRooted(relativeCapture) && relativeCapture.Split(Path.DirectorySeparatorChar)[0] != "..")
                throw new ArgumentException("Choose a capture directory outside the verified test bundle.");
        }
        CheckPath(root);
        Directory.CreateDirectory(root);
        CheckPath(root);
        var info = new ProcessStartInfo(host)
        {
            UseShellExecute = false, CreateNoWindow = true,
            RedirectStandardOutput = true, RedirectStandardError = true,
            WorkingDirectory = Path.GetFullPath(project)
        };
        if (assembly is not null) info.ArgumentList.Add(assembly);
        foreach (var argument in new[] { "--project", Path.GetFullPath(project), "--dev-channel", channel,
            "--dev-capture-dir", root, "--dev-hidden", "on" }) info.ArgumentList.Add(argument);
        if (editor)
        {
            foreach (var argument in selection.EditorArguments()) info.ArgumentList.Add(argument);
            info.ArgumentList.Add("--recent"); info.ArgumentList.Add("off");
        }
        Console.Error.WriteLine($"[bridge] host={host} sha256={SelectionContext.HashFile(host)} sdk={selection.Id} build={selection.BuildId} manifest={selection.ManifestHash}");
        cancellationToken.ThrowIfCancellationRequested();
        var process = Process.Start(info) ?? throw new IOException("Host process did not start.");
        // The host and its build tools close with this server, even when it does not exit cleanly.
        var job = HostJob.Contain(process);
        // Neither the native host nor its diagnostics may contaminate MCP stdout.
        _ = RelayAsync(process.StandardOutput);
        _ = RelayAsync(process.StandardError);
        var pipe = new NamedPipeClientStream(".", $"pixie-{channel}", PipeDirection.InOut,
            PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
        var connection = new EngineConnection(process, job, pipe, root, selection, editor, testBuild is not null);
        try
        {
            // The editor opens the channel only after building a project's code, which can take minutes.
            var clock = Stopwatch.StartNew();
            while (true)
            {
                if (process.HasExited) throw new IOException($"Host exited with code {process.ExitCode} before opening its channel.");
                var remaining = startup - clock.Elapsed;
                if (remaining <= TimeSpan.Zero) throw new TimeoutException($"Host did not open its channel within {startup.TotalSeconds:0} seconds.");
                try
                {
                    await pipe.ConnectAsync((int)Math.Min(500, Math.Ceiling(remaining.TotalMilliseconds)), cancellationToken);
                    break;
                }
                catch (TimeoutException) { }
            }
            using var helloDeadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            var helloRemaining = startup - clock.Elapsed;
            if (helloRemaining <= TimeSpan.Zero) throw new TimeoutException($"Host did not complete its handshake within {startup.TotalSeconds:0} seconds.");
            helloDeadline.CancelAfter(helloRemaining);
            JsonNode hello;
            try { hello = await connection.SendAsync("world.hello", new JsonObject(), helloDeadline.Token); }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            { throw new TimeoutException($"Host did not complete its handshake within {startup.TotalSeconds:0} seconds."); }
            var value = hello["result"]?["value"];
            connection.operations = selection.ValidateHello(value, editor);
            var actualRoot = value?["capture_directory"]?.GetValue<string>();
            if (!string.Equals(Path.GetFullPath(actualRoot ?? ""), root, StringComparison.OrdinalIgnoreCase))
                throw new IOException("Host capture directory does not match the configured directory.");
            return connection;
        }
        catch { await connection.DisposeAsync(); throw; }
    }

    private static async Task RelayAsync(StreamReader reader)
    {
        try { while (await reader.ReadLineAsync() is { } line) await Console.Error.WriteLineAsync(line); }
        catch (IOException) { }
    }

    private async Task<JsonNode> SendAsync(string method, JsonNode parameters, CancellationToken cancellationToken, bool admitted = false)
    {
        await gate.WaitAsync(cancellationToken);
        bool sent = false;
        try
        {
            if (poisoned) throw new IOException("Connection is unusable after an uncertain response; no automatic retry.");
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeout.CancelAfter(TimeSpan.FromSeconds(30));
            var id = ++nextId;
            var request = new JsonObject { ["jsonrpc"] = "2.0", ["id"] = id, ["method"] = method, ["params"] = parameters.DeepClone() };
            var body = Encoding.UTF8.GetBytes(request.ToJsonString());
            if (body.Length > 65536) throw new ArgumentException("Engine request exceeds 64 KiB.");
            sent = true;
            await pipe.WriteAsync(body, timeout.Token);
            await pipe.WriteAsync(new byte[] { 10 }, timeout.Token);
            await pipe.FlushAsync(timeout.Token);
            // Only one request is outstanding; extra data after its line is a protocol error.
            // Bound bytes before parsing rather than using an unbounded ReadLineAsync.
            using var response = new MemoryStream();
            var chunk = new byte[4096];
            while (true)
            {
                var count = await pipe.ReadAsync(chunk, timeout.Token);
                if (count == 0) throw new IOException("Host disconnected before its response.");
                var newline = Array.IndexOf(chunk, (byte)10, 0, count);
                var size = newline >= 0 ? newline : count;
                if (response.Length + size >= 4 * 1024 * 1024) throw new IOException("Host response exceeds 4 MiB.");
                response.Write(chunk, 0, size);
                if (newline >= 0)
                {
                    if (newline != count - 1) throw new IOException("Unexpected pipelined host response.");
                    break;
                }
            }
            using var document = SelectionContext.Parse(response.ToArray(), 64);
            var parsed = JsonNode.Parse(document.RootElement.GetRawText(), documentOptions: new() { MaxDepth = 64 })
                ?? throw new IOException("Empty host response.");
            if (parsed is not JsonObject envelope || envelope["id"]?.GetValue<long>() != id || envelope["jsonrpc"]?.GetValue<string>() != "2.0")
                throw new IOException("Host response does not match request.");
            var hasResult = envelope.ContainsKey("result");
            if (hasResult == envelope.ContainsKey("error") || envelope[hasResult ? "result" : "error"] is not JsonObject)
                throw new IOException("Host response must contain exactly one object result or error.");
            if (admitted && method == "world.hello")
            {
                // Retain the send lease until a repeated handshake is verified. No queued
                // request may race ahead of an identity/capability refusal.
                if (parsed["error"] is not null) { poisoned = true; pipe.Dispose(); }
                else if (!JsonNode.DeepEquals(selection.ValidateHello(parsed["result"]?["value"], editor), operations))
                    throw new IOException("Host operation capabilities changed after admission.");
            }
            return parsed;
        }
        catch
        {
            if (sent) { poisoned = true; pipe.Dispose(); }
            throw;
        }
        finally { gate.Release(); }
    }

    public async Task<CallToolResult> CallAsync(string method, JsonNode parameters, CancellationToken cancellationToken)
    {
        try { RequireOperation(method, parameters); }
        catch (Exception e) when (e is IOException or ArgumentException or InvalidOperationException)
        {
            return Failure(e.Message, false);
        }
        try
        {
            var response = await SendAsync(method, parameters, cancellationToken, admitted: true);
            var content = new List<ContentBlock> { new TextContentBlock { Text = response.ToJsonString() } };
            var failed = response["error"] is not null;
            if (method == "world.capture" && !failed)
            {
                var path = Path.GetFullPath(response["result"]?["value"]?["path"]?.GetValue<string>() ?? "");
                if (!string.Equals(Path.GetDirectoryName(path), captureRoot, StringComparison.OrdinalIgnoreCase))
                    throw new IOException("Returned capture is outside the configured root.");
                CheckPath(path);
                var file = new FileInfo(path);
                if (file.Length < 8 || file.Length > 16 * 1024 * 1024) throw new IOException("Capture size is outside the image bound.");
                var bytes = await File.ReadAllBytesAsync(path, cancellationToken);
                if (!bytes.AsSpan(0, 8).SequenceEqual(new byte[] { 137, 80, 78, 71, 13, 10, 26, 10 })) throw new IOException("Capture is not PNG.");
                content.Add(ImageContentBlock.FromBytes(bytes, "image/png"));
            }
            var result = new CallToolResult { IsError = failed, Content = content };
            if (!failed && method.StartsWith("game.", StringComparison.Ordinal))
                result.StructuredContent = JsonSerializer.SerializeToElement(response["result"]?["value"]);
            return result;
        }
        catch (Exception e) when (e is IOException or ArgumentException or OperationCanceledException or JsonException or InvalidOperationException or FormatException or OverflowException or KeyNotFoundException)
        {
            return Failure(e.Message, true);
        }
    }

    private static CallToolResult Failure(string message, bool possiblyApplied) => new()
    {
        IsError = true, Content = [new TextContentBlock { Text = new JsonObject { ["error"] = message,
            ["possibly_applied"] = possiblyApplied, ["retry_automatically"] = false }.ToJsonString() }]
    };

    private void RequireOperation(string method, JsonNode parameters)
    {
        if (method.StartsWith("game.", StringComparison.Ordinal) && !gameTools)
            throw new IOException("Game commands require an explicit private test bundle.");
        var capability = method.StartsWith("inspector.", StringComparison.Ordinal) ? "inspector" :
            method.StartsWith("editor.", StringComparison.Ordinal) ? "editor" : method == "world.capture" ? "capture" : "world";
        if (operations[capability]?.GetValue<bool>() != true) throw new IOException($"Host capability {capability} is unavailable.");
        if (method == "editor.do")
        {
            var command = parameters["command"]?.GetValue<string>() ?? throw new ArgumentException("editor.do requires a command.");
            var verb = command.Split('=', 2)[0].TrimEnd('+');
            if (verb is "build" or "export" or "import" or "drop" or "font" or "image")
                throw new ArgumentException("Disabled development gesture; build/export/import/drop/font/image are unavailable.");
            if ((verb is "save" or "sceneconvert") && operations["save"]?.GetValue<bool>() != true)
                throw new IOException("Host save capability is unavailable.");
        }
    }

    private static void CheckPath(string path, string label = "Capture")
    {
        var part = Path.GetPathRoot(path) ?? throw new IOException("Capture path has no root.");
        foreach (var item in path[part.Length..].Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar))
        {
            if (item.Length == 0) continue;
            part = Path.Combine(part, item);
            if ((File.Exists(part) || Directory.Exists(part)) && (File.GetAttributes(part) & FileAttributes.ReparsePoint) != 0)
                throw new IOException($"{label} path contains a reparse point.");
        }
    }

    public async ValueTask DisposeAsync()
    {
        pipe.Dispose();
        if (!process.HasExited)
        {
            process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync();
        }
        process.Dispose();
        job.Dispose();
        gate.Dispose();
    }

    // Read-only preflight of one explicitly supplied C2 tuple. This does not discover
    // another root, adopt a project, or replace B5's runtime/session selector.
    private sealed class SelectionContext
    {
        private static readonly string[] NativeFields = ["schemaVersion", "sdkId", "sdkBuildId", "cApiVersion", "moduleIdentityContract",
            "platform", "architecture", "compilerId", "compilerVersion", "msvcFullVersion", "stlVersion", "stlUpdate", "iteratorDebugLevel",
            "cppStandard", "configuration", "crt", "cpuFeatures", "profile", "distributionClass", "exceptions", "rtti", "windowsSdkVersion", "dependencyInventorySha256"];
        private static readonly string[] IntegerFields = ["schemaVersion", "cApiVersion", "moduleIdentityContract", "msvcFullVersion", "stlVersion", "stlUpdate", "iteratorDebugLevel", "cppStandard"];
        private static readonly UTF8Encoding StrictUtf8 = new(false, true);
        private readonly JsonElement expected;
        private readonly string root, configuration;
        public string Id { get; }
        public string BuildId { get; }
        public string ManifestHash { get; }
        public string PlayerPath { get; }
        public string PlayerHash { get; }
        public string Configuration => configuration;
        public Dictionary<string, string> RuntimeHashes() =>
            expected.GetProperty("nativeDependencies").EnumerateArray().Append(expected.GetProperty("runtime"))
                .ToDictionary(item => Path.GetFileName(Text(item, "path")), item => Text(item, "sha256"), StringComparer.OrdinalIgnoreCase);
        private SelectionContext(string root, string configuration, string manifestHash, JsonElement identity, string player)
        {
            this.root = root; this.configuration = configuration; expected = identity.Clone();
            ManifestHash = manifestHash; Id = Text(identity, "sdkId"); BuildId = Text(identity, "sdkBuildId"); PlayerPath = player;
            PlayerHash = Text(identity.GetProperty("player"), "sha256");
        }
        public IEnumerable<string> EditorArguments() => ["--sdk", root, "--sdk-id", Id, "--sdk-build-id", BuildId,
            "--sdk-manifest-sha256", ManifestHash, "--sdk-profile", "local-development", "--native-configuration", configuration];

        public static SelectionContext Load(IReadOnlyDictionary<string, string> options, string? processRoot)
        {
            var supplied = options.GetValueOrDefault("--sdk") ?? processRoot;
            if (string.IsNullOrWhiteSpace(supplied)) throw new IOException("Select an explicit private SDK with --sdk or process-local PIXIE_SDK_ROOT.");
            var root = Path.TrimEndingDirectorySeparator(Path.GetFullPath(supplied));
            CheckPath(root, "SDK");
            if (!Directory.Exists(root)) throw new IOException("The explicit SDK root is absent; no fallback is allowed.");
            var profile = options.GetValueOrDefault("--sdk-profile", "local-development");
            var configuration = options.GetValueOrDefault("--native-configuration", "Release");
            if (profile != "local-development" || configuration is not ("Debug" or "Release"))
                throw new IOException("Automation requires the explicit private local-development Debug/Release profile.");
            var bytes = ReadMetadata(CheckedPath(root, "sdk.json"));
            var manifestHash = Convert.ToHexStringLower(SHA256.HashData(bytes));
            using var document = Parse(bytes, 32);
            var manifest = document.RootElement;
            if (manifest.GetProperty("schemaVersion").GetInt32() != 1 || Text(manifest, "distributionClass") != "local-private")
                throw new IOException("Automation requires a separate private SDK distribution.");
            Digest(Text(manifest, "sdkId"));
            if (!Regex.IsMatch(Text(manifest, "productVersion"), "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$"))
                throw new IOException("The selected SDK product version is malformed.");
            Expected(options, "--sdk-id", Text(manifest, "sdkId"));
            Expected(options, "--sdk-manifest-sha256", manifestHash);
            var inventory = new Dictionary<string, (string Hash, long Size)>(StringComparer.OrdinalIgnoreCase);
            foreach (var file in manifest.GetProperty("files").EnumerateArray())
            {
                var relative = Text(file, "path"); var path = CheckedPath(root, relative);
                var hash = Text(file, "sha256"); Digest(hash); _ = Text(file, "role");
                var size = file.GetProperty("size").GetInt64();
                if (size < 0 || relative.Equals("sdk.json", StringComparison.OrdinalIgnoreCase) || inventory.Count >= 65536 || !inventory.TryAdd(relative, (hash, size)))
                    throw new IOException("The SDK inventory is recursive, duplicate or outside its bounds.");
                if (!File.Exists(path) || new FileInfo(path).Length != size || HashFile(path) != hash)
                    throw new IOException($"Selected SDK payload is missing or differs from its inventory: {relative}");
            }
            var pending = new Stack<string>(); pending.Push(root);
            var actual = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var entries = 0;
            while (pending.TryPop(out var directory))
                foreach (var path in Directory.EnumerateFileSystemEntries(directory))
                {
                    if (++entries > 131072) throw new IOException("The SDK directory inventory exceeds its bound.");
                    CheckPath(path, "SDK");
                    if (Directory.Exists(path)) pending.Push(path);
                    else
                    {
                        var relative = Path.GetRelativePath(root, path).Replace('\\', '/');
                        if (!actual.Add(relative) || relative != "sdk.json" && !inventory.ContainsKey(relative))
                            throw new IOException($"Selected SDK contains an undeclared/colliding payload: {relative}");
                    }
                }
            JsonElement? selected = null;
            var tuples = new HashSet<string>(StringComparer.Ordinal);
            foreach (var entry in manifest.GetProperty("profiles").EnumerateArray())
            {
                var kind = Text(entry, "profile"); var config = Text(entry, "configuration");
                if (kind != "local-development" || config is not ("Debug" or "Release") || !tuples.Add(kind + "/" + config))
                    throw new IOException("The private SDK catalog has a mixed, duplicate or unsupported tuple.");
                var relative = Text(entry, "manifest"); var hash = Text(entry, "manifestSha256"); Digest(hash);
                if (relative != $"profiles/{kind}/{config}/profile.json" || !inventory.TryGetValue(relative, out var listed) || listed.Hash != hash)
                    throw new IOException("The profile manifest differs from its catalog/inventory.");
                var profileBytes = ReadMetadata(CheckedPath(root, relative));
                if (Convert.ToHexStringLower(SHA256.HashData(profileBytes)) != hash) throw new IOException("Profile bytes changed after inventory validation.");
                using var profileDocument = Parse(profileBytes, 32);
                var identity = profileDocument.RootElement;
                ValidateIdentity(identity);
                if (Text(identity, "sdkId") != Text(manifest, "sdkId") || Text(identity, "sdkBuildId") != Text(entry, "sdkBuildId") ||
                    Text(identity, "configuration") != config || Text(identity, "profile") != kind)
                    throw new IOException("The SDK/profile identities disagree.");
                _ = Payload(root, identity.GetProperty("runtime"), inventory);
                _ = Payload(root, identity.GetProperty("player"), inventory);
                if (config == configuration) selected = identity.Clone();
            }
            if (selected is not { } chosen) throw new IOException($"The selected SDK has no local-development/{configuration} tuple; no configuration fallback is allowed.");
            Expected(options, "--sdk-build-id", Text(chosen, "sdkBuildId"));
            return new(root, configuration, manifestHash, chosen, Payload(root, chosen.GetProperty("player"), inventory));
        }

        public JsonObject ValidateHello(JsonNode? value, bool editor)
        {
            if (value?["protocol_version"]?.GetValue<int>() != 1 || value["pixie_api_version"]?.GetValue<int>() != NativeApiVersion.Current || value["engine"] is not JsonObject engine)
                throw new IOException("Host development protocol/API/engine identity is absent or unsupported.");
            using var parsed = Parse(Encoding.UTF8.GetBytes(engine.ToJsonString()), 32);
            var actual = parsed.RootElement;
            ValidateIdentity(actual);
            if (!actual.GetProperty("capabilityBits").TryGetUInt64(out var bits) || bits != 7)
                throw new IOException("Host native capability bits are absent or disagree with private automation.");
            foreach (var field in NativeFields)
                if (actual.GetProperty(field).ValueKind != expected.GetProperty(field).ValueKind || !JsonElement.DeepEquals(actual.GetProperty(field), expected.GetProperty(field)))
                    throw new IOException($"Host native identity differs from the selected context: {field}.");
            if (Bits(actual.GetProperty("capabilities")) != Bits(expected.GetProperty("capabilities")))
                throw new IOException("Host native capabilities differ from the selected private context.");
            var operations = value["capabilities"] as JsonObject ?? throw new IOException("Host operation capabilities are absent.");
            bool Operation(string name) => operations[name]?.GetValue<bool>() ?? throw new IOException($"Host operation capability is absent: {name}.");
            if (!Operation("world") || !Operation("transport") || !Operation("capture") || Operation("inspector") != editor ||
                (operations.ContainsKey("editor") ? Operation("editor") : false) != editor || !editor && Operation("save"))
                throw new IOException("Host operation capabilities do not admit the requested player/Editor surface.");
            _ = Operation("save");
            return operations.DeepClone().AsObject();
        }

        private static void ValidateIdentity(JsonElement identity)
        {
            if (identity.ValueKind != JsonValueKind.Object) throw new IOException("Native identity must be an object.");
            foreach (var field in NativeFields) _ = identity.GetProperty(field);
            foreach (var field in IntegerFields)
                if (!identity.GetProperty(field).TryGetInt32(out var number) || number < 0) throw new IOException($"Native identity integer is malformed: {field}.");
            foreach (var field in NativeFields.Except(IntegerFields).Except(["cpuFeatures", "exceptions", "rtti"])) _ = Text(identity, field);
            foreach (var field in new[] { "sdkId", "sdkBuildId", "dependencyInventorySha256" }) Digest(Text(identity, field));
            foreach (var field in new[] { "exceptions", "rtti" }) _ = identity.GetProperty(field).GetBoolean();
            if (identity.GetProperty("schemaVersion").GetInt32() != NativeApiVersion.IdentitySchema || identity.GetProperty("cApiVersion").GetInt32() != NativeApiVersion.Current ||
                identity.GetProperty("moduleIdentityContract").GetInt32() != 1 || identity.GetProperty("cppStandard").GetInt32() != 20 ||
                Text(identity, "platform") != "windows" || Text(identity, "architecture") != "x64" || Text(identity, "compilerId") != "MSVC" ||
                Text(identity, "profile") != "local-development" || Text(identity, "distributionClass") != "local-private" ||
                Text(identity, "configuration") is not ("Debug" or "Release") || Text(identity, "crt") != (Text(identity, "configuration") == "Debug" ? "MDd" : "MD") ||
                !identity.GetProperty("cpuFeatures").EnumerateArray().Select(item => item.GetString()).SequenceEqual(["avx2", "fma", "f16c"]))
                throw new IOException("Unsupported native API/schema/private profile/toolchain tuple.");
            var bits = Bits(identity.GetProperty("capabilities"));
            if (bits != 7 || identity.TryGetProperty("capabilityBits", out var reported) && (!reported.TryGetUInt64(out var actual) || actual != bits))
                throw new IOException("Native automation capability names/bits disagree or are unsupported.");
        }
        private static ulong Bits(JsonElement capabilities) =>
            (capabilities.GetProperty("moduleLoading").GetBoolean() ? 1UL : 0) |
            (capabilities.GetProperty("nativeAutomation").GetBoolean() ? 2UL : 0) |
            (capabilities.GetProperty("candidateAuthoring").GetBoolean() ? 4UL : 0);
        private static string Payload(string root, JsonElement payload, Dictionary<string, (string Hash, long Size)> inventory)
        {
            var relative = Text(payload, "path"); var hash = Text(payload, "sha256"); Digest(hash);
            if (!inventory.TryGetValue(relative, out var entry) || entry.Hash != hash) throw new IOException("Profile payload differs from its SDK inventory.");
            return CheckedPath(root, relative);
        }
        private static string Text(JsonElement value, string name) => value.GetProperty(name).GetString() is { Length: > 0 } text && !text.Contains('\0')
            ? text : throw new IOException($"Missing or malformed SDK text: {name}.");
        private static void Digest(string text) { if (!Regex.IsMatch(text, "^[0-9a-f]{64}$")) throw new IOException("Invalid SDK/artifact SHA256."); }
        private static void Expected(IReadOnlyDictionary<string, string> options, string name, string actual)
        {
            if (!options.TryGetValue(name, out var expected)) return;
            Digest(expected);
            if (expected != actual) throw new IOException($"Selected context differs from {name}.");
        }
        private static string CheckedPath(string root, string relative)
        {
            if (relative.Length > 1024 || relative.Contains('\\') || relative.Contains(':') || relative.Contains('\0') ||
                relative.Split('/').Any(part => part is "" or "." or ".." || part.EndsWith('.') || part.EndsWith(' ')))
                throw new IOException("SDK paths must be normalized root-relative paths.");
            var path = Path.GetFullPath(Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar)));
            if (Path.IsPathFullyQualified(relative) || Path.GetRelativePath(root, path).Split(Path.DirectorySeparatorChar).Contains(".."))
                throw new IOException("SDK payload escapes its explicit root.");
            CheckPath(path, "SDK"); return path;
        }
        public static string HashFile(string path) { using var stream = File.OpenRead(path); return Convert.ToHexStringLower(SHA256.HashData(stream)); }
        public static byte[] ReadMetadata(string path)
        {
            using var file = File.OpenRead(path);
            if (file.Length > 16 * 1024 * 1024) throw new IOException("SDK metadata exceeds 16 MiB.");
            var bytes = new byte[(int)file.Length]; file.ReadExactly(bytes);
            if (file.ReadByte() != -1) throw new IOException("SDK metadata changed while reading.");
            return bytes;
        }
        public static JsonDocument Parse(byte[] bytes, int depth)
        {
            _ = StrictUtf8.GetCharCount(bytes);
            var parsed = JsonDocument.Parse(bytes, new JsonDocumentOptions { MaxDepth = depth });
            try { Unique(parsed.RootElement); return parsed; }
            catch { parsed.Dispose(); throw; }
        }
        private static void Unique(JsonElement value)
        {
            if (value.ValueKind == JsonValueKind.Object)
            {
                var names = new HashSet<string>(StringComparer.Ordinal);
                foreach (var field in value.EnumerateObject())
                { if (!names.Add(field.Name)) throw new JsonException("Duplicate SDK/host JSON member."); Unique(field.Value); }
            }
            else if (value.ValueKind == JsonValueKind.Array) foreach (var item in value.EnumerateArray()) Unique(item);
        }
    }
}
