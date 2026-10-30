using System.Globalization;
using System.Numerics;
using Hexa.NET.ImGui;
using PixieEditor.Inspection;
using PixieEditor.Native;
using PixieInspector;
using PixieInspector.ImGui;
using PixieLib;

namespace PixieEditor.Ui;

// The selected entities' components: one PixieInspector per component type that every selected
// entity has, built from the engine's field descriptors, bound to the first entity and with AddBind to
// the rest, and drawn by its Dear ImGui view under a header with the component's name. An edit
// changes every selected entity; values that differ show as mixed. The inspectors are made again when
// the selection or the set of shared components changes.
//
// Text fields are drawn here, above the component's inspector: a text box whose edit is one step when
// it lets go of the keyboard, and, for a field that names a file (an asset), a "..." button that picks
// one of its kind (copying one from outside the project into it); under an image, a preview of it, or
// why it cannot be read; under a font, whether its file is missing.
// So are lists of points: a row of X and Y per point, each edit one step when it lets go, with buttons
// to remove a point and to add one.
internal sealed partial class InspectorPanel(EngineSession session, Authoring authoring, Editing.EditRecorder recorder, AssetChooser files) : IDisposable
{
    private static readonly Vector4 ProblemColor = new(1.0f, 0.45f, 0.4f, 1.0f);
    private const float PreviewSize = 96f;

    // Set by a scenario's button; the editor rebuilds the scene at the start of the next frame, not
    // while an inspector is drawing.
    public bool RebuildRequested { get; set; }

    private sealed record TextField(int Field, string Name, string Label, string Tooltip, string Asset, PixieFieldKind Kind);

    private sealed record Section(int Type, string Name, Inspector Inspector, ImGuiInspectorView View, TextField[] Texts);

    private readonly List<Section> Sections = [];
    private uint[] Entities = [];
    private int[] Types = [];
    // The name being typed in the name field, while it has the keyboard.
    private string NameText = "";
    private bool NameActive;
    // A text field's text while it has the keyboard.
    private readonly Dictionary<(int Type, int Field), string> Typing = [];
    // A point being typed, while its row has the keyboard.
    private readonly Dictionary<(int Type, int Field, int Index), Vector2> PointTyping = [];

    public void Draw(IReadOnlyList<uint> selected)
    {
        var types = selected.Count == 0
            ? []
            : Enumerable.Range(0, session.TypeCount).Where(t => selected.All(id => session.Has(id, t))).ToArray();
        if (!selected.SequenceEqual(Entities) || !types.SequenceEqual(Types))
            Rebuild([.. selected], types);

        if (ImGui.Begin("Inspector"))
        {
            if (Entities.Length == 0)
            {
                ImGui.TextDisabled("Choose an entity.");
            }
            else
            {
                if (Entities.Length == 1)
                    DrawName(Entities[0]);
                else
                    ImGui.TextUnformatted($"{Entities.Length} entities: {string.Join(", ", Entities.Take(4).Select(Describe))}{(Entities.Length > 4 ? ", ..." : "")}");
                if (Entities.Length > 1 && Types.Length == 0)
                    ImGui.TextDisabled("They share no component.");
                ImGui.Separator();
                int? remove = null;
                var makeCompound = false;
                foreach (var section in Sections)
                {
                    ImGui.PushID(section.Type);
                    // The header's close button removes the component from every selected entity.
                    var keep = true;
                    if (ImGui.CollapsingHeader(section.Name, ref keep, ImGuiTreeNodeFlags.DefaultOpen))
                    {
                        foreach (var text in section.Texts)
                        {
                            if (text.Kind == PixieFieldKind.RecordList)
                            {
                                if (session.FieldInfo(section.Type, text.Field).Editor == PixieEditorKind.LayerNames)
                                    DrawLayerNames(section.Type, text.Field);
                                else DrawRecords(section.Type, text);
                            }
                            else if (text.Kind == PixieFieldKind.PointList)
                                DrawPoints(section.Type, text);
                            else
                                DrawText(section.Type, text);
                        }
                        DrawLayerMasks(section.Type, Entities.Select(id => new ComponentHandle(session, id, section.Type, recorder)).ToArray());
                        section.View.Draw();
                        if (section.Name == "RigidBody2D")
                            DrawBodyMass();
                        if (section.Name == "Collider2D" && session.TypeIndex("CompoundCollider2D") >= 0 && ImGui.Button("Make compound"))
                            makeCompound = true;
                    }
                    if (!keep)
                        remove = section.Type;
                    ImGui.PopID();
                }

                ImGui.Spacing();
                DrawAddComponent();
                // After drawing, so no inspector of the removed component is in use; the sections are
                // made again on the next frame.
                if (remove is { } type)
                    authoring.Change(Entities, $"removal of {session.TypeName(type)}", id => session.RemoveComponent(id, type));
                else if (makeCompound)
                {
                    recorder.Close();
                    authoring.MakeCompound(Entities);
                }
            }
        }

        ImGui.End();
    }

    // A button with the component types not every selected entity has; choosing one adds it to all.
    private void DrawAddComponent()
    {
        if (ImGui.Button("Add component"))
            ImGui.OpenPopup("add-component");
        if (!ImGui.BeginPopup("add-component"))
            return;

        for (var type = 0; type < session.TypeCount; type++)
        {
            if (Types.Contains(type) || session.IsValueType(type))
                continue;
            var reason = authoring.AdditionError(Entities, type);
            ImGui.BeginDisabled(reason != null);
            if (ImGui.Selectable(session.TypeName(type)))
            {
                var chosen = type;
                authoring.AddComponent(Entities, chosen);
            }
            ImGui.EndDisabled();
            if (reason != null && ImGui.IsItemHovered(ImGuiHoveredFlags.AllowWhenDisabled))
                ImGui.SetTooltip(reason);
        }

        ImGui.EndPopup();
    }

    // The entity's name, which can be typed over; the rename happens when the field lets go of the
    // keyboard, as one step.
    private void DrawName(uint entity)
    {
        if (!NameActive)
            NameText = session.EntityName(entity);
        ImGui.SetNextItemWidth(-60);
        ImGui.InputText("##name", ref NameText, 128);
        NameActive = ImGui.IsItemActive();
        if (ImGui.IsItemDeactivatedAfterEdit())
            authoring.Rename(entity, NameText.Trim());
        if (ImGui.IsItemHovered())
            ImGui.SetTooltip("The entity's name; type a new one and press Enter.");
        ImGui.SameLine();
        ImGui.TextDisabled($"#{entity & 0xFFFFF}");
    }

    // A text field of the selected entities: their common text, or empty with "mixed" when they differ.
    private void DrawText(int type, TextField field)
    {
        ImGui.PushID(field.Field);
        var values = Entities.Select(e => session.GetText(e, type, field.Field)).Distinct().ToArray();
        var mixed = values.Length > 1;
        var key = (type, field.Field);
        if (!Typing.TryGetValue(key, out var text))
            text = mixed ? "" : values[0];

        ImGui.TextUnformatted(mixed ? $"{field.Label} (mixed)" : field.Label);
        if (field.Tooltip.Length > 0 && ImGui.IsItemHovered())
            ImGui.SetTooltip(field.Tooltip);
        var asset = field.Asset.Length > 0;
        ImGui.SetNextItemWidth(asset ? -40 : -1);
        ImGui.InputText("##text", ref text, 1024);
        if (ImGui.IsItemActive())
            Typing[key] = text;
        else
            Typing.Remove(key);
        if (ImGui.IsItemDeactivatedAfterEdit())
            authoring.SetText(Entities, type, field.Field, text.Trim());
        if (field.Tooltip.Length > 0 && ImGui.IsItemHovered())
            ImGui.SetTooltip(field.Tooltip);

        if (asset)
        {
            ImGui.SameLine();
            if (ImGui.Button("...", new Vector2(-1, 0)))
            {
                var chosen = Entities;
                var kind = field.Asset;
                files.Open(kind, file =>
                {
                    if (authoring.ImportAsset(file, kind) is { } path)
                        authoring.SetText(chosen, type, field.Field, path);
                });
            }

            if (ImGui.IsItemHovered())
                ImGui.SetTooltip($"Choose a .{field.Asset} file; one outside the project is copied into its " +
                                 $"{Editing.ProjectAssets.FolderFor(field.Asset)} folder.");
            if (!mixed && field.Asset == "png")
                DrawPreview(values[0]);
            else if (!mixed && values[0].Length == 0)
                ImGui.TextDisabled($"No file: choose a .{field.Asset} one.");
            else if (!mixed && !Editing.ProjectAssets.Exists(session.ProjectFolder, values[0]))
                ImGui.TextColored(ProblemColor, $"{values[0]}: the file does not exist");
        }

        ImGui.Spacing();
        ImGui.PopID();
    }

    // A list of points of the selected entities: a row per point, its X and Y typed over, with a
    // button that removes it, and a button that adds one after the last.
    private void DrawPoints(int type, TextField field)
    {
        ImGui.PushID(field.Field);
        var lists = Entities.Select(e => session.GetPoints(e, type, field.Field)).ToArray();
        var mixed = lists.Skip(1).Any(l => !l.SequenceEqual(lists[0]));
        ImGui.TextUnformatted(mixed ? $"{field.Label} (mixed)" : $"{field.Label} ({lists[0].Length})");
        if (field.Tooltip.Length > 0 && ImGui.IsItemHovered())
            ImGui.SetTooltip(field.Tooltip);
        if (mixed)
        {
            ImGui.TextDisabled("The selected entities differ; select one to edit its points.");
            ImGui.PopID();
            return;
        }

        var points = lists[0];
        int? remove = null;
        for (var i = 0; i < points.Length; i++)
        {
            ImGui.PushID(i);
            var key = (type, field.Field, i);
            if (!PointTyping.TryGetValue(key, out var value))
                value = new Vector2((float)points[i].X, (float)points[i].Y);
            ImGui.SetNextItemWidth(-40);
            ImGui.InputFloat2("##point", ref value);
            if (ImGui.IsItemActive())
                PointTyping[key] = value;
            else
                PointTyping.Remove(key);
            if (ImGui.IsItemDeactivatedAfterEdit())
            {
                var changed = points.ToArray();
                changed[i] = (value.X, value.Y);
                authoring.SetPoints(Entities, type, field.Field, changed);
            }

            ImGui.SameLine();
            if (ImGui.Button("-", new Vector2(-1, 0)))
                remove = i;
            if (ImGui.IsItemHovered())
                ImGui.SetTooltip("Remove this point.");
            ImGui.PopID();
        }

        if (ImGui.Button("Add point"))
            authoring.AddPoint(Entities, type, field.Field);
        if (remove is { } index)
            authoring.RemovePoint(Entities, type, field.Field, index);
        ImGui.Spacing();
        ImGui.PopID();
    }

    // The image a sprite names, fitted in a square, with its size; or why it cannot be read.
    private unsafe void DrawPreview(string path)
    {
        if (path.Length == 0)
        {
            ImGui.TextDisabled("No image: choose one, or drop a PNG on the viewport.");
            return;
        }

        if (session.Image(path, out var problem) is not { } found)
        {
            ImGui.PushTextWrapPos(0f);
            ImGui.TextColored(ProblemColor, problem ?? path);
            ImGui.PopTextWrapPos();
            return;
        }

        var scale = PreviewSize / Math.Max(found.Width, found.Height);
        var size = new Vector2(found.Width * scale, found.Height * scale);
        var origin = ImGui.GetCursorScreenPos();
        ImGui.GetWindowDrawList().AddRectFilled(origin, origin + size, ImGui.GetColorU32(ImGuiCol.FrameBg));
        // The texture's first row is the image's top, as ImGui draws it.
        ImGui.Image(new ImTextureRef(null, new ImTextureID(found.Texture)), size);
        ImGui.SameLine();
        ImGui.TextDisabled($"{found.Width} x {found.Height} px");
    }

    private string Describe(uint entity)
    {
        var name = session.EntityName(entity);
        return name.Length > 0 ? $"{name} (#{entity & 0xFFFFF})" : $"#{entity & 0xFFFFF}";
    }

    // Writes field `property` of the selected entities' component `type` through its inspector, as the
    // view does when a value is edited (a point's or vector's axis is the field's name with X or Y).
    // Returns false when there is no such component or field.
    public bool Set(string type, string property, string text
#if PIXIE_DEVELOPMENT
        , Action? onWrite = null
#endif
    )
    {
        var section = Sections.FirstOrDefault(s => s.Name == type);
        if (section == null)
            return false;

        var maskField = Enumerable.Range(0, session.FieldCount(section.Type)).FirstOrDefault(f => session.FieldName(section.Type, f) == property, -1);
        if (maskField >= 0 && session.FieldInfo(section.Type, maskField).Editor == PixieEditorKind.LayerMask)
        {
            var mask = int.Parse(text, CultureInfo.InvariantCulture);
#if PIXIE_DEVELOPMENT
            onWrite?.Invoke();
#endif
            foreach (var entity in Entities)
                new ComponentHandle(session, entity, section.Type, recorder).Set(maskField,
                    new PixieValue { X = mask });
            return true;
        }

        if (section.Texts.FirstOrDefault(t => t.Name == property) is { } textField)
        {
            if (textField.Kind == PixieFieldKind.PointList)
            {
                var points = ParsePoints(text);
#if PIXIE_DEVELOPMENT
                onWrite?.Invoke();
#endif
                authoring.SetPoints(Entities, section.Type, textField.Field, points);
            }
            else
            {
#if PIXIE_DEVELOPMENT
                onWrite?.Invoke();
#endif
                authoring.SetText(Entities, section.Type, textField.Field, text);
            }
            return true;
        }

        var node = section.Inspector[property];
        if (node is ButtonNode button)
        {
#if PIXIE_DEVELOPMENT
            onWrite?.Invoke();
#endif
            button.Press();
            return true;
        }

        object value = node.ValueType switch
        {
            var t when t == typeof(bool) => bool.Parse(text),
            var t when t == typeof(int) => int.Parse(text, CultureInfo.InvariantCulture),
            var t when t == typeof(double) => double.Parse(text, CultureInfo.InvariantCulture),
            var t when t == typeof(PxColorRgba) => ParseColor(text),
            _ => text,
        };
#if PIXIE_DEVELOPMENT
        onWrite?.Invoke();
#endif
        node.SetValue(value);
        return true;
    }

    // "x,y;x,y;...", as --set writes a list of points.
    private static (double X, double Y)[] ParsePoints(string text) =>
        text.Split(';', StringSplitOptions.RemoveEmptyEntries)
            .Select(p => p.Split(','))
            .Select(p => (double.Parse(p[0], CultureInfo.InvariantCulture), double.Parse(p[1], CultureInfo.InvariantCulture)))
            .ToArray();

    private static PxColorRgba ParseColor(string text)
    {
        var parts = text.Split(',').Select(p => byte.Parse(p.Trim(), CultureInfo.InvariantCulture)).ToArray();
        return new PxColorRgba(parts[0], parts[1], parts[2], parts.Length > 3 ? parts[3] : (byte)255);
    }

    private void Rebuild(uint[] entities, int[] types)
    {
        Release();
        Typing.Clear();
        PointTyping.Clear();
        LayerTyping.Clear();
        Entities = entities;
        Types = types;
        foreach (var type in types)
        {
            var handles = entities.Select(id => new ComponentHandle(session, id, type, recorder)).ToArray();
            var inspector = InspectorFactory.Build(session, type, handles, () => RebuildRequested = true);
            var texts = Enumerable.Range(0, session.FieldCount(type))
                .Select(f => (Field: f, Kind: session.FieldInfo(type, f).Kind))
                .Where(f => f.Kind is PixieFieldKind.Text or PixieFieldKind.PointList or PixieFieldKind.RecordList)
                .Select(f => new TextField(f.Field, session.FieldName(type, f.Field), session.FieldText(type, f.Field, PixieFieldText.Label),
                    session.FieldText(type, f.Field, PixieFieldText.Tooltip), session.FieldText(type, f.Field, PixieFieldText.Asset), f.Kind))
                .ToArray();
            Sections.Add(new Section(type, session.TypeName(type), inspector, inspector.CreateImGuiView(), texts));
        }
    }

    private void Release()
    {
        ReleaseRecords();
        foreach (var section in Sections)
        {
            section.View.Dispose();
            section.Inspector.Dispose();
        }

        Sections.Clear();
    }

    public void Dispose() => Release();
}
