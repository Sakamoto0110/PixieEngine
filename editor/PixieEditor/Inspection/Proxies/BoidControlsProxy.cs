using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// SteerMode of the boids sample.
public enum SteerMode
{
    Attract,
    Repel,
}

// The boids sample's BoidControls, read by every Boid.
public sealed class BoidControlsProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "BoidControls")
{
    [InspectorOrder(0)]
    public SteerMode Mode { get => GetEnum<SteerMode>("mode"); set => SetEnum("mode", value); }

    [InspectorOrder(1), InspectorRange(0, 600)]
    public double Radius { get => GetNumber("radius"); set => SetNumber("radius", value); }

    [InspectorOrder(2), InspectorRange(0, 800)]
    public double MaxSpeed { get => GetNumber("maxSpeed"); set => SetNumber("maxSpeed", value); }

    [InspectorOrder(3), InspectorRange(0, 800)]
    public double CruiseSpeed { get => GetNumber("cruiseSpeed"); set => SetNumber("cruiseSpeed", value); }

    [InspectorOrder(4), InspectorRange(0, 3000)]
    public double MaxForce { get => GetNumber("maxForce"); set => SetNumber("maxForce", value); }

    [InspectorOrder(5), InspectorRange(0, 10)]
    public double Wander { get => GetNumber("wander"); set => SetNumber("wander", value); }
}
