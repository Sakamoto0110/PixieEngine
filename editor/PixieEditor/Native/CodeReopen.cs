using System.Text.Json;

namespace PixieEditor.Native;

// A private handoff across the existing process restart, not a scene format or an implicit save.
internal static class CodeReopen
{
    internal sealed record Request(string Name, string Source, ulong[] Targets);
    internal sealed record CameraChoice(int Kind, ulong Entity, ulong Scene);
    internal sealed record FreeCameraState(double X, double Y, double Zoom);
    internal sealed record EnvironmentChoice(PixieEnvironmentSelectionKind Kind, ulong SceneId);
    internal sealed record State(int Version, string Project, bool Unsaved, string Snapshot, ulong[] Selection, Request? Pending,
        ulong? Anchor = null, EnvironmentChoice? Environment = null, bool World = false, ulong? Scope = null, CameraChoice? Camera = null, FreeCameraState? FreeCamera = null);
    private const int MaxBytes = 128 * 1024 * 1024;

    internal static string Capture(EngineSession session, IReadOnlyList<uint> selected, Request? pending, uint? anchor = null, bool world = false, ulong? scope = null)
    {
        if (session.ProjectFolder == null || !session.Editing)
            throw new EngineException("Stop Play before rebuilding code. Authoring state can only be carried from Edit mode.");
        session.PrepareBindingsReplacement();
        var environment = session.ViewportEnvironmentSelection;
        var camera = session.ViewportCameraSelection; var free = session.FreeCamera;
        var data = new State(2, Path.GetFullPath(session.ProjectFolder), session.Unsaved,
            session.Unsaved ? session.ProjectSnapshot() : "", selected.Select(session.SceneId).OfType<ulong>().ToArray(), pending,
            anchor is { } entity ? session.SceneId(entity) : null, new(environment.Kind, environment.SceneId), world, scope, new(camera.Kind, camera.Entity, camera.Scene), new(free.X, free.Y, free.Zoom));
        var bytes = JsonSerializer.SerializeToUtf8Bytes(data);
        if (bytes.Length > MaxBytes) throw new EngineException("Code rebuild handoff exceeds128MiB. Save the scene explicitly before rebuilding.");
        var path = ProjectCode.CheckedPath(session.ProjectFolder, $".local/editor-reopen/{Guid.NewGuid():N}.json");
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        using var file = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None);
        file.Write(bytes); file.Flush(flushToDisk: true);
        return path;
    }

    internal static State Restore(EngineSession session, string path)
    {
        if (!session.Editing) throw new EngineException("Code state can only be restored in Edit mode.");
        var folder = session.ProjectFolder ?? throw new EngineException("A code handoff requires an open project.");
        var full = ProjectCode.CheckedPath(folder, Path.GetRelativePath(folder, Path.GetFullPath(path)));
        var expected = ProjectCode.CheckedPath(folder, ".local/editor-reopen").TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        if (!full.StartsWith(expected, OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal)
            || !File.Exists(full) || new FileInfo(full).Length > MaxBytes)
            throw new EngineException("The code handoff must be a bounded file in this project's .local/editor-reopen folder.");
        var data = JsonSerializer.Deserialize<State>(File.ReadAllBytes(full)) ?? throw new EngineException("Invalid code handoff.");
        if (data.Version is not (1 or 2) || string.IsNullOrWhiteSpace(data.Project) || !string.Equals(Path.GetFullPath(data.Project), Path.GetFullPath(folder),
                OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal)
            || data.Selection == null || data.Pending is { Targets: null }
            || (data.World && (data.Selection.Length != 0 || data.Anchor != null))
            || (data.Scope != null && (data.Scope == 0 || data.World || data.Selection.Length != 0 || data.Anchor != null))
            || data.Pending is { } request && !ProjectCode.ValidName(request.Name))
            throw new EngineException("The code handoff has an unsupported version, project or component request.");
        if (data.Environment is { } choice &&
            (choice.Kind == PixieEnvironmentSelectionKind.LegacyWorld ? choice.SceneId != 0
                : choice.Kind != PixieEnvironmentSelectionKind.Scene || choice.SceneId == 0))
            throw new EngineException("The code handoff has an invalid viewport Environment selection.");
        if (data.Camera is { } checkedCamera && (checkedCamera.Kind is < 0 or > 2 ||
            (checkedCamera.Kind == 2 ? checkedCamera.Entity == 0 : checkedCamera.Entity != 0) || (checkedCamera.Kind == 0 && checkedCamera.Scene != 0)))
            throw new EngineException("The code handoff has an invalid camera choice.");
        if (data.FreeCamera is { } checkedFree && (!double.IsFinite(checkedFree.X) || !double.IsFinite(checkedFree.Y) ||
            !double.IsFinite(checkedFree.Zoom) || checkedFree.Zoom < 0.0001 || checkedFree.Zoom > 10000))
            throw new EngineException("The code handoff has invalid free-camera values.");
        State Finish()
        {
            if (data.Environment is { } environment)
                session.SelectViewportEnvironment(environment.Kind, environment.SceneId);
            if (data.Camera is { } camera) session.SelectViewportCamera(camera.Kind, camera.Entity, camera.Scene);
            if (data.FreeCamera is { } free) session.FreeCamera = (free.X, free.Y, free.Zoom);
            return data;
        }
        if (!data.Unsaved) return Finish(); // A clean scene is read from disk, never dirtied just by rebuilding.
        if (string.IsNullOrWhiteSpace(data.Snapshot)) throw new EngineException("The unsaved code handoff has no snapshot.");
        if (data.Version == 2)
        {
            try { session.ProjectRestore(data.Snapshot); }
            catch (EngineException error)
            {
                throw new EngineException($"Could not restore unsaved state for the code rebuild. The saved scene remains on disk and the handoff remains at {full}. {error.Message}");
            }
            return Finish();
        }
        if (session.SceneFormatVersion == 2)
            throw new EngineException("A legacy entity-only handoff cannot replace a v2 authored World. The saved scene and handoff remain intact.");
        using var parsed = JsonDocument.Parse(data.Snapshot);
        if (!parsed.RootElement.TryGetProperty("entities", out var array) || array.ValueKind != JsonValueKind.Array)
            throw new EngineException("The code handoff has no entity snapshot; the saved scene was left intact.");
        // Reject the proposed legacy replacement before even capturing rollback data.
        session.ValidateRestore(data.Snapshot, newIds: false,
            replacing: session.Entities().Select(session.SceneId).OfType<ulong>().ToArray());
        var before = session.Snapshot(session.Entities().Where(entity => session.SceneId(entity) != null).ToArray());
        void Replace(string snapshot)
        {
            var current = session.Entities().Where(entity => session.SceneId(entity) != null).ToArray();
            session.ValidateRestore(snapshot, newIds: false, replacing: current.Select(entity => session.SceneId(entity)!.Value).ToArray());
            foreach (var entity in current) session.Destroy(entity);
            session.Restore(snapshot, newIds: false);
            session.Reset();
        }
        try { Replace(data.Snapshot); }
        catch (EngineException error)
        {
            Replace(before);
            throw new EngineException($"Could not restore unsaved state for the code rebuild. The saved scene remains on disk and the handoff remains at {full}. {error.Message}");
        }
        return Finish();
    }

    internal static (int Type, uint[] Entities) ResolveAttachment(EngineSession session, Request request)
    {
        var type = session.TypeIndex(request.Name);
        if (type < 0 || session.ComponentSource(type) != request.Source)
            throw new EngineException($"The rebuilt module has no matching {request.Name} component/source registration. Fix its Describe registration and use Code, Build and reopen; the request and unsaved snapshot are preserved.");
        ProjectCode.ResolveSource(session.ProjectFolder!, request.Source);
        var entities = request.Targets.Select(session.Find).ToArray();
        if (entities.Length == 0 || entities.Any(entity => entity == null))
            throw new EngineException("A code component target no longer exists. The component was not attached to a guessed or partial selection.");
        var resolved = entities.OfType<uint>().Distinct().ToArray();
        foreach (var entity in resolved)
        {
            var reason = session.ComponentAdditionError(entity, type);
            if (reason.Length > 0) throw new EngineException(reason);
        }
        return (type, resolved);
    }
}
