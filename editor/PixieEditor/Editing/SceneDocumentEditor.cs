using System.Diagnostics;
using System.Text.Json;
using System.Text.Json.Nodes;
using PixieEditor.Native;
using PixieEditor.Ui;

namespace PixieEditor.Editing;

internal readonly record struct SceneEditMeasurement(PixieSceneEditReceipt Native,
    double PreparationMilliseconds, double NativeCallMilliseconds,
    double HistoryMilliseconds, double RebindMilliseconds, double TotalMilliseconds,
    bool RecoveryRequired, int SnapshotNativeCalls = 0);

// Shared serial Scene authoring boundary. The host supplies the complete
// Inspector/text/viewport gesture predicate and must recover bindings after publication.
internal sealed class SceneDocumentEditor
{
    private readonly EngineSession session;
    private readonly Selection selection;
    private readonly History history;
    private readonly EditRecorder recorder;
    private readonly Func<bool> gestureActive;
    private readonly Action rebind;
    private readonly Guid RequestOwner = Guid.NewGuid();
    internal SceneDocumentEditor(EngineSession session, Selection selection, History history,
        EditRecorder recorder, Func<bool> gestureActive, Action rebind)
    {
        this.session = session; this.selection = selection; this.history = history;
        this.recorder = recorder; this.gestureActive = gestureActive; this.rebind = rebind;
        if (history.Gate != null && !ReferenceEquals(history.Gate, session.Edits))
            throw new EngineException("History belongs to another editor session.");
        history.Gate = session.Edits;
        session.Edits.RetryRecovery = RecoverBindings;
        session.RestoreFragmentCandidate = (fragment, replacing, newIds) =>
            EditFragment("import of entities", fragment, replacing, newIds);
        session.RestoreHistoryFragmentCandidate = ReplaceHistoryFragment;
        session.RestoreHistoryDocumentCandidate = ReplaceHistoryDocument;
        recorder.SceneEditor = this;
    }

    // Immutable serial handoff: no World, descriptor, view, callback or binding references.
    internal sealed record Prepared(Guid Owner, PixieSceneEditToken Token, long HistoryRevision,
        string Before, string After, double PreparationMilliseconds, int SnapshotNativeCalls);

    private sealed record Targets(bool World, ulong[] Ids, ulong? Anchor, ulong? Scope = null);
    internal sealed record Observation(string Document, PixieSceneEditToken Token, long HistoryRevision,
        bool World, ulong[] Ids, ulong? Anchor, ulong? Scope);

    internal Observation ObserveChange()
    {
        session.Edits.RequireMutation();
        _ = session.CandidateEditRevision();
        var targets = Capture();
        history.ReserveRecord();
        return new(session.ProjectSnapshot(), session.ProjectRevision(), history.Revision,
            targets.World, targets.Ids, targets.Anchor, targets.Scope);
    }

    internal void RecordExistingChange(Observation observation)
    {
        session.Edits.RequireMutation();
        var token = session.ProjectRevision();
        if (token.SessionIdentity != observation.Token.SessionIdentity || token.WorldRevision != observation.Token.WorldRevision ||
            history.Revision != observation.HistoryRevision)
            throw new EngineException("The field gesture crossed a World or history replacement.");
        var after = session.ProjectSnapshot();
        if (after == observation.Document) return;
        var beforeTargets = new Targets(observation.World, observation.Ids, observation.Anchor, observation.Scope);
        var step = new SceneStep(this, "edit of properties", observation.Document, after, beforeTargets, Capture());
        history.RecordPrepared(step);
    }
    private sealed class SceneStep(SceneDocumentEditor owner, string name, string before, string after, Targets beforeTargets, Targets afterTargets) : IStep
    {
        public string Name => name;
        internal bool BelongsTo(SceneDocumentEditor candidate) => ReferenceEquals(owner, candidate);
        private void RequireTravel()
        {
            if (!owner.Busy || !owner.Traveling) throw new EngineException("Candidate history requires its admitted coordinator.");
        }
        public uint[] Undo(EngineSession _) { RequireTravel(); owner.Publish(before, beforeTargets); return []; }
        public uint[] Redo(EngineSession _) { RequireTravel(); owner.Publish(after, afterTargets); return []; }
    }

    private Targets? Pending;
    private bool Busy;
    private bool Traveling;
    private PixieSceneEditReceipt Receipt;
    private double NativeMilliseconds;
    internal bool RecoveryRequired => Pending != null;
    internal Exception? RecoveryError { get; private set; }

    private void Admit()
    {
        if (Busy || RecoveryRequired) throw new EngineException("Finish candidate binding recovery before another edit.");
        session.Edits.RequireCandidateAdmission(gestureActive() || recorder.HasPendingEdits);
        _ = session.CandidateEditRevision();
    }

    private Targets Capture() => new(selection.IsWorld,
        selection.Items.Select(session.SceneId).OfType<ulong>().ToArray(),
        selection.Anchor is { } anchor ? session.SceneId(anchor) : null, selection.Scope);

    private static string VersionTwoSnapshot(string snapshot)
    {
        using var envelope = JsonDocument.Parse(snapshot);
        if (envelope.RootElement.GetProperty("version").GetInt32() == 2) return snapshot;
        var document = JsonNode.Parse(snapshot)!.AsObject();
        if (document["version"]!.GetValue<int>() == 1)
        {
            // In-memory history representation only. Saving to a v1 destination still refuses.
            document["version"] = 2;
            document["sceneScopes"] = new JsonArray();
        }
        return document.ToJsonString();
    }

    private void Publish(string document, Targets targets)
    {
        var token = session.ProjectRevision();
        var start = Stopwatch.GetTimestamp();
        Receipt = session.PublishSceneCandidate(document, token);
        NativeMilliseconds = Stopwatch.GetElapsedTime(start).TotalMilliseconds;
        Pending = targets; // References only: the recovery representation already exists.
    }

    internal bool RecoverBindings()
    {
        if (Busy) throw new EngineException("Candidate recovery cannot reenter an active publication or rebind.");
        Busy = true;
        try { return RecoverCore(); }
        finally { Busy = false; }
    }

    private bool RecoverCore()
    {
        if (Pending is not { } targets) return true;
        try
        {
            if (targets.Scope is { } scope)
            {
                if (session.SceneScopes().Any(s => s.Id == scope)) selection.SetScope(scope);
                else selection.SetWorld();
            }
            else if (targets.World) selection.SetWorld();
            else selection.Set(targets.Ids.Select(session.Find).OfType<uint>(),
                targets.Anchor is { } id ? session.Find(id) : null);
            session.Edits.BeginBindingRecovery();
            try { rebind(); }
            finally { session.Edits.EndBindingRecovery(); }
            Pending = null;
            RecoveryError = null;
            session.Edits.Recovered();
            return true;
        }
        catch (Exception error)
        {
            RecoveryError = error; // Committed World/history remain; never pretend to roll back.
            return false;
        }
    }

    internal Prepared Prepare(string document)
    {
        if (document == null || document.Contains('\0') || System.Text.Encoding.UTF8.GetByteCount(document) > 128 * 1024 * 1024)
            throw new EngineException("Invalid or oversized proposed scene document.");
        return Prepare(_ => document);
    }

    internal Prepared Prepare(Func<string, string> transform)
    {
        Admit();
        using var lease = session.Edits.BeginCandidate(false);
        Busy = true;
        var start = Stopwatch.GetTimestamp();
        try
        {
            var token = session.ProjectRevision();
            var historyRevision = history.Revision;
            var before = VersionTwoSnapshot(session.ProjectSnapshot());
            var document = transform(before);
            if (document == null || document.Contains('\0') || System.Text.Encoding.UTF8.GetByteCount(document) > 128 * 1024 * 1024)
                throw new EngineException("Invalid or oversized proposed scene document.");
            if (!session.ProjectRevision().Equals(token) || history.Revision != historyRevision)
                throw new EngineException("The authored scene changed while preparing its snapshot.");
            return new(RequestOwner, token, historyRevision, before, document, Stopwatch.GetElapsedTime(start).TotalMilliseconds,
                session.LastSnapshotNativeCalls);
        }
        finally { Busy = false; }
    }

    internal SceneEditMeasurement Apply(string name, string document)
    {
        var start = Stopwatch.GetTimestamp();
        var prepared = Prepare(document);
        var result = Commit(name, prepared);
        return result with { TotalMilliseconds = Stopwatch.GetElapsedTime(start).TotalMilliseconds };
    }

    internal SceneEditMeasurement Commit(string name, Prepared prepared, ulong? selectScope = null, ulong[]? selectEntities = null)
    {
        Admit();
        if (prepared.Owner != RequestOwner || prepared.HistoryRevision != history.Revision ||
            !prepared.Token.Equals(session.ProjectRevision()))
            throw new EngineException("Prepared scene edit expired after session, content or history changed.");
        using var lease = session.Edits.BeginCandidate(false);
        Busy = true;
        var start = Stopwatch.GetTimestamp();
        try
        {
            var targets = Capture(); // Keep the current selection, not a stale view snapshot.
            var afterTargets = selectEntities != null ? new Targets(false, selectEntities, selectEntities.FirstOrDefault() is var first && first != 0 ? first : null)
                : selectScope is { } scope ? new Targets(scope == 0, [], null, scope == 0 ? null : scope) : targets;
            var step = new SceneStep(this, name, prepared.Before, prepared.After, targets, afterTargets);
            history.ReserveRecord();
            var ready = Stopwatch.GetTimestamp();
            var nativeStart = Stopwatch.GetTimestamp();
            Receipt = session.PublishSceneCandidate(prepared.After, prepared.Token);
            NativeMilliseconds = Stopwatch.GetElapsedTime(nativeStart).TotalMilliseconds;
            Pending = afterTargets;
            var committed = Stopwatch.GetTimestamp();
            history.RecordPrepared(step);
            var recorded = Stopwatch.GetTimestamp();
            RecoverCore();
            return new(Receipt, prepared.PreparationMilliseconds + Stopwatch.GetElapsedTime(start, ready).TotalMilliseconds,
                NativeMilliseconds, Stopwatch.GetElapsedTime(committed, recorded).TotalMilliseconds,
                Stopwatch.GetElapsedTime(recorded).TotalMilliseconds,
                prepared.PreparationMilliseconds + Stopwatch.GetElapsedTime(start).TotalMilliseconds, RecoveryRequired,
                prepared.SnapshotNativeCalls);
        }
        finally { Busy = false; }
    }

    internal uint[] EditFragment(string name, string fragment, ulong[] replacing, bool newIds,
        double offsetX = 0, double offsetY = 0)
    {
        ulong[] assigned = [];
        var prepared = Prepare(_ =>
        {
            var proposal = JsonNode.Parse(session.PrepareFragment(session.ProjectRevision(), fragment, newIds,
                replacing, offsetX, offsetY))!;
            assigned = proposal["ids"]!.AsArray().Select(id => Convert.ToUInt64(id!.GetValue<string>(), 16)).ToArray();
            return proposal["document"]!.ToJsonString();
        });
        var result = new uint[assigned.Length];
        Commit(name, prepared, selectEntities: assigned);
        for (var i = 0; i < assigned.Length; ++i) result[i] = session.Find(assigned[i]) ?? 0;
        return result;
    }

    // Old field/fragment history may predate v2 conversion. Replace its affected set
    // in one candidate; do not destroy published entities before a restore can fail.
    internal uint[] ReplaceHistoryFragment(string fragment, ulong[] replacing)
    {
        Admit();
        using var lease = session.Edits.BeginCandidate(false);
        Busy = true;
        try
        {
            var targets = Capture();
            var token = session.ProjectRevision();
            var proposal = JsonNode.Parse(session.PrepareFragment(token, fragment, false, replacing))!;
            var assigned = proposal["ids"]!.AsArray().Select(id => Convert.ToUInt64(id!.GetValue<string>(), 16)).ToArray();
            var document = proposal["document"]!.ToJsonString();
            var result = new uint[assigned.Length];
            Receipt = session.PublishSceneCandidate(document, token);
            Pending = targets;
            RecoverCore();
            for (var i = 0; i < assigned.Length; ++i) result[i] = session.Find(assigned[i]) ?? 0;
            return result;
        }
        finally { Busy = false; }
    }

    private uint[] ReplaceHistoryDocument(string document)
    {
        Admit();
        var promoted = VersionTwoSnapshot(document);
        using var lease = session.Edits.BeginCandidate(false);
        Busy = true;
        try
        {
            var targets = Capture();
            var selected = new uint[targets.Ids.Length];
            Publish(promoted, targets);
            RecoverCore();
            for (var i = 0; i < targets.Ids.Length; ++i) selected[i] = session.Find(targets.Ids[i]) ?? 0;
            return selected;
        }
        finally { Busy = false; }
    }

    internal void Undo() => Travel(false);
    internal void Redo() => Travel(true);
    internal bool OwnsNext(bool redo) => (redo ? history.NextRedo : history.NextUndo) is SceneStep step && step.BelongsTo(this);
    private void Travel(bool redo)
    {
        Admit();
        var next = redo ? history.NextRedo : history.NextUndo;
        if (next == null) return;
        if (next is not SceneStep step || !step.BelongsTo(this))
            throw new EngineException("This history entry requires its existing authoring path.");
        using var lease = session.Edits.BeginCandidate(false);
        Busy = true;
        Traveling = true;
        try
        {
            if (redo) history.RedoPrepared(session); else history.UndoPrepared(session);
            Traveling = false; // Rebinding must not inherit authority to replay another step.
            RecoverCore();
        }
        finally { Traveling = false; Busy = false; }
    }
}
