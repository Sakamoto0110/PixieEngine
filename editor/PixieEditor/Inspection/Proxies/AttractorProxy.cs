using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// The engine's Attractor: a point that pulls particles, falling with the square of the distance.
public sealed class AttractorProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "Attractor")
{
    [InspectorOrder(0), InspectorTooltip("World units cubed per second squared; negative pushes away")]
    public double Strength { get => GetNumber("strength"); set => SetNumber("strength", value); }

    [InspectorOrder(1), InspectorRange(0, 100)]
    public double Softening { get => GetNumber("softening"); set => SetNumber("softening", value); }

    [InspectorOrder(2), InspectorTooltip("0 means no limit")]
    public double Radius { get => GetNumber("radius"); set => SetNumber("radius", value); }
}
