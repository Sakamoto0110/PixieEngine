#pragma once
#include "pixie/runtime/Project.hpp"
#include "pixie/runtime/Entity.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include "pixie/Telemetry.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/RigidBody2D.hpp"

struct SparksScenario { int burstCount = 48; };

class SparksProject final : public pixie::Project {
public:
    std::string_view Name() const override { return "sparks"; }
    void Describe(pixie::ComponentTypes& types) const override;
    void Build(pixie::World& world, pixie::Camera2D& camera) override;
    void BeforeStep(pixie::World& world, pixie::Camera2D& camera, const pixie::Input& input) override;
    void DrawScreen(pixie::World& world, pixie::ScreenCanvas& screen) override;
    bool SpawnAt(pixie::World& world, pxPoint at) override;
private:
    void Burst(pixie::World& world, pxPoint at, int count, double direction = 0);
    void ResetRoom(pixie::World& world);
    void ContactSparks(pixie::World& world, double delta);
    using ContactKey = std::tuple<std::uint64_t, std::uint64_t, std::uint16_t>;
    struct ContactHistory { double impulse = 0, pending = 0; };
    std::map<ContactKey, ContactHistory> contacts_;
    pixie::MetricHandle contactTime_, contactCount_, emittedCount_;
    struct InitialBody { pixie::Entity entity; pixie::Transform2D transform; pixie::RigidBody2D body; };
    std::vector<InitialBody> initialBodies_;
    pixie::Entity selected_, dragged_;
    pxPoint dragLocal_;
    std::string massText_, massError_;
    bool editingMass_ = false, replaceMass_ = false;
    void SelectBody(pixie::Entity entity);
    void EditMass(const pixie::Input& input);
    pixie::Entity pool_, box_, floor_, scenario_, player_;
    pxPoint initialBox_, initialPlayer_;
    std::uint64_t seed_ = 0;
};
