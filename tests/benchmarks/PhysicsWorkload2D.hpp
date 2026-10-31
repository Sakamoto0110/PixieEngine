#pragma once
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include <string_view>

namespace pixie::benchmark {
// Test/qualification fixture data, not a runtime API. All sizes/counts describe authored setup.
struct PhysicsWorkload2D {
    std::vector<entt::entity> dynamicBodies;
    std::vector<entt::entity> motors; // Scene characters, in creation order; only "coupled" has them.
    std::size_t expectedBodies = 0, expectedShapes = 0, expectedSensors = 0, expectedMotors = 0;
    double gravityY = 0;
    const char* driver = "none";
};
bool IsPhysicsWorkload2D(std::string_view name);
// Installs synchronization before any optional command driver. Returned entity IDs belong to world.
PhysicsWorkload2D BuildPhysicsWorkload2D(World& world, int count, std::string_view name, bool awake);
// Scene motors report blocked and unsafe placement through the scene diagnostics. True when every
// current diagnostic is one such status of a motor that reports it, so none is a configuration error.
bool OnlyCouplingStatus2D(World& world, const PhysicsWorkload2D& setup);
}
