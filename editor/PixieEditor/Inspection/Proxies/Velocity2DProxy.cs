using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// The engine's Velocity2D, in world units and radians per second.
public sealed class Velocity2DProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "Velocity2D")
{
    [InspectorOrder(0)]
    public double X { get => GetX("linear"); set => SetX("linear", value); }

    [InspectorOrder(1)]
    public double Y { get => GetY("linear"); set => SetY("linear", value); }

    [InspectorOrder(2)]
    public double Angular { get => GetNumber("angular"); set => SetNumber("angular", value); }
}
