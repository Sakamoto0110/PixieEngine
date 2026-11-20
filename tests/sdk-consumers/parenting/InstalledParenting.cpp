#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Parenting2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/World.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

pixie::Entity Find(pixie::World& world, std::uint64_t id) {
    pixie::Entity found;
    world.Each<const pixie::SceneId>([&](pixie::Entity entity, const pixie::SceneId& value) {
        if (value.value == id) found = entity;
    });
    if (!found) throw std::runtime_error("missing fixture entity");
    return found;
}

void CheckPose(pixie::Entity entity, double x, double y) {
    const auto& pose = entity.Get<pixie::Transform2D>();
    if (!(std::abs(pose.position.x - x) < 1e-9 && std::abs(pose.position.y - y) < 1e-9)) {
        throw std::runtime_error("fixture pose differs from the expected world coordinates");
    }
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 2) throw std::runtime_error("pass the path to the parenting fixture scene");
        pixie::ComponentTypes types;
        pixie::DescribeRuntimeComponents(types);
        pixie::World world;
        const auto load = pixie::LoadSceneFile(world, types, argv[1]);
        if (!load.ok) throw std::runtime_error(load.error);
        if (!load.warnings.empty() || pixie::SceneFormatVersion(world) != 3) {
            throw std::runtime_error("fixture was not admitted as a clean version 3 scene");
        }
        const auto marker = Find(world, 1);
        const auto pivot = Find(world, 3);
        if (pixie::Parent2D(marker) != pivot || pixie::Parent2D(pivot) != Find(world, 4)) {
            throw std::runtime_error("fixture links were not resolved");
        }
        CheckPose(marker, 120, 110);
        world.Step(1, pixie::Input{});
        CheckPose(marker, 90, 120);

        const auto snapshot = pixie::WriteScene(world, types);
        pixie::World reopened;
        if (!pixie::ReadScene(reopened, types, snapshot).ok || pixie::WriteScene(reopened, types) != snapshot) {
            throw std::runtime_error("authored hierarchy did not survive round trip");
        }
        CheckPose(Find(reopened, 1), 90, 120);
        if (pixie::Parent2D(Find(reopened, 1)) != Find(reopened, 3)) {
            throw std::runtime_error("reopened scene lost its link");
        }
        std::cout << "Installed SDK: v3 links, composed poses and scene round trip passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
