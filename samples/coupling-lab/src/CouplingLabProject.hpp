#pragma once
#include "pixie/runtime/Project.hpp"

// Drives a kinematic body back and forth: its velocity and angular velocity reverse every `period`
// seconds of simulated time. Stopping, reversing and crushing are game policy, so they live here,
// in the sample, rather than in the engine's character/body coupling.
struct Shuttle {
    pxVec2 velocity{0, 0};
    double angularVelocity = 0;
    double period = 2;
};

// The room's starting values: one factor on every shuttle's speed.
struct CouplingScenario { double speedScale = 1; };

class CouplingLabProject final : public pixie::Project {
public:
    std::string_view Name() const override { return "coupling-lab"; }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    bool SpawnAt(pixie::World& world, pxPoint at) override;
    void DrawScreen(pixie::World& world, pixie::ScreenCanvas& screen) override;
};
