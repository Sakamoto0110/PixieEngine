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
// it lets go of the keyboard, and, for a field that names an image, a "..." button that picks a PNG
// (copying one from outside the project into it) and a preview of the image, or why it cannot be read.
internal sealed class InspectorPanel(EngineSession session, Authoring authoring, Editing.EditRecorder recorder, ImageChooser images) : IDisposable
{
    private static readonly Vector4 ProblemColor = new(1.0f, 0.45f, 0.4f, 1.0f);
    private const float PreviewSize = 96f;

    // Set by a scenario's button; the editor rebuilds the scene at the start of the next frame, not
    // while an inspector is drawing.
    public bool RebuildRequested { get; set; }

    private sealed record TextField(int Field, string Name, string Label, string Tooltip, string Asset);

    private sealed record Section(int Type, string Name, Inspector Inspector, ImGuiInspectorView View, TextField[] Texts);

    private readonly List<Section> Sections = [];
    private uint[] Entities = [];
    private int[] Types = [];
    // The name being typed in the name field, while it has the keyboard.
    private string NameText = "";
    private bool NameActive;
    // A text field's text while it has the keyboard.
    private readonly Dictionary<(int Type, int Field), string> Typing = [];

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
                foreach (var section in Sections)
                {
                    ImGui.PushID(section.Type);
                    // The header's close button removes the component from every selected entity.
                    var keep = true;
                    if (ImGui.CollapsingHeader(section.Name, ref keep, ImGuiTreeNodeFlags.DefaultOpen))
                    {
                        foreach (var text in section.Texts)
                            DrawText(section.Type, text);
                        section.View.Draw();
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
            if (Types.Contains(type))
                continue;
            if (ImGui.Selectable(session.TypeName(type)))
            {
                var chosen = type;
                authoring.Change(Entities, $"addition of {session.TypeName(chosen)}", id => session.AddComponent(id, chosen));
            }
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
        var image = field.Asset == "png";
        ImGui.SetNextItemWidth(image ? -40 : -1);
        ImGui.InputText("##text", ref text, 1024);
        if (ImGui.IsItemActive())
            Typing[key] = text;
        else
            Typing.Remove(key);
        if (ImGui.IsItemDeactivatedAfterEdit())
            authoring.SetText(Entities, type, field.Field, text.Trim());
        if (field.Tooltip.Length > 0 && ImGui.IsItemHovered())
            ImGui.SetTooltip(field.Tooltip);

        if (image)
        {
            ImGui.SameLine();
            if (ImGui.Button("...", new Vector2(-1, 0)))
            {
                var chosen = Entities;
                images.Open(file =>
                {
                    if (authoring.ImportImage(file) is { } path)
                        authoring.SetText(chosen, type, field.Field, path);
                });
            }

            if (ImGui.IsItemHovered())
                ImGui.SetTooltip("Choose a PNG; one outside the project is copied into its images folder.");
            if (!mixed)
                DrawPreview(values[0]);
        }

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
    public bool Set(string type, string property, string text)
    {
        var section = Sections.FirstOrDefault(s => s.Name == type);
        if (section == null)
            return false;

        if (section.Texts.FirstOrDefault(t => t.Name == property) is { } textField)
        {
            authoring.SetText(Entities, section.Type, textField.Field, text);
            return true;
        }

        var node = section.Inspector[property];
        if (node is ButtonNode button)
        {
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
        node.SetValue(value);
        return true;
    }

    private static PxColorRgba ParseColor(string text)
    {
        var parts = text.Split(',').Select(p => byte.Parse(p.Trim(), CultureInfo.InvariantCulture)).ToArray();
        return new PxColorRgba(parts[0], parts[1], parts[2], parts.Length > 3 ? parts[3] : (byte)255);
    }

    private void Rebuild(uint[] entities, int[] types)
    {
        Release();
        Typing.Clear();
        Entities = entities;
        Types = types;
        foreach (var type in types)
        {
            var handles = entities.Select(id => new ComponentHandle(session, id, type, recorder)).ToArray();
            var inspector = InspectorFactory.Build(session, type, handles, () => RebuildRequested = true);
            var texts = Enumerable.Range(0, session.FieldCount(type))
                .Where(f => session.FieldInfo(type, f).Kind == PixieFieldKind.Text)
                .Select(f => new TextField(f, session.FieldName(type, f), session.FieldText(type, f, PixieFieldText.Label),
                    session.FieldText(type, f, PixieFieldText.Tooltip), session.FieldText(type, f, PixieFieldText.Asset)))
                .ToArray();
            Sections.Add(new Section(type, session.TypeName(type), inspector, inspector.CreateImGuiView(), texts));
        }
    }

    private void Release()
    {
        foreach (var section in Sections)
        {
            section.View.Dispose();
            section.Inspector.Dispose();
        }

        Sections.Clear();
    }

    public void Dispose() => Release();
}
