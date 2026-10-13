using PixieEditor.Inspection.Proxies;
using PixieEditor.Native;
using PixieInspector;

namespace PixieEditor.Inspection;

// Which component types have a hand-written proxy, and how to inspect one: a typed PixieInspector,
// which discovers the proxy's properties, bound to a proxy of the entity's component. Until proxies
// are generated from the engine's field descriptors, a type without an entry here is listed but not
// inspected.
internal static class ProxyCatalog
{
    private static readonly Dictionary<string, Func<EngineSession, uint, Inspector>> Inspectors = new()
    {
        ["Transform2D"] = Entry((s, e) => new Transform2DProxy(s, e)),
        ["Velocity2D"] = Entry((s, e) => new Velocity2DProxy(s, e)),
        ["Acceleration2D"] = Entry((s, e) => new Acceleration2DProxy(s, e)),
        ["Shape"] = Entry((s, e) => new ShapeProxy(s, e)),
        ["Attractor"] = Entry((s, e) => new AttractorProxy(s, e)),
        ["ParticleEmitter"] = Entry((s, e) => new ParticleEmitterProxy(s, e)),
        ["BoidControls"] = Entry((s, e) => new BoidControlsProxy(s, e)),
        ["FlockControls"] = Entry((s, e) => new FlockControlsProxy(s, e)),
        ["ParticleControls"] = Entry((s, e) => new ParticleControlsProxy(s, e)),
    };

    public static bool Has(string typeName) => Inspectors.ContainsKey(typeName);

    // An inspector of the entity's component, bound to a new proxy of it.
    public static Inspector Inspect(string typeName, EngineSession session, uint entity) => Inspectors[typeName](session, entity);

    private static Func<EngineSession, uint, Inspector> Entry<T>(Func<EngineSession, uint, T> create) where T : ComponentProxy
    {
        return (session, entity) =>
        {
            var inspector = Inspector.Create<T>();
            inspector.Bind(create(session, entity));
            return inspector;
        };
    }
}
