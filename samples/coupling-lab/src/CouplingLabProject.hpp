#pragma once
#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Project.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include <cstdint>
#include <map>
#include <vector>

// Drives a kinematic body back and forth: its velocity and angular velocity reverse every `period`
// seconds of simulated time. With reverseOnCrush, it also turns back as soon as a character it was
// carrying reports unsafe placement. Stopping, reversing and crushing are game policy, so they live
// here, in the sample, rather than in the engine's character/body coupling.
struct Shuttle {
    pxVec2 velocity{0, 0};
    double angularVelocity = 0;
    double period = 2;
    bool reverseOnCrush = false;
};

// The room's starting values: one factor on every shuttle's speed.
struct CouplingScenario { double speedScale = 1; };

class CouplingLabProject final : public pixie::Project {
public:
    std::string_view Name() const override { return "coupling-lab"; }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    bool SpawnAt(pixie::World& world, pxPoint at) override;
    void DrawScreen(pixie::World& world, pixie::ScreenCanvas& screen) override;

private:
    void Drive(pixie::World& world, double delta);
    void ResetRoom(pixie::World& world);
    struct Motion { double sign = 1, timer = 0; };
    std::map<entt::entity, Motion> shuttles_;              // Runtime phase of each shuttle.
    std::map<entt::entity, std::uint64_t> lastSupport_;    // Body handle each character last stood on.
    std::vector<std::pair<pixie::Entity, pixie::Transform2D>> initial_;
    std::vector<pixie::Entity> dropped_;
    pixie::Entity dragged_;
    pxPoint dragLocal_;
    int dropKind_ = 0;
};
