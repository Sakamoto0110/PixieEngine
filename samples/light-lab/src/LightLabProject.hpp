#pragma once
#include "pixie/runtime/Project.hpp"
#include <pixie/math/pxVec.hpp>
#include <entt/entity/entity.hpp>
#include <map>

// A door that slides back and forth: a kinematic body whose velocity reverses every `period` seconds
// of simulated time. Opening and closing a door is game policy, so it lives here, in the sample, and
// the door is an occluder like any other: lighting room 2's moving door.
struct Door2D {
    pxVec2 velocity{0, -60};
    double period = 2;
};

class LightLabProject final : public pixie::Project {
public:
    std::string_view Name() const override { return "light-lab"; }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;

private:
    struct Motion { double sign = 1, timer = 0; };
    std::map<entt::entity, Motion> doors_;
};
