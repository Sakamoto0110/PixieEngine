using System.Globalization;
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text.Json.Nodes;
using PixieEditor.Native;
using PixieEditor.Ui;

namespace PixieEditor.Editing;

// Serial document commands. UI intents carry authored IDs, never native entity handles.
internal sealed class SceneAuthoring(EngineSession session, Selection selection, History history, SceneDocumentEditor editor)
{
    private sealed record Intent(string Command, ulong Scope, string Name, ulong[] Entities,
        PixieSceneEditToken Token, long HistoryRevision);
    private Intent? Pending;
    internal SceneEditMeasurement LastMeasurement { get; private set; }
    internal bool HasPending => Pending != null;
    private long NextAdmissionRead;
    private string? AdmissionProblem;
    internal string? UnavailableReason
    {
        get
        {
            if (session.ProjectFolder == null) return "Open a project to author Scenes.";
            if (!session.Editing) return "Stop Play to author Scenes.";
            if (Environment.TickCount64 >= NextAdmissionRead)
            {
                NextAdmissionRead = Environment.TickCount64 + 250;
                try { _ = session.CandidateEditRevision(); AdmissionProblem = null; }
                catch (EngineException) { AdmissionProblem = "Scene authoring requires an Engine scene project or an explicitly contracted C++ module."; }
            }
            return AdmissionProblem;
        }
    }
    internal bool NeedsConversion => session.ProjectFolder != null && session.SceneFormatVersion == 1;
    internal bool Run(string command, string argument)
    {
        session.Edits.RequireMutation();
        static ulong Parse(string value) => value.Length == 16 && ulong.TryParse(value, NumberStyles.AllowHexSpecifier,
            CultureInfo.InvariantCulture, out var id) && id != 0 ? id : throw new EngineException("Use a16-digit nonzero Scene scope ID.");
        switch (command)
        {
            case "scenecreate": Create(argument); break;
            case "sceneselect": if (argument == "world") selection.SetWorld(); else Select(Parse(argument)); break;
            case "scenerename":
                var split = argument.IndexOf(':');
                if (split < 0) throw new EngineException("Use scenerename=<scope ID>:<name>.");
                Rename(Parse(argument[..split]), argument[(split + 1)..]); break;
            case "sceneremove": Remove(Parse(argument)); break;
            case "scenemove":
                var ids = selection.Items.Select(session.SceneId).ToArray();
                if (ids.Any(id => id == null)) throw new EngineException("Generated entities cannot be transferred as authored members.");
                Transfer(ids.OfType<ulong>().ToArray(), argument == "world" ? 0 : Parse(argument)); break;
            case "worldpresentation": ApplyPresentation(0, argument); break;
            case "scenepresentation":
                var separator = argument.IndexOf(':');
                if (separator < 0) throw new EngineException("Use scenepresentation=<scope ID>:<settings JSON>.");
                ApplyPresentation(Parse(argument[..separator]), argument[(separator + 1)..]); break;
            case "presentationmigrate":
                if (argument != "confirm") throw new EngineException("Explicit presentation migration requires presentationmigrate=confirm.");
                MigratePresentation(); break;
            case "sceneconvert":
                if (argument != "confirm") throw new EngineException("Explicit conversion requires sceneconvert=confirm; it saves current edits and preserves a .v1-backup.");
                Convert(); break;
            default: return false;
        }
        return true;
    }
    internal void Select(ulong id)
    {
        session.Edits.RequireMutation();
        if (!session.SceneScopes().Any(s => s.Id == id)) throw new EngineException("The Scene no longer exists.");
        selection.SetScope(id);
    }
    internal void Queue(string command, ulong scope = 0, string name = "", ulong[]? entities = null)
    {
        session.Edits.RequireMutation();
        if (Pending is { } previous)
        {
            if ((command == "rename" || command == "presentation") && previous.Command == command && previous.Scope == scope)
                Pending = previous with { Name = name }; // One text gesture, retaining its original freshness token.
            else Status.Show("A Scene command is already waiting. Finish it before another Scene edit.", error: true);
            return;
        }
        Pending = new(command, scope, name, entities ?? [], session.ProjectRevision(), history.Revision);
    }
    internal void ProcessPending()
    {
        if (Pending is not { } intent || session.Edits.GestureActive?.Invoke() == true) return;
        Pending = null;
        try
        {
            if (!intent.Token.Equals(session.ProjectRevision()) || intent.HistoryRevision != history.Revision)
                throw new EngineException("The Scene command expired after another edit. Try it again.");
            switch (intent.Command)
            {
                case "create": Create(intent.Name); break;
                case "rename": Rename(intent.Scope, intent.Name); break;
                case "remove": Remove(intent.Scope); break;
                case "transfer": Transfer(intent.Entities, intent.Scope); break;
                case "convert": Convert(); break;
                case "presentation": ApplyPresentation(intent.Scope, intent.Name); break;
                case "migrate-presentation": MigratePresentation(); break;
                default: throw new EngineException("Unknown Scene command.");
            }
        }
        catch (EngineException error) { Status.Show(error.Message, error: true); }
    }
    internal JsonObject PresentationModel(ulong scope)
    {
        if (Pending is { Command: "presentation" } pending && pending.Scope == scope)
            return JsonNode.Parse(pending.Name)!.AsObject();
        var snapshot = session.PresentationSnapshot();
        if (scope == 0) return snapshot["presentation"]!.DeepClone().AsObject();
        var selected = snapshot["scopes"]!.AsArray().Single(s => Id(s!["id"]) == scope)!;
        return new JsonObject { ["environment"] = selected["environment"]?.DeepClone() ?? new JsonObject
            { ["mode"] = "inherit", ["value"] = snapshot["presentation"]!["environment"]!.DeepClone() },
            ["camera"] = selected["camera"]?.DeepClone(), ["gravity"] = selected["gravity"]!.DeepClone() };
    }
    internal void QueuePresentation(ulong scope, JsonObject value) => Queue("presentation", scope, value.ToJsonString());
    private static void Merge(JsonObject target, JsonObject values)
    {
        foreach (var (key, value) in values)
        {
            if (value is JsonObject nested && target[key] is JsonObject current) Merge(current, nested);
            else target[key] = value?.DeepClone();
        }
    }
    internal void ApplyPresentation(ulong scope, string json)
    {
        RequireV2();
        JsonObject values;
        if (string.IsNullOrWhiteSpace(json)) throw new EngineException("Presentation settings must be a JSON object.");
        try
        {
            values = JsonNode.Parse(json) as JsonObject
                ?? throw new EngineException("Presentation settings must be a JSON object.");
        }
        catch (System.Text.Json.JsonException error)
        {
            throw new EngineException("Invalid presentation settings JSON: " + error.Message);
        }
        if (scope != 0 && (values.ContainsKey("id") || values.ContainsKey("name")))
            throw new EngineException("Presentation edits cannot change Scene identity or name; use the Scene commands.");
        if (scope == 0 && values["legacyEffects"] is { } compatibility &&
            !JsonNode.DeepEquals(compatibility, session.PresentationSnapshot()["presentation"]!["legacyEffects"]))
            throw new EngineException("Retained legacy effects are read-only.");
        var prepared = Change(document =>
        {
            if (scope == 0)
            {
                if (document["presentation"] is not JsonObject root)
                    throw new EngineException("Migrate the legacy lighting adapter explicitly before editing World presentation settings.");
                Merge(root, values);
            }
            else Merge(Scope(document, scope), values);
        });
        editor.Commit(scope == 0 ? "edit of World/view/renderer settings" : "edit of Scene presentation", prepared);
    }
    internal void MigratePresentation()
    {
        RequireV2();
        var canonical = session.PresentationSnapshot()["presentation"]!.DeepClone();
        var prepared = Change(document =>
        {
            if (document.ContainsKey("presentation")) throw new EngineException("Presentation settings are already canonical.");
            var owners = document["entities"]!.AsArray().OfType<JsonObject>()
                .Where(e => e["components"] is JsonObject c && c.ContainsKey("LightingSettings2D")).ToArray();
            if (owners.Length > 1) throw new EngineException("Resolve duplicate legacy lighting authorities before migration.");
            document["presentation"] = canonical;
            foreach (var owner in owners)
            {
                var components = owner["components"]!.AsObject();
                // Retain the complete preimage as opaque compatibility data. It is not another authority.
                document["presentation"]!["legacySource"] = components["LightingSettings2D"]!.DeepClone();
                components.Remove("LightingSettings2D");
            }
        });
        editor.Commit("migration of presentation authority", prepared);
    }

    private void RequireV2()
    {
        session.Edits.RequireCandidateAdmission(false);
        _ = session.CandidateEditRevision();
        if (session.SceneFormatVersion != 2) throw new EngineException("Explicitly convert and save this v1 project before authoring Scenes.");
    }
    internal void Convert()
    {
        _ = session.CandidateEditRevision();
        using var lease = session.Edits.BeginCandidate(false);
        session.ConvertSceneV2(session.ProjectRevision());
    }
    private static ulong Id(JsonNode? node) => node is JsonValue value && value.TryGetValue<string>(out var text) &&
        ulong.TryParse(text, NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var id) && id != 0
        ? id : throw new EngineException("Invalid authored identity in the scene document.");
    private static JsonArray Scopes(JsonObject document) => document["sceneScopes"]!.AsArray();
    private static JsonObject Scope(JsonObject document, ulong id) => Scopes(document).OfType<JsonObject>()
        .SingleOrDefault(s => Id(s["id"]) == id) ?? throw new EngineException("The Scene no longer exists.");
    private static string Name(string name)
    {
        name = name.Trim();
        if (name.Length is 0 or > 256 || name.Contains('\0')) throw new EngineException("Scene names must contain1..256 characters without NUL.");
        return name;
    }
    private SceneDocumentEditor.Prepared Change(Action<JsonObject> change) => editor.Prepare(text =>
    {
        var document = JsonNode.Parse(text)!.AsObject();
        change(document);
        return document.ToJsonString();
    });
    internal ulong Create(string name)
    {
        var started = Stopwatch.GetTimestamp();
        RequireV2(); name = Name(name);
        ulong id = 0;
        var prepared = Change(document =>
        {
            var used = Scopes(document).Select(s => Id(s!["id"])).ToHashSet();
            do { id = BitConverter.ToUInt64(RandomNumberGenerator.GetBytes(sizeof(ulong))); } while (id == 0 || used.Contains(id));
            Scopes(document).Add(new JsonObject { ["id"] = id.ToString("x16"), ["name"] = name });
        });
        LastMeasurement = editor.Commit("creation of Scene", prepared, id) with { TotalMilliseconds = Stopwatch.GetElapsedTime(started).TotalMilliseconds };
        return id;
    }
    internal void Rename(ulong id, string name)
    {
        RequireV2(); name = Name(name);
        if (session.SceneScopes().Any(s => s.Id == id && s.Name == name)) return;
        var prepared = Change(document => Scope(document, id)["name"] = name);
        editor.Commit("rename of Scene", prepared);
    }
    internal void Remove(ulong id)
    {
        var started = Stopwatch.GetTimestamp();
        RequireV2();
        var prepared = Change(document =>
        {
            var scope = Scope(document, id);
            if (document["entities"]!.AsArray().Any(e => e!["sceneScope"] is { } membership && Id(membership) == id))
                throw new EngineException("This Scene is occupied. Move its entities to World or another Scene before removal.");
            Scopes(document).Remove(scope);
        });
        LastMeasurement = editor.Commit("removal of Scene", prepared) with { TotalMilliseconds = Stopwatch.GetElapsedTime(started).TotalMilliseconds };
    }
    internal void Transfer(ulong[] entities, ulong destination)
    {
        RequireV2();
        if (entities.Length == 0 || entities.Distinct().Count() != entities.Length || entities.Contains(0UL))
            throw new EngineException("Select distinct persistent entities before changing membership.");
        var prepared = Change(document =>
        {
            if (destination != 0) _ = Scope(document, destination);
            var byId = document["entities"]!.AsArray().OfType<JsonObject>().ToDictionary(e => Id(e["id"]));
            if (entities.Any(id => !byId.ContainsKey(id))) throw new EngineException("A selected entity is generated or no longer exists. No memberships were changed.");
            foreach (var id in entities)
            {
                if (destination == 0) byId[id].Remove("sceneScope");
                else byId[id]["sceneScope"] = destination.ToString("x16");
            }
        });
        editor.Commit("change of Scene membership", prepared);
    }
}
