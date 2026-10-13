using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// PointerMode of the flocks sample.
public enum PointerMode
{
    Attract,
    Repel,
}

// The flocks sample's FlockControls, read by every Flocker.
public sealed class FlockControlsProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "FlockControls")
{
    [InspectorOrder(0), InspectorRange(1, 200)]
    public double NeighbourRadius { get => GetNumber("neighbourRadius"); set => SetNumber("neighbourRadius", value); }

    [InspectorOrder(1), InspectorRange(0, 100)]
    public double SeparationRadius { get => GetNumber("separationRadius"); set => SetNumber("separationRadius", value); }

    [InspectorOrder(2), InspectorRange(0, 5)]
    public double SeparationWeight { get => GetNumber("separationWeight"); set => SetNumber("separationWeight", value); }

    [InspectorOrder(3), InspectorRange(0, 5)]
    public double AlignmentWeight { get => GetNumber("alignmentWeight"); set => SetNumber("alignmentWeight", value); }

    [InspectorOrder(4), InspectorRange(0, 5)]
    public double CohesionWeight { get => GetNumber("cohesionWeight"); set => SetNumber("cohesionWeight", value); }

    [InspectorOrder(5), InspectorRange(0, 5)]
    public double PointerWeight { get => GetNumber("pointerWeight"); set => SetNumber("pointerWeight", value); }

    [InspectorOrder(6), InspectorRange(0, 600)]
    public double PointerRadius { get => GetNumber("pointerRadius"); set => SetNumber("pointerRadius", value); }

    [InspectorOrder(7)]
    public PointerMode PointerMode { get => GetEnum<PointerMode>("pointerMode"); set => SetEnum("pointerMode", value); }

    [InspectorOrder(8), InspectorRange(0, 600)]
    public double MinSpeed { get => GetNumber("minSpeed"); set => SetNumber("minSpeed", value); }

    [InspectorOrder(9), InspectorRange(0, 600)]
    public double MaxSpeed { get => GetNumber("maxSpeed"); set => SetNumber("maxSpeed", value); }

    [InspectorOrder(10), InspectorRange(0, 2000)]
    public double MaxForce { get => GetNumber("maxForce"); set => SetNumber("maxForce", value); }
}
