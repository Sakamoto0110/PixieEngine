#define BOUNDARY_ENGINE_BUILD
#include "Gate.hpp"
#include "pixie/runtime/SceneProject.hpp"

namespace { boundary_fixture::Factory factory = nullptr; }
std::unique_ptr<pixie::Project> pixie::CreateProject() {
    return factory ? factory() : std::make_unique<SceneProject>();
}
void boundary_factory(boundary_fixture::Factory value) { factory = value; }
pixie::World* boundary_world_create() { return new pixie::World(42); }
void boundary_world_destroy(pixie::World* world) { delete world; }
unsigned boundary_hash() { return entt::type_hash<boundary_fixture::Component>::value(); }
unsigned boundary_index() { return entt::type_index<boundary_fixture::Component>::value(); }
bool boundary_world_check(pixie::World* world) {
    using boundary_fixture::Component;
    double total = 0;
    world->Each<Component>([&](pixie::Entity, Component& value) { total += value.value; });
    if (total != 42) return false;
    const auto entity = world->Create("dll-added");
    entity.Add<Component>(84.0);
    total = 0;
    world->Each<Component>([&](pixie::Entity, Component& value) { total += value.value; });
    if (total != 126) return false;
    entity.Remove<Component>();
    return !entity.Has<Component>();
}
