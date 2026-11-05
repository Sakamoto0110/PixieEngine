using System.Text.Json;

namespace PixieEditor.Native;

// Portable requirements constrain a chosen installation; they never discover one.
internal static class SdkRequirements
{
    internal const string RelativePath = ".pixie-code/sdk.json";
    private static readonly HashSet<string> Allowed = ["schemaVersion", "sdkId", "profile", "platform", "architecture", "compilerVersion", "cpuFeatures", "requiredCapabilities"];

    internal static bool Validate(string folder, EngineSdk sdk)
    {
        var path = ProjectCode.CheckedPath(folder, RelativePath);
        if (!File.Exists(path)) return false;
        try
        {
            using var document = EngineSdk.Parse(EngineSdk.ReadBytes(path, 64 * 1024));
            var requirement = document.RootElement;
            EngineSdk.Schema(requirement); EngineSdk.HashText(requirement, "sdkId");
            if (requirement.EnumerateObject().Any(x => !Allowed.Contains(x.Name)))
                throw new EngineException("Portable SDK requirements cannot contain installation paths or unknown settings.");
            foreach (var field in new[] { "sdkId", "profile", "platform", "architecture", "compilerVersion" })
                if (!JsonElement.DeepEquals(requirement.GetProperty(field), sdk.Identity.GetProperty(field)))
                    throw new EngineException($"This project requires a different Engine SDK ({field}). Choose a matching SDK; adoption must be explicit.");
            var requestedCpu = new HashSet<string>(StringComparer.Ordinal);
            foreach (var entry in requirement.GetProperty("cpuFeatures").EnumerateArray())
                if (entry.GetString() is not { } feature || !requestedCpu.Add(feature))
                    throw new EngineException("Portable SDK requirements contain a malformed or duplicate CPU feature.");
            if (!requestedCpu.SetEquals(sdk.Identity.GetProperty("cpuFeatures").EnumerateArray().Select(x => x.GetString()!)))
                throw new EngineException("This project requires a different Engine SDK (cpuFeatures). Choose a matching SDK; adoption must be explicit.");
            if (requirement.TryGetProperty("requiredCapabilities", out var capabilities))
                foreach (var item in capabilities.EnumerateArray())
                {
                    var name = item.GetString();
                    if (name is not ("moduleLoading" or "nativeAutomation" or "candidateAuthoring") ||
                        !sdk.Identity.GetProperty("capabilities").GetProperty(name).GetBoolean())
                        throw new EngineException("The selected SDK does not provide a required project capability.");
                }
            return true;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException or KeyNotFoundException or InvalidOperationException or FormatException)
        { throw new EngineException($"Cannot read this project's SDK requirements: {error.Message}"); }
    }

    // The caller must explicitly request adoption; unknown or existing requirements are never overwritten.
    internal static void Adopt(string folder, EngineSdk sdk)
    {
        if (!Directory.Exists(folder)) throw new EngineException("Open or create the project before recording its SDK requirement.");
        var path = ProjectCode.CheckedPath(folder, RelativePath);
        if (File.Exists(path)) throw new EngineException("Existing SDK requirements were preserved. Choose their matching SDK or migrate them explicitly.");
        var data = new Dictionary<string, JsonElement>();
        foreach (var field in new[] { "schemaVersion", "sdkId", "profile", "platform", "architecture", "compilerVersion", "cpuFeatures" })
            data[field] = sdk.Identity.GetProperty(field);
        var temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            using (var file = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
            {
                file.Write(JsonSerializer.SerializeToUtf8Bytes(data, new JsonSerializerOptions { WriteIndented = true }));
                file.Flush(true);
            }
            File.Move(temporary, path, overwrite: false);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        { throw new EngineException($"Could not record this project's SDK requirement: {error.Message}"); }
        finally
        {
            try { if (File.Exists(temporary)) File.Delete(temporary); }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException)
            { Console.Error.WriteLine($"[warning] SDK adoption temporary file remains at {temporary}: {error.Message}"); }
        }
    }
}
