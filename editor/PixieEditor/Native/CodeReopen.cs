using System.Text.Json;

namespace PixieEditor.Native;

// A private handoff across the existing process restart, not a scene format or an implicit save.
internal static class CodeReopen
{
    internal sealed record Request(string Name, string Source, ulong[] Targets);
    internal sealed record State(int Version, string Project, bool Unsaved, string Snapshot, ulong[] Selection, Request? Pending);
    private const int MaxBytes = 128 * 1024 * 1024;

    internal static string Capture(EngineSession session, IReadOnlyList<uint> selected, Request? pending)
    {
        if (session.ProjectFolder == null || !session.Editing)
            throw new EngineException("Stop Play before rebuilding code. Authoring state can only be carried from Edit mode.");
        session.PrepareBindingsReplacement();
        var entities = session.Entities().Where(entity => session.SceneId(entity) != null).ToArray();
        var data = new State(1, Path.GetFullPath(session.ProjectFolder), session.Unsaved,
            session.Unsaved ? session.Snapshot(entities) : "", selected.Select(session.SceneId).OfType<ulong>().ToArray(), pending);
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
        if (data.Version != 1 || string.IsNullOrWhiteSpace(data.Project) || !string.Equals(Path.GetFullPath(data.Project), Path.GetFullPath(folder),
                OperatingSystem.IsWindows() ? StringComparison.OrdinalIgnoreCase : StringComparison.Ordinal)
            || data.Selection == null || data.Pending is { Targets: null }
            || data.Pending is { } request && !ProjectCode.ValidName(request.Name))
            throw new EngineException("The code handoff has an unsupported version, project or component request.");
        if (!data.Unsaved) return data; // A clean scene is read from disk, never dirtied just by rebuilding.
        if (string.IsNullOrWhiteSpace(data.Snapshot)) throw new EngineException("The unsaved code handoff has no snapshot.");
        using var parsed = JsonDocument.Parse(data.Snapshot);
        if (!parsed.RootElement.TryGetProperty("entities", out var array) || array.ValueKind != JsonValueKind.Array)
            throw new EngineException("The code handoff has no entity snapshot; the saved scene was left intact.");
        var before = session.Snapshot(session.Entities().Where(entity => session.SceneId(entity) != null).ToArray());
        void Replace(string snapshot)
        {
            foreach (var entity in session.Entities().Where(entity => session.SceneId(entity) != null).ToArray()) session.Destroy(entity);
            session.Restore(snapshot, newIds: false);
            session.Reset();
        }
        try { Replace(data.Snapshot); }
        catch (EngineException error)
        {
            Replace(before);
            throw new EngineException($"Could not restore unsaved state for the code rebuild. The saved scene remains on disk and the handoff remains at {full}. {error.Message}");
        }
        return data;
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
