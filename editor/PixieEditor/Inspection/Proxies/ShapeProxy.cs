using PixieEditor.Native;
using PixieInspector.Attributes;
using PixieLib;

namespace PixieEditor.Inspection.Proxies;

// The shapes of the engine's ShapeRenderer, in the order of pixie::ShapeKind.
public enum ShapeKind
{
    Triangle,
    Circle,
    Ring,
    Quad,
}

// The engine's Shape: how an entity looks.
public sealed class ShapeProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "Shape")
{
    [InspectorOrder(0)]
    public ShapeKind Kind { get => GetEnum<ShapeKind>("kind"); set => SetEnum("kind", value); }

    [InspectorOrder(1), InspectorTooltip("Half width, in world units, before the transform's scale")]
    public double Width { get => GetX("size"); set => SetX("size", value); }

    [InspectorOrder(2), InspectorTooltip("Half height, in world units, before the transform's scale")]
    public double Height { get => GetY("size"); set => SetY("size", value); }

    [InspectorOrder(3)]
    public PxColorRgba Color { get => GetColor("color"); set => SetColor("color", value); }
}
