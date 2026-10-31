#pragma once
#include "pixie/core/Telemetry.hpp"
#include "pixie/runtime/Project.hpp"

// The machine's power, the room's scenario on the scene's "scenario" entity: the logical state,
// which switches at once, and how long the lamps take to follow it. P switches it.
struct PowerSwitch2D {
    bool powered = true;
    double fadeSeconds = 0.6;
};

// A lamp whose Emission2D intensity follows the power: `onIntensity` when powered, `offIntensity`
// when not (a fault lamp is the other way round), fading over the switch's fadeSeconds.
struct PoweredLamp2D {
    double onIntensity = 1.6;
    double offIntensity = 0.0;
};

// A ParticleSpawner that runs at `onRate` while powered and stops while not.
struct PoweredSpawner2D {
    double onRate = 220;
};

// Lighting rooms 3 and 4: the machine with a power switch whose logical state and lamp transition are
// shown together on the panel and recorded in telemetry (machine.powered, machine.lamp_intensity);
// F turns the optional effects (ambient occlusion, fog, bloom) on and off, which with the panel open is
// room 4. A World built without the scene (as the C API test builds one) gets a small room made in
// code: the scenario, a lamp and a light.
class LightMachineProject final : public pixie::Project {
public:
    std::string_view Name() const override { return "light-machine"; }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    void DrawScreen(pixie::World& world, pixie::ScreenCanvas& screen) override;
    // Drops a box that casts shadows, outside the scene, so it is never saved.
    bool SpawnAt(pixie::World& world, pxPoint at) override;

private:
    void Follow(pixie::World& world, double delta);
    pixie::MetricHandle powered_;
    pixie::MetricHandle lampIntensity_;
    bool fading_ = false;
};
