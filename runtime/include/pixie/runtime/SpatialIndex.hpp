#pragma once

#include "pixie/runtime/SpatialGrid.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/World.hpp"

#include <string>

namespace pixie {

// Keeps `owner`'s SpatialGrid component filled with every entity that has a Transform2D and a
// Member component: a PreUpdate system rebuilds it at the start of each step, so every Behaviour
// of the step queries the same snapshot, whatever order they run in. The owner gets a SpatialGrid
// with `cellSize` if it has none.
template<typename Member>
void AddSpatialIndex(World& world, Entity owner, double cellSize, std::string name = "spatial.index") {
    if (!owner.Has<SpatialGrid>()) {
        owner.Add<SpatialGrid>(cellSize);
    }
    world.AddSystem(SystemPhase::PreUpdate, std::move(name),
                    [owner](World& w, const FrameTime&, const Input&) {
                        SpatialGrid* grid = owner.TryGet<SpatialGrid>();
                        if (grid == nullptr) {
                            return;
                        }
                        grid->Clear();
                        auto members = w.Registry().view<const Transform2D, const Member>();
                        for (const entt::entity id : members) {
                            grid->Add(id, members.template get<const Transform2D>(id).position);
                        }
                        grid->Build();
                    });
}

} // namespace pixie
