using System.Text.Json;
using PixieEditor.Native;

internal static class ModuleLoadRetryTests
{
    public static int Run(string native, string work, bool repair = false)
    {
        Directory.CreateDirectory(work);
        var checks = new List<string>();
        void Check(bool condition, string name)
        {
            if (!condition) throw new InvalidOperationException(name);
            checks.Add(name);
            Console.WriteLine("PASS " + name);
        }
        var missing = Path.Combine(Path.GetDirectoryName(Path.GetFullPath(native))!, "missing-retry-module.dll");
        if (File.Exists(missing)) throw new InvalidOperationException("Retry fixture must be absent.");
        for (var attempt = 0; attempt < 2; ++attempt)
        {
            var refused = false;
            try { GameLibrary.Use(missing); }
            catch (EngineException) { refused = true; }
            Check(refused, $"failed-module-load-{attempt}-refused");
            Check(!GameLibrary.IsLoaded, $"failed-module-load-{attempt}-never-admitted");
            var inaccessible = false;
            try { _ = GameLibrary.Loaded; }
            catch (InvalidOperationException) { inaccessible = true; }
            Check(inaccessible, $"failed-module-load-{attempt}-has-no-loaded-path");
        }
        var restart = false;
        try { GameLibrary.Use(native); }
        catch (EngineException) { restart = true; }
        Check(restart && !GameLibrary.IsLoaded, "failed-module-cannot-silently-switch-to-fallback");
        if (repair)
        {
            var fixture = Path.Combine(Path.GetDirectoryName(Path.GetFullPath(native))!, "candidate-authoring-probe-module.dll");
            try
            {
                File.Copy(fixture, missing, overwrite: false);
                GameLibrary.Use(missing);
                Check(GameLibrary.IsLoaded && GameLibrary.Loaded == missing, "repaired-same-path-loads-without-resolver-registration-replay");
                EngineSession.Check(PixieCapi.ModuleLoad(null), "release repaired module");
            }
            finally { if (File.Exists(missing)) File.Delete(missing); }
        }
        File.WriteAllText(Path.Combine(work, "results.json"), JsonSerializer.Serialize(new { passed = checks.Count, checks }));
        return 0;
    }
}
