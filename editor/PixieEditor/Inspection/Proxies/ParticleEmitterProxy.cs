using PixieEditor.Native;
using PixieInspector.Attributes;

namespace PixieEditor.Inspection.Proxies;

// The emission values of the engine's ParticleEmitter.
public sealed class ParticleEmitterProxy(EngineSession session, uint entity) : ComponentProxy(session, entity, "ParticleEmitter")
{
    [InspectorOrder(0), InspectorTooltip("Particles per second at the entity; 0 emits nothing")]
    public double Rate { get => GetNumber("rate"); set => SetNumber("rate", value); }

    [InspectorOrder(1), InspectorTooltip("Seconds; 0 means forever")]
    public double Lifetime { get => GetNumber("lifetime"); set => SetNumber("lifetime", value); }

    [InspectorOrder(2)]
    public double SpeedMin { get => GetNumber("speedMin"); set => SetNumber("speedMin", value); }

    [InspectorOrder(3)]
    public double SpeedMax { get => GetNumber("speedMax"); set => SetNumber("speedMax", value); }

    [InspectorOrder(4), InspectorTooltip("Radians around the entity's rotation")]
    public double Spread { get => GetNumber("spread"); set => SetNumber("spread", value); }
}
