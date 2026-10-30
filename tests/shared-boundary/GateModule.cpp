#include "Gate.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include <cstdio>

namespace {
// This type is private to the module; only its descriptors cross into the engine C API.
struct PrivateComponent { double value = 123; };
#ifdef BOUNDARY_BREAK_IDENTITY
struct WrongComponent { double value = 0; };
using SharedComponent = WrongComponent;
#else
using SharedComponent = boundary_fixture::Component;
#endif
class GateProject final : public pixie::Project {
public:
    std::string_view Name() const override { return "boundary-gate"; }
    void Describe(pixie::ComponentTypes& types) const override {
        types.Add<PrivateComponent>("ModulePrivate").Field<&PrivateComponent::value>("value");
    }
    void Build(pixie::World& world, pixie::Camera2D&) override {
        world.Create("private").Add<PrivateComponent>();
    }
};
std::unique_ptr<pixie::Project> Create() { return std::make_unique<GateProject>(); }
}
extern "C" __declspec(dllexport) bool module_check() {
    // Prime a different sequence locally: hash identity must not depend on sequence order.
    (void)entt::type_index<PrivateComponent>::value();
    const auto hash = entt::type_hash<SharedComponent>::value();
    const auto index = entt::type_index<SharedComponent>::value();
    std::printf("module hash=%u index=%u; engine hash=%u index=%u\n", hash, index,
        boundary_hash(), boundary_index());
    if (hash != boundary_hash()) return false;
    pixie::World* world = boundary_world_create();
    world->Create("module-added").Add<boundary_fixture::Component>(42.0);
    const bool result = boundary_world_check(world);
    double total = 0;
    world->Each<boundary_fixture::Component>([&](pixie::Entity, auto& value) { total += value.value; });
    boundary_world_destroy(world);
    boundary_factory(&Create);
    return result && total == 42;
}
