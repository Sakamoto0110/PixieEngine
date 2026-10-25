#include "Check.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/World.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace pixie;
namespace {
using Json = nlohmann::ordered_json;

// A motor as an older (or newer) build saved it: only the fields given, so the others keep defaults.
std::string Motor(const std::string& id, double x, const std::string& motorFields, int version = 1) {
    return R"({"id": ")" + id + R"(", "components": {"Transform2D": {"v": 1, "position": [)" + std::to_string(x)
         + R"(, 0]}, "CharacterMotor2D": {"v": )" + std::to_string(version) + ", " + motorFields + "}}}";
}
std::string Settings(double gravityY) {
    return R"({"id": "5e77", "components": {"PhysicsSettings2D": {"v": 1, "gravity": [0, )"
         + std::to_string(gravityY) + "]}}}";
}
std::string Scene(const std::vector<std::string>& entities) {
    std::string list;
    for (const auto& entity : entities) list += (list.empty() ? "" : ", ") + entity;
    return R"({"format": "pixie-scene", "version": 1, "seed": 1, "entities": [)" + list + "]}";
}
ComponentTypes Types() {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    return types;
}
// The loaded motors by their starting X.
std::map<double, entt::entity> Motors(World& world) {
    std::map<double, entt::entity> motors;
    for (auto [id, motor, transform] : world.Registry().view<const CharacterMotor2D, const Transform2D>().each()) {
        (void)motor;
        motors[transform.position.x] = id;
    }
    return motors;
}
Json SavedMotor(const std::string& text) {
    return Json::parse(text)["entities"][0]["components"]["CharacterMotor2D"];
}
}

// PHY-018: no settings, legacy zero and several differing legacy values, with and without world
// settings. Each motor's first airborne step must gain what the version 1 rules gave it: its own
// acceleration without settings, world gravity × (acceleration / 1100) with them.
TEST(LegacyMotorGravityBecomesWorldGravityScaleWithoutChangingTheFall) {
    const auto types = Types();
    const std::map<double, double> legacy{{0, 1100}, {100, 550}, {200, 0}, {300, 1000}};
    for (const bool withSettings : {false, true}) {
        std::vector<std::string> entities;
        for (const auto& [x, gravity] : legacy)
            entities.push_back(Motor("10" + std::to_string(static_cast<int>(x) + 1), x,
                                     R"("gravity": )" + std::to_string(gravity)));
        if (withSettings) entities.push_back(Settings(600));
        World world;
        const auto report = ReadScene(world, types, Scene(entities));
        CHECK(report.ok && report.warnings.empty());
        auto motors = Motors(world);
        CHECK(motors.size() == legacy.size());
        InstallSceneMovement2D(world);
        world.Step(1.0 / 60, {});
        CHECK(SceneMovementErrors2D(world).empty());
        for (const auto& [x, gravity] : legacy) {
            const auto& motor = world.Registry().get<CharacterMotor2D>(motors[x]);
            CHECK(motor.gravityScale == gravity / 1100.0);
            const double versionOneGravity = withSettings ? 600 * (gravity / 1100.0) : gravity;
            const auto* motion = CharacterMotion2D(world, motors[x]);
            CHECK(motion != nullptr);
            // The step's delta is single precision, so the measured velocity is too.
            if (motion) CHECK(std::abs(motion->velocity.y - versionOneGravity / 60) < 1e-4);
        }
    }
}

TEST(MigratedMotorSavesAsVersionTwoOnceAndKeepsUnknownData) {
    const auto types = Types();
    World world;
    const auto report = ReadScene(world, types,
        Scene({Motor("a1", 0, R"("maxSpeed": 250, "gravity": 550, "futureTuning": [1, 2], "futureName": "x")")}));
    CHECK(report.ok && report.warnings.empty());
    const auto text = WriteScene(world, types);
    const auto saved = SavedMotor(text);
    CHECK(saved["v"] == 2);
    CHECK(saved["gravityScale"] == 0.5);
    CHECK(saved["maxSpeed"] == 250);
    CHECK(!saved.contains("gravity"));
    // Unknown fields, even numeric ones offered to the upgrade, survive unchanged.
    CHECK(saved["futureTuning"] == Json::array({1, 2}));
    CHECK(saved["futureName"] == "x");
    // A version 2 save is not converted again.
    World reopened;
    CHECK(ReadScene(reopened, types, text).ok);
    for (auto [id, motor] : reopened.Registry().view<const CharacterMotor2D>().each()) {
        (void)id;
        CHECK(motor.gravityScale == 0.5);
    }
    CHECK(SavedMotor(WriteScene(reopened, types)) == saved);
}

// Sensitivity: the conversion depends on the saved version, not on the presence of a "gravity" key.
TEST(OnlyOlderMotorVersionsConvertLegacyGravity) {
    const auto types = Types();
    for (const int version : {2, 3}) {
        World world;
        const auto report = ReadScene(world, types, Scene({Motor("b1", 0, R"("gravity": 550)", version)}));
        CHECK(report.ok);
        CHECK(report.warnings.empty() == (version == 2)); // a newer version warns
        for (auto [id, motor] : world.Registry().view<const CharacterMotor2D>().each()) {
            (void)id;
            CHECK(motor.gravityScale == 1);
        }
        CHECK(SavedMotor(WriteScene(world, types))["gravity"] == 550); // kept as unknown data
    }
}

// A frozen copy of the movement laboratory as saved before version 2, kept for comparison: the
// sample itself may be saved again at version 2. It is read in place and never rewritten here.
TEST(MovementLabVersionOneSaveLoadsWithItsOriginalGravity) {
    std::ifstream file(PIXIE_TEST_MOVEMENT_V1_SCENE, std::ios::binary);
    std::stringstream contents;
    contents << file.rdbuf();
    const auto original = Json::parse(contents.str());
    bool legacyMotor = false;
    for (const auto& entity : original["entities"])
        if (entity["components"].contains("CharacterMotor2D")) {
            const auto& motor = entity["components"]["CharacterMotor2D"];
            legacyMotor = motor["v"] == 1 && motor["gravity"] == 1100;
        }
    CHECK(legacyMotor);
    const auto types = Types();
    World world;
    const auto report = LoadSceneFile(world, types, PIXIE_TEST_MOVEMENT_V1_SCENE);
    CHECK(report.ok && report.warnings.empty());
    CHECK(world.Registry().view<PhysicsSettings2D>().empty());
    const auto motors = world.Registry().view<const CharacterMotor2D>();
    CHECK(motors.size() == 1);
    if (motors.empty()) return;
    const auto id = *motors.begin();
    // × the default 1100 world gravity: the acceleration the file saved.
    CHECK(world.Registry().get<CharacterMotor2D>(id).gravityScale == 1);
    InstallSceneMovement2D(world);
    for (int i = 0; i < 120; ++i) world.Step(1.0 / 60, {});
    CHECK(SceneMovementErrors2D(world).empty());
    CHECK(CharacterMotion2D(world, id) != nullptr && CharacterMotion2D(world, id)->grounded);
}

TEST(InvalidMotorGravityScaleIsDiagnosedAndRecovers) {
    World world;
    auto actor = world.Create("motor");
    actor.Add<Transform2D>();
    actor.Add<CharacterMotor2D>();
    InstallSceneMovement2D(world);
    for (const double invalid : {-1.0, std::numeric_limits<double>::quiet_NaN()}) {
        actor.Get<CharacterMotor2D>().gravityScale = invalid;
        SyncSceneMovement2D(world);
        CHECK(CharacterMotion2D(world, actor.Id()) == nullptr);
        const auto errors = SceneMovementErrors2D(world);
        CHECK(errors.size() == 1 && errors[0].find("gravity scale") != std::string::npos);
    }
    actor.Get<CharacterMotor2D>().gravityScale = 0;
    world.Step(1.0 / 60, {});
    CHECK(SceneMovementErrors2D(world).empty());
    CHECK(CharacterMotion2D(world, actor.Id()) != nullptr && CharacterMotion2D(world, actor.Id())->velocity.y == 0);
}
