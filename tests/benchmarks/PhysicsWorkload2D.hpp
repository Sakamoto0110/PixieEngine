#pragma once
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include <string_view>

namespace pixie::benchmark {
// Test/qualification fixture data, not a runtime API. All sizes/counts describe authored setup.
struct PhysicsWorkload2D {
    std::vector<entt::entity> dynamicBodies;
    std::size_t expectedBodies = 0, expectedShapes = 0, expectedSensors = 0;
    double gravityY = 0;
    const char* driver = "none";
};
bool IsPhysicsWorkload2D(std::string_view name);
// Installs synchronization before any optional command driver. Returned entity IDs belong to world.
PhysicsWorkload2D BuildPhysicsWorkload2D(World& world, int count, std::string_view name, bool awake);
}
