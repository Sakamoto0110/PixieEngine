using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// The particles sample's ParticleControls: the particle-to-particle interaction's values.
public sealed class ParticleControlsProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "ParticleControls")
{
    [InspectorOrder(0)]
    public bool Interaction { get => GetBool("interaction"); set => SetBool("interaction", value); }

    [InspectorOrder(1), InspectorRange(0.5, 20)]
    public double InteractionRadius { get => GetNumber("interactionRadius"); set => SetNumber("interactionRadius", value); }

    [InspectorOrder(2), InspectorRange(0, 10000)]
    public double Stiffness { get => GetNumber("stiffness"); set => SetNumber("stiffness", value); }

    [InspectorOrder(3), InspectorRange(0, 20)]
    public double Damping { get => GetNumber("damping"); set => SetNumber("damping", value); }
}
