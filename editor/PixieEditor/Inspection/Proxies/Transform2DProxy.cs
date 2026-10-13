using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// The engine's Transform2D: where an entity is (Y down), its rotation in radians, and its scale.
public sealed class Transform2DProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "Transform2D")
{
    [InspectorOrder(0)]
    public double X { get => GetX("position"); set => SetX("position", value); }

    [InspectorOrder(1)]
    public double Y { get => GetY("position"); set => SetY("position", value); }

    [InspectorOrder(2), InspectorTooltip("Radians, from +X toward +Y (clockwise on screen)")]
    public double Rotation { get => GetNumber("rotation"); set => SetNumber("rotation", value); }

    [InspectorOrder(3)]
    public double ScaleX { get => GetX("scale"); set => SetX("scale", value); }

    [InspectorOrder(4)]
    public double ScaleY { get => GetY("scale"); set => SetY("scale", value); }
}
