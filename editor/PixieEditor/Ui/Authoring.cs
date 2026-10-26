using PixieEditor.Editing;
using PixieEditor.Native;
using System.Text.Json.Nodes;

namespace PixieEditor.Ui;

internal enum PhysicsPreset { Static, Dynamic, Compound, Kinematic, Sensor, Character, World }

// Changing the scene by hand: creating, deleting, duplicating, renaming, changing components, and
// moving entities, from the entity list, the viewport, the inspector, and the shortcuts. In a
// project's edit mode, each change is one step of the history; while playing, or in a game opened
// without a project, the same changes happen and nothing is recorded, since nothing of them is kept.
internal sealed partial class Authoring(EngineSession session, Selection selection, History history)
{
    // A shape's default size is one unit: too small to see or click.
    private const double ShapeSize = 24;
    // How far a duplicate lands from what it copies, in world units on each axis.
    private const double DuplicateOffset = 16;

    public History History => history;

    // Names are scene data; slots never follow list order. A rename/clear is one native
    // snapshot history step and never writes any collider category or mask.
    public bool RenameCollisionLayer(int slot, string name)
    {
        return Guard(() =>
        {
            if (slot is < 1 or > 31) throw new EngineException("Collision layer slots are 1..31.");
            var type = session.TypeIndex("CollisionLayers2D");
            if (type < 0) throw new EngineException("Rebuild the library for collision layer authoring.");
            var owners = session.Entities().Where(e => session.Has(e, type)).ToArray();
            if (owners.Length > 1) throw new EngineException("Remove the duplicate CollisionLayers2D configuration first.");
            var created = owners.Length == 0;
            var owner = created ? session.CreateEntity("collision layers") : owners[0];
            try
            {
                if (created) session.AddComponent(owner, type);
                var list = session.FieldIndex(type, "layers");
                var (item, count) = session.RecordInfo(owner, type, list);
                var slotField = session.FieldIndex(item, "slot");
                var nameField = session.FieldIndex(item, "name");
                var matches = Enumerable.Range(0, count).Where(i => session.GetRecord(owner, type, list, i, slotField).X == slot).ToArray();
                if (matches.Length > 1) throw new EngineException($"Remove duplicate records for layer {slot} first.");
                if (matches.Length == 0 && count >= 256) throw new EngineException("Remove invalid name records before adding another layer name.");
                void Write(uint entity)
                {
                    var index = matches.Length == 0 ? count : matches[0];
                    if (matches.Length == 0)
                    {
                        session.EditRecord(entity, type, list, index, remove: false);
                        session.SetRecord(entity, type, list, index, slotField, new PixieValue { X = slot });
                    }
                    session.SetRecordText(entity, type, list, index, nameField, name.Trim());
                }
                if (created)
                {
                    Write(owner);
                    Record("creation of collision layer names", [owner], before: null, after: true);
                }
                else Change([owner], $"rename of collision layer {slot}", Write);
                return true;
            }
            catch { if (created) session.Destroy(owner); throw; }
        });
    }

    public void EditRecord(uint entity, int type, int list, int index, bool remove) =>
        Change([entity], remove ? "removal of shape" : "addition of shape", e => session.EditRecord(e, type, list, index, remove));

    public void SetRecordPoints(uint entity, int type, int list, int index, int field, IReadOnlyList<(double X, double Y)> points) =>
        Change([entity], "edit of shape points", e => session.SetRecordPoints(e, type, list, index, field, points));

    public void SetRecordText(uint entity, int type, int list, int index, int field, string text) =>
        Change([entity], "edit of shape text", e => session.SetRecordText(e, type, list, index, field, text));

    public bool MakeCompound(IReadOnlyList<uint> entities)
    {
        return Guard(() =>
        {
            var collider = session.TypeIndex("Collider2D");
            if (entities.Count == 0 || session.TypeIndex("CompoundCollider2D") < 0 || entities.Any(e => !session.Has(e, collider)))
                throw new EngineException("Select entities with Collider2D to convert their geometry.");
            var (ids, before) = Take(entities);
            if (before == null || ids.Length != entities.Count)
                throw new EngineException("Compound conversion needs authored scene entities.");
            var document = JsonNode.Parse(before)!;
            foreach (var entity in document["entities"]!.AsArray())
            {
                var components = entity!["components"]!.AsObject();
                if (components.ContainsKey("CompoundCollider2D") || components.ContainsKey("CharacterMotor2D"))
                    throw new EngineException("Remove conflicting geometry/movement components before conversion.");
                var original = components["Collider2D"]!.DeepClone();
                components.Remove("Collider2D");
                components["CompoundCollider2D"] = new JsonObject
                {
                    ["v"] = 1, ["enabled"] = true, ["shapes"] = new JsonArray(original)
                };
            }
            // Snapshots are the existing native scene transport. Move the entire saved collider,
            // including unknown fields; no physics values or backend state are reconstructed here.
            foreach (var entity in entities) session.Destroy(entity);
            uint[] restored;
            try { restored = session.Restore(document.ToJsonString(), newIds: false); }
            catch { selection.Set(session.Restore(before, newIds: false)); throw; }
            selection.Set(restored);
            if (Recording) history.Record(new ReplaceStep("conversion to compound", ids, before, session.Snapshot(restored)));
            return true;
        });
    }

    public static readonly (PhysicsPreset Kind, string Label)[] PhysicsPresets =
    [
        (PhysicsPreset.Static, "Static obstacle"), (PhysicsPreset.Dynamic, "Dynamic box"),
        (PhysicsPreset.Compound, "Compound box"),
        (PhysicsPreset.Kinematic, "Kinematic platform"), (PhysicsPreset.Sensor, "Sensor region"),
        (PhysicsPreset.Character, "Character"), (PhysicsPreset.World, "World settings")
    ];

    public uint? CreateMovementEntity(bool character, (double X, double Y)? at = null) =>
        CreatePhysicsEntity(character ? PhysicsPreset.Character : PhysicsPreset.Static, at);

    // One creation/history step, using the engine's reflected fields and serialized scene.
    public uint? CreatePhysicsEntity(PhysicsPreset preset, (double X, double Y)? at = null)
    {
        return Guard(() =>
        {
            var character = preset == PhysicsPreset.Character;
            var world = preset == PhysicsPreset.World;
            var movingBody = preset is PhysicsPreset.Dynamic or PhysicsPreset.Compound or PhysicsPreset.Kinematic;
            string[] required = world ? ["PhysicsSettings2D"]
                : character ? ["Transform2D", "Shape", "CharacterMotor2D", "KeyboardControl2D"]
                : preset == PhysicsPreset.Compound ? ["Transform2D", "Shape", "CompoundCollider2D", "RigidBody2D"]
                : movingBody ? ["Transform2D", "Shape", "Collider2D", "RigidBody2D"]
                : ["Transform2D", "Shape", "Collider2D"];
            if (required.Any(name => session.TypeIndex(name) < 0))
                throw new EngineException("Rebuild the game library for physics authoring.");
            if (world)
            {
                var settings = session.TypeIndex("PhysicsSettings2D");
                foreach (var existing in session.Entities())
                    if (session.Has(existing, settings))
                    {
                        selection.Set(existing);
                        Status.Show("Selected the scene's existing world settings.");
                        return existing;
                    }
            }
            var transform = session.TypeIndex("Transform2D");
            var shape = session.TypeIndex("Shape");
            var prefix = preset switch
            {
                PhysicsPreset.Static => "collider", PhysicsPreset.Dynamic => "dynamic box",
                PhysicsPreset.Compound => "compound box",
                PhysicsPreset.Kinematic => "platform", PhysicsPreset.Sensor => "sensor",
                PhysicsPreset.Character => "character", _ => "world physics"
            };
            var names = session.Entities().Select(session.EntityName).ToHashSet();
            var number = Enumerable.Range(1, names.Count + 1).First(n => !names.Contains($"{prefix} {n}"));
            var entity = session.CreateEntity($"{prefix} {number}");
            try
            {
                foreach (var name in required) session.AddComponent(entity, session.TypeIndex(name));
                if (!world)
                {
                    if (at is { } point) Set(transform, "position", new PixieValue { X = point.X, Y = point.Y });
                    var size = preset switch
                    {
                        PhysicsPreset.Character => new PixieValue { X = 12, Y = 24 },
                        PhysicsPreset.Dynamic or PhysicsPreset.Compound => new PixieValue { X = 20, Y = 28 },
                        PhysicsPreset.Kinematic => new PixieValue { X = 80, Y = 8 },
                        PhysicsPreset.Sensor => new PixieValue { X = 36, Y = 40 },
                        _ => new PixieValue { X = 80, Y = 12 }
                    };
                    Set(shape, "kind", new PixieValue { X = 3 });
                    Set(shape, "size", size);
                    Set(shape, "color", preset switch
                    {
                        PhysicsPreset.Character => new PixieValue { X = 70, Y = 225, Z = 180, W = 255 },
                        PhysicsPreset.Dynamic or PhysicsPreset.Compound => new PixieValue { X = 235, Y = 165, Z = 65, W = 255 },
                        PhysicsPreset.Kinematic => new PixieValue { X = 175, Y = 120, Z = 230, W = 255 },
                        PhysicsPreset.Sensor => new PixieValue { X = 80, Y = 200, Z = 240, W = 80 },
                        _ => new PixieValue { X = 85, Y = 105, Z = 140, W = 255 }
                    });
                    if (preset == PhysicsPreset.Compound)
                    {
                        var compound = session.TypeIndex("CompoundCollider2D");
                        var collider = session.TypeIndex("Collider2D");
                        session.SetRecord(entity, compound, session.FieldIndex(compound, "shapes"), 0,
                            session.FieldIndex(collider, "halfSize"), size);
                    }
                    else if (!character)
                    {
                        var collider = session.TypeIndex("Collider2D");
                        Set(collider, "halfSize", size);
                        if (preset == PhysicsPreset.Sensor) Set(collider, "sensor", new PixieValue { X = 1 });
                    }
                    if (preset == PhysicsPreset.Kinematic)
                    {
                        var body = session.TypeIndex("RigidBody2D");
                        Set(body, "type", new PixieValue { X = 1 });
                        Set(body, "initialVelocity", new PixieValue { X = 40 });
                    }
                }
                Record($"creation of {prefix} {number}", [entity], before: null, after: true);
            }
            catch
            {
                session.Destroy(entity);
                throw;
            }
            selection.Set(entity);
            return entity;

            void Set(int type, string field, PixieValue value) => session.Set(entity, type, session.FieldIndex(type, field), value);
        });
    }

    public bool Recording => session.ProjectFolder != null && session.Editing;

    // A scene entity named "entity N", with the first N no entity has, with a transform at `at` (world
    // units; the origin when null) and a circle, chosen as the selection.
    public uint? CreateEntity((double X, double Y)? at = null)
    {
        return Guard(() =>
        {
            var names = session.Entities().Select(session.EntityName).ToHashSet();
            var number = Enumerable.Range(1, names.Count + 1).First(n => !names.Contains($"entity {n}"));
            var entity = session.CreateEntity($"entity {number}");
            if (session.TypeIndex("Transform2D") is var transform and >= 0)
            {
                session.AddComponent(entity, transform);
                if (at is { } point)
                    session.Set(entity, transform, session.FieldIndex(transform, "position"), new PixieValue { X = point.X, Y = point.Y });
            }

            if (session.TypeIndex("Shape") is var shape and >= 0)
            {
                session.AddComponent(entity, shape);
                session.Set(entity, shape, session.FieldIndex(shape, "size"), new PixieValue { X = ShapeSize, Y = ShapeSize });
            }

            selection.Set(entity);
            Record($"creation of entity {number}", [entity], before: null, after: true);
            return entity;
        });
    }

    // One sprite per PNG among `files`, named after its file, placed from `at` (world units) on, each a
    // little to the side of the one before; they become the selection, as one step. Images from outside
    // the project are copied into it first (ProjectAssets). A sprite drawn larger than `fit` world units
    // on either side is scaled down to it, so a large image does not cover the view. Returns the
    // sprites made.
    public uint[] PlaceImages(IReadOnlyList<string> files, (double X, double Y) at, double fit = double.PositiveInfinity)
    {
        var images = files.Where(ProjectAssets.IsImage).ToArray();
        var skipped = files.Where(f => !ProjectAssets.IsImage(f)).Select(Path.GetFileName).ToArray();
        if (skipped.Length > 0)
            Status.Show($"Only PNG images become sprites; left out: {string.Join(", ", skipped)}.", error: true);
        if (images.Length == 0)
            return [];

        return Guard(() =>
        {
            var transform = session.TypeIndex("Transform2D");
            var sprite = session.TypeIndex("Sprite");
            if (transform < 0 || sprite < 0)
            {
                Status.Show("This game has no Sprite component.", error: true);
                return Array.Empty<uint>();
            }

            var position = session.FieldIndex(transform, "position");
            var scale = session.FieldIndex(transform, "scale");
            var image = session.FieldIndex(sprite, "image");
            var names = session.Entities().Select(session.EntityName).ToHashSet();
            var made = new List<uint>();
            var copied = new List<string>();
            foreach (var file in images)
            {
                var (path, copiedTo) = ProjectAssets.Import(session.ProjectFolder, file);
                if (copiedTo != null)
                    copied.Add(copiedTo);
                var stem = Path.GetFileNameWithoutExtension(file);
                var name = Enumerable.Range(1, names.Count + 1).Select(n => n == 1 ? stem : $"{stem} {n}").First(n => !names.Contains(n));
                names.Add(name);

                var entity = session.CreateEntity(name);
                session.AddComponent(entity, transform);
                var offset = made.Count * DuplicateOffset;
                session.Set(entity, transform, position, new PixieValue { X = at.X + offset, Y = at.Y + offset });
                session.AddComponent(entity, sprite);
                session.SetText(entity, sprite, image, path);
                if (session.Image(path, out _) is { } size && Math.Max(size.Width, size.Height) > fit)
                {
                    var factor = fit / Math.Max(size.Width, size.Height);
                    session.Set(entity, transform, scale, new PixieValue { X = factor, Y = factor });
                }

                made.Add(entity);
            }

            selection.Set(made);
            Record(made.Count == 1 ? $"creation of sprite {Describe(made[0])}" : $"creation of {made.Count} sprites", made, before: null, after: true);
            if (copied.Count > 0)
                Status.Show($"Copied into the project: {string.Join(", ", copied)}");
            return made.ToArray();
        }) ?? [];
    }

    // Writes a text field of the entities' component `type`, as one step; a file chosen for an asset
    // field comes in through ImportAsset first.
    public void SetText(IReadOnlyList<uint> entities, int type, int field, string text)
    {
        if (entities.All(e => session.GetText(e, type, field) == text))
            return;
        Change(entities, $"edit of {session.TypeName(type)}.{session.FieldName(type, field)}", e => session.SetText(e, type, field, text));
    }

    // Replaces a list of points of the entities' component `type`, as one step.
    public void SetPoints(IReadOnlyList<uint> entities, int type, int field, IReadOnlyList<(double X, double Y)> points)
    {
        if (entities.All(e => session.GetPoints(e, type, field).SequenceEqual(points)))
            return;
        Change(entities, $"edit of {session.TypeName(type)}.{session.FieldName(type, field)}", e => session.SetPoints(e, type, field, points));
    }

    // Adds a point after the last, 32 units to the right of it (at the origin for an empty list); one step.
    public void AddPoint(IReadOnlyList<uint> entities, int type, int field)
    {
        if (entities.Count == 0)
            return;
        var points = session.GetPoints(entities[0], type, field).ToList();
        points.Add(points.Count == 0 ? (0, 0) : (points[^1].X + 32, points[^1].Y));
        SetPoints(entities, type, field, points);
    }

    // Removes the point at `index`; one step.
    public void RemovePoint(IReadOnlyList<uint> entities, int type, int field, int index)
    {
        if (entities.Count == 0)
            return;
        var points = session.GetPoints(entities[0], type, field).ToList();
        if (index < 0 || index >= points.Count)
            return;
        points.RemoveAt(index);
        SetPoints(entities, type, field, points);
    }

    // The path a component names for a file of asset kind `kind` ("png", "ttf"), copying it into the
    // project when it is outside; null, with the reason shown, when it cannot be used.
    public string? ImportAsset(string file, string kind)
    {
        if (!ProjectAssets.IsKind(file, kind))
        {
            Status.Show($"Only .{kind} files can be used here: {Path.GetFileName(file)}.", error: true);
            return null;
        }

        try
        {
            var (path, copiedTo) = ProjectAssets.Import(session.ProjectFolder, file);
            if (copiedTo != null)
                Status.Show($"Copied into the project: {copiedTo}");
            return path;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            Status.Show($"Could not copy {Path.GetFileName(file)} into the project: {e.Message}", error: true);
            return null;
        }
    }

    public void Delete(IReadOnlyList<uint> entities)
    {
        if (entities.Count == 0)
            return;
        Guard(() =>
        {
            var name = entities.Count == 1 ? $"deletion of {Describe(entities[0])}" : $"deletion of {entities.Count} entities";
            var (ids, before) = Take(entities);
            foreach (var entity in entities)
                session.Destroy(entity);
            selection.Prune(session);
            if (Recording && ids.Length > 0)
                history.Record(new ReplaceStep(name, ids, before, null));
            return 0;
        });
    }

    // Copies the selected scene entities a little to the side and selects the copies; entities the
    // game generates are not in the scene and are not copied.
    public void Duplicate(IReadOnlyList<uint> entities)
    {
        Guard(() =>
        {
            var inScene = entities.Where(e => session.SceneId(e) != null).ToArray();
            if (inScene.Length == 0)
            {
                Status.Show("Nothing to duplicate: only entities of the scene can be.", error: true);
                return 0;
            }

            var copies = session.Restore(session.Snapshot(inScene), newIds: true);
            if (session.TypeIndex("Transform2D") is var transform and >= 0)
            {
                var position = session.FieldIndex(transform, "position");
                foreach (var copy in copies.Where(c => session.Has(c, transform)))
                {
                    var at = session.Get(copy, transform, position);
                    session.Set(copy, transform, position, at with { X = at.X + DuplicateOffset, Y = at.Y + DuplicateOffset });
                }
            }

            selection.Set(copies);
            Record(copies.Length == 1 ? $"duplicate of {Describe(inScene[0])}" : $"duplicate of {copies.Length} entities",
                copies, before: null, after: true);
            return 0;
        });
    }

    public void Rename(uint entity, string name)
    {
        if (session.EntityName(entity) == name)
            return;
        Change([entity], $"rename to \"{name}\"", e => session.SetName(e, name));
    }

    // Adds a component of `type` to the entities, as one step. A Text without a font gets the project's
    // first TrueType font, so it shows at once.
    public void AddComponent(IReadOnlyList<uint> entities, int type)
    {
        // Preflight the whole selection before changing any entity, so a rejected batch cannot
        // leave an unrecorded partial edit. The native add call checks again at the mutation.
        if (AdditionError(entities, type) is { } reason)
        {
            Status.Show(reason, error: true);
            return;
        }
        var name = session.TypeName(type);
        var font = name == "Text" ? ProjectAssets.First(session.ProjectFolder, "ttf") : null;
        Change(entities, $"addition of {name}", e =>
        {
            session.AddComponent(e, type);
            if (font != null && session.GetText(e, type, session.FieldIndex(type, "font")).Length == 0)
                session.SetText(e, type, session.FieldIndex(type, "font"), font);
        });
    }

    public string? AdditionError(IReadOnlyList<uint> entities, int type)
    {
        foreach (var entity in entities)
        {
            var reason = session.ComponentAdditionError(entity, type);
            if (reason.Length > 0) return $"{Describe(entity)}: {reason}";
        }
        // A singleton can pass each individual check when absent. Do not add it to a batch.
        // The native second add would reject, but this avoids a partial history entry.
        if (session.IsSingleton(type) && entities.Count(e => !session.Has(e, type)) > 1)
            return $"{session.TypeName(type)} belongs to one entity. Select a single entity.";
        return null;
    }

    // Changes the entities one by one, as one step: adding or removing a component.
    public void Change(IReadOnlyList<uint> entities, string name, Action<uint> change)
    {
        Guard(() =>
        {
            var (ids, before) = Take(entities);
            foreach (var entity in entities)
                change(entity);
            if (Recording && ids.Length > 0)
                history.Record(new ReplaceStep(name, ids, before, session.Snapshot(entities)));
            return 0;
        });
    }

    // Starts moving the entities that have a transform; `anchor` is the one under the cursor, which
    // snapping aligns.
    public Move? BeginMove(IReadOnlyList<uint> entities, uint anchor)
    {
        if (session.TypeIndex("Transform2D") is not (var transform and >= 0))
            return null;
        var position = session.FieldIndex(transform, "position");
        var moved = entities.Where(e => session.Has(e, transform)).ToArray();
        if (moved.Length == 0)
            return null;
        var before = moved.Select(e => session.Get(e, transform, position)).ToArray();
        var anchorAt = session.Has(anchor, transform) ? session.Get(anchor, transform, position) : before[0];
        return new Move(session, transform, position, moved, before, anchorAt);
    }

    // Ends a move: one step, when something moved and the project is in edit mode.
    public void EndMove(Move move)
    {
        var changes = new List<FieldStep.Change>();
        for (var i = 0; i < move.Entities.Length; i++)
        {
            if (!session.IsAlive(move.Entities[i]) || session.SceneId(move.Entities[i]) is not { } id)
                continue;
            var after = session.Get(move.Entities[i], move.Type, move.Field);
            if (!EditRecorder.Same(move.Before[i], after))
                changes.Add(new FieldStep.Change(id, move.Type, move.Field, move.Before[i], after));
        }

        if (Recording && changes.Count > 0)
            history.Record(new FieldStep(changes.Count == 1 ? $"move of {Describe(move.Entities[0])}" : $"move of {changes.Count} entities", changes));
    }

    public void Undo()
    {
        if (!session.Editing && session.ProjectFolder != null)
        {
            Status.Show("Undo waits for Stop.", error: true);
            return;
        }

        Guard(() =>
        {
            if (history.Undo(session) is { } chosen)
                selection.Set(chosen);
            return 0;
        });
    }

    public void Redo()
    {
        if (!session.Editing && session.ProjectFolder != null)
        {
            Status.Show("Redo waits for Stop.", error: true);
            return;
        }

        Guard(() =>
        {
            if (history.Redo(session) is { } chosen)
                selection.Set(chosen);
            return 0;
        });
    }

    public string Describe(uint entity)
    {
        var name = session.IsAlive(entity) ? session.EntityName(entity) : "";
        return name.Length > 0 ? name : $"#{entity & 0xFFFFF}";
    }

    // The scene IDs of the entities that are in the scene, and a snapshot of them (null when none).
    private (ulong[] Ids, string? Snapshot) Take(IReadOnlyList<uint> entities)
    {
        var inScene = entities.Where(e => session.SceneId(e) != null).ToArray();
        return inScene.Length == 0 ? ([], null) : (inScene.Select(e => session.SceneId(e)!.Value).ToArray(), session.Snapshot(inScene));
    }

    // Records entities that did not exist before, as they are now.
    private void Record(string name, IReadOnlyList<uint> entities, string? before, bool after)
    {
        if (!Recording)
            return;
        var (ids, snapshot) = Take(entities);
        if (ids.Length > 0)
            history.Record(new ReplaceStep(name, ids, before, after ? snapshot : null));
    }

    private static T? Guard<T>(Func<T> change)
    {
        try
        {
            return change();
        }
        catch (EngineException e)
        {
            Status.Show(e.Message, error: true);
            return default;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            Status.Show(e.Message, error: true);
            return default;
        }
    }
}

// A move in progress: the entities, where they started, and the one the cursor holds.
internal sealed class Move(EngineSession session, int typeIndex, int fieldIndex, uint[] entities, PixieValue[] before, PixieValue anchor)
{
    public uint[] Entities => entities;
    public PixieValue[] Before => before;
    public int Type => typeIndex;
    public int Field => fieldIndex;
    public PixieValue Anchor => anchor;

    // Places every entity at where it started, moved by (dx, dy) world units.
    public void By(double dx, double dy)
    {
        for (var i = 0; i < entities.Length; i++)
        {
            if (session.IsAlive(entities[i]))
                session.Set(entities[i], typeIndex, fieldIndex, before[i] with { X = before[i].X + dx, Y = before[i].Y + dy });
        }
    }
}
