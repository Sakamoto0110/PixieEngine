using PixieEditor.Native;

namespace PixieEditor.Ui;

// The names behind a layer mask field: the scene component that names its slots, how many slots
// its checkboxes show, and the word for them. A field's PIXIE_TEXT_REFERENCES chooses it.
internal sealed record LayerScheme(string NamesType, int Slots, string Noun)
{
    public static readonly LayerScheme Collision = new("CollisionLayers2D", 31, "Collision");
    public static readonly LayerScheme Light = new("LightLayers2D", 8, "Light");

    public static LayerScheme Of(EngineSession session, int type, int field) =>
        session.FieldText(type, field, PixieFieldText.References) == Light.NamesType ? Light : Collision;

    public static LayerScheme OfNamesType(EngineSession session, int namesType) =>
        session.TypeIndex(Light.NamesType) == namesType ? Light : Collision;

    public string Name(EngineSession session, int slot) =>
        this == Light ? session.LightLayerName(slot) : session.CollisionLayerName(slot);
}
