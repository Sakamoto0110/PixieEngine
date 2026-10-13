using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// The engine's Acceleration2D. A steering Behaviour rewrites it every step, so an edit lasts one step.
public sealed class Acceleration2DProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "Acceleration2D")
{
    [InspectorOrder(0)]
    public double X { get => GetX("linear"); set => SetX("linear", value); }

    [InspectorOrder(1)]
    public double Y { get => GetY("linear"); set => SetY("linear", value); }
}
