#include "Check.hpp"

#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/AttractorAffector.hpp"
#include "pixie/runtime/InteractionAffector.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleStep.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/RigidBody2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace pixie;

TEST(DeclarativeProjectDataRoundTripsV2WhileUnsupportedTypesRefusePureAdmission) {
    struct Data { double value; int count; };
    struct LegacyData { double value; };
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    types.AddData<Data>("Data").Field<&Data::value>("value").Field<&Data::count>("count");
    types.Add<LegacyData>("LegacyData").Field<&LegacyData::value>("value");
    const std::string text = R"({"format":"pixie-scene","version":2,"seed":3,"sceneScopes":[],"entities":[{"id":"0000000000000001","components":{"Data":{"v":1,"value":12.5,"count":7,"future":42}}}]})";
    CHECK(SceneImportAdmissionError(types, text).empty());
    World world;
    CHECK(ReadScene(world, types, text).ok);
    const auto view = world.Registry().view<const Data>();
    CHECK(view.size() == 1);
    CHECK(view.get<const Data>(view.front()).value == 12.5);
    CHECK(view.get<const Data>(view.front()).count == 7);
    const auto written = WriteScene(world, types);
    CHECK(written.find("future") != std::string::npos);
    const auto before = written;
    auto unsupported = nlohmann::json::parse(text);
    unsupported["entities"][0]["components"]["LegacyData"] = {{"v", 1}};
    const auto refused = SceneImportAdmissionError(types, unsupported.dump());
    CHECK(refused.find("LegacyData") != std::string::npos);
    CHECK(!ReadScene(world, types, unsupported.dump()).ok);
    CHECK(WriteScene(world, types) == before);
    for (const auto invalid : {1e100, -1e100, 1.5}) {
        auto malformed = nlohmann::json::parse(text);
        malformed["entities"][0]["components"]["Data"]["count"] = invalid;
        World rejected;
        CHECK(!ReadScene(rejected, types, malformed.dump()).ok);
        CHECK(rejected.Registry().view<SceneId>().empty());
    }
}

namespace {

const Input kNoInput;

// A Behaviour declared with fields, saved like any component.
struct Spinner : Behaviour {
    double speed = 1.0;
    int turns = 0;
    void Update(const FrameTime&, const Input&) override { ++turns; }
};

struct ThrowOnLoad {
    int marker = 0;
    static inline int attempts = 0;
    static inline int failureAt = 0;
    static inline World* consumeFrom = nullptr;
    static inline Entity* existingToMutate = nullptr;
    static inline int externalEffects = 0;
    ThrowOnLoad() {
        ++attempts;
        if (consumeFrom != nullptr) consumeFrom->GetRandom().NextU64();
        if (existingToMutate != nullptr) {
            existingToMutate->Get<Transform2D>().position = pxPoint(777.0, 888.0);
            ++externalEffects;
        }
        if (attempts == failureAt) throw std::runtime_error("intentional component construction failure");
    }
};

World* hookWorld = nullptr;
Entity* hookExisting = nullptr;
int hookExternalEffects = 0;

void ThrowFromRuntimeComponentHook(entt::registry&, entt::entity) {
    hookExisting->Get<Transform2D>().position = pxPoint(777.0, 888.0);
    hookWorld->GetRandom().NextU64();
    ++hookExternalEffects;
    throw std::runtime_error("intentional runtime component hook failure");
}

ComponentTypes Types() {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    types.Add<Spinner>("Spinner").Field<&Spinner::speed>("speed").Field<&Spinner::turns>("turns");
    types.Add<ThrowOnLoad>("ThrowOnLoad");
    return types;
}

// A World with every built-in component, a Behaviour, an Entity field, and one generated entity.
void Fill(World& world) {
    Entity pool = world.Create("pool");
    pool.Add<Transform2D>(Transform2D{pxPoint(12.5, -3.0), 0.75, pxVec2(2.0, 0.5)});
    pool.Add<Velocity2D>(Velocity2D{pxVec2(1.0, 2.0), 0.1});
    pool.Add<Acceleration2D>(Acceleration2D{pxVec2(-4.0, 9.81)});
    pool.Add<Shape>(Shape{ShapeKind::Ring, pxVec2(3.0, 4.0), pxColorRgba(10, 20, 30, 40)});
    ParticleSystem& system = pool.Add<ParticleSystem>();
    system.capacity = 5000;
    system.style.color = pxColorRgba(255, 120, 40, 200);
    system.style.size = 3.5f;
    system.style.additive = false;
    system.style.fade = false;
    system.style.shape = ParticleShape::Streak;
    pool.Add<ParticleAttraction>(ParticleAttraction{false, -0.5});
    pool.Add<ParticleInteraction>(ParticleInteraction{true, 6.5, 900.0, 1.25});
    Attractor& attractor = pool.Add<Attractor>();
    attractor.strength = -2.5e5;
    attractor.killOnContact = true;
    AddToScene(pool);

    Entity spawner = world.Create("spawner");
    spawner.Add<Transform2D>();
    ParticleSpawner& feed = spawner.Add<ParticleSpawner>();
    feed.system = pool.Id();
    feed.rate = 120.0;
    feed.lifetime = 0.6f;
    feed.shape = EmissionShape::Ring;
    feed.directionMode = DirectionMode::TowardTarget;
    feed.segmentEnd = pxVec2(5.0, -5.0);
    feed.ownLook = true;
    feed.particleColor = pxColorRgba(30, 60, 90, 120);
    feed.particleSize = 4.5;
    feed.particleShape = ParticleShape::Square;
    AddToScene(spawner);

    Entity spinner = world.Create();
    spinner.AddBehaviour<Spinner>().speed = 2.25;
    AddToScene(spinner);

    world.Create("generated").Add<Transform2D>();
}

std::unordered_map<std::uint64_t, Entity> ById(World& world) {
    std::unordered_map<std::uint64_t, Entity> found;
    world.Each<const SceneId>([&](Entity entity, const SceneId& id) { found.emplace(id.value, entity); });
    return found;
}

} // namespace

TEST(ARoundTripKeepsEveryComponentAndValue) {
    const ComponentTypes types = Types();
    World original(7);
    Fill(original);
    std::vector<std::string> warnings;
    const std::string text = WriteScene(original, types, &warnings);
    CHECK(warnings.empty());

    World loaded(1);
    const SceneReport report = ReadScene(loaded, types, text);
    CHECK(report.ok && report.warnings.empty());
    CHECK(loaded.GetRandom().Seed() == 7);
    CHECK(loaded.EntityCount() == 3);

    auto before = ById(original);
    auto after = ById(loaded);
    CHECK(before.size() == 3 && after.size() == 3);
    for (const auto& [id, entity] : before) {
        CHECK(after.contains(id));
        if (!after.contains(id)) {
            continue;
        }
        const Entity copy = after[id];
        const Name* name = entity.TryGet<Name>();
        const Name* copyName = copy.TryGet<Name>();
        CHECK((name == nullptr) == (copyName == nullptr));
        CHECK(name == nullptr || name->value == copyName->value);
        for (const ComponentDescriptor& type : types.All()) {
            void* a = type.find(original.Registry(), entity.Id());
            void* b = type.find(loaded.Registry(), copy.Id());
            CHECK((a == nullptr) == (b == nullptr));
            if (a == nullptr || b == nullptr || type.name == "ParticleSpawner") {
                continue;
            }
            for (const FieldDescriptor& field : type.fields) {
                const FieldValue x = field.read(a);
                const FieldValue y = field.read(b);
                CHECK(x.x == y.x && x.y == y.y && x.z == y.z && x.w == y.w);
            }
        }
    }

    // The spawner's system is the loaded pool, found through its ID.
    Entity spawner;
    Entity pool;
    loaded.Each<const ParticleSpawner>([&](Entity entity, const ParticleSpawner&) { spawner = entity; });
    loaded.Each<const ParticleSystem>([&](Entity entity, const ParticleSystem&) { pool = entity; });
    CHECK(spawner && pool);
    const ParticleSpawner& feed = spawner.Get<ParticleSpawner>();
    CHECK(feed.system == pool.Id());
    CHECK(feed.target == entt::null);
    CHECK(feed.lifetime == 0.6f && feed.rate == 120.0);
    CHECK(feed.shape == EmissionShape::Ring && feed.directionMode == DirectionMode::TowardTarget);
    CHECK(feed.ownLook && feed.particleColor == pxColorRgba(30, 60, 90, 120));
    CHECK(feed.particleSize == 4.5 && feed.particleShape == ParticleShape::Square);

    // The pool's draw style comes back too.
    const ParticleStyle& style = pool.Get<ParticleSystem>().style;
    CHECK(style.color == pxColorRgba(255, 120, 40, 200) && style.size == 3.5f);
    CHECK(!style.additive && !style.fade && style.shape == ParticleShape::Streak);

    // Writing the loaded World gives the same text.
    CHECK(WriteScene(loaded, types) == text);
}

TEST(ASceneSpawnerEmitsWithTheParticleStepInstalledOnce) {
    const ComponentTypes types = Types();
    World original;
    Entity pool = original.Create("pool");
    pool.Add<ParticleSystem>();
    AddToScene(pool);
    Entity spawner = original.Create("spawner");
    spawner.Add<Transform2D>();
    ParticleSpawner& feed = spawner.Add<ParticleSpawner>();
    feed.system = pool.Id();
    feed.rate = 60.0; // one per step
    AddToScene(spawner);

    World loaded;
    CHECK(ReadScene(loaded, types, WriteScene(original, types)).ok);
    // As a project that adds the step itself, before the session adds it to every World.
    AddParticleStep(loaded);
    AddParticleStep(loaded);
    for (int i = 0; i < 10; ++i) {
        loaded.Step(1.0 / 60.0, kNoInput);
    }
    std::size_t particles = 0;
    loaded.Each<const ParticleSystem>([&](Entity, const ParticleSystem& system) { particles += system.particles.size(); });
    CHECK(particles == 10);
}

TEST(ABehaviourLoadedFromASceneRuns) {
    const ComponentTypes types = Types();
    World original;
    Fill(original);
    World loaded;
    CHECK(ReadScene(loaded, types, WriteScene(original, types)).ok);
    int spinners = 0;
    loaded.Each<const Spinner>([&](Entity, const Spinner& spinner) {
        ++spinners;
        CHECK(spinner.speed == 2.25);
    });
    CHECK(spinners == 1);
    loaded.Step(0.1, kNoInput);
    loaded.Each<const Spinner>([](Entity, const Spinner& spinner) { CHECK(spinner.turns == 1); });
}

TEST(GeneratedEntitiesAreNotWritten) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const auto document = nlohmann::json::parse(WriteScene(world, types));
    CHECK(document["entities"].size() == 3);
    for (const auto& entity : document["entities"]) {
        CHECK(entity.value("name", "") != "generated");
    }
}

TEST(TheFileShowsReadableValues) {
    const ComponentTypes types = Types();
    World world(42);
    Fill(world);
    const auto document = nlohmann::json::parse(WriteScene(world, types));
    CHECK(document["format"] == "pixie-scene" && document["version"] == 1 && document["seed"] == 42);
    const auto& pool = document["entities"][0];
    CHECK(pool["name"] == "pool");
    CHECK(pool["id"].get<std::string>().size() == 16);
    const auto& components = pool["components"];
    CHECK(components["Transform2D"]["v"] == 1);
    CHECK(components["Transform2D"]["position"] == nlohmann::json::array({12.5, -3.0}));
    CHECK(components["Shape"]["kind"] == "Ring");
    CHECK(components["Shape"]["color"] == nlohmann::json::array({10, 20, 30, 40}));
    CHECK(components["Attractor"]["killOnContact"] == true);
    const auto& feed = document["entities"][1]["components"]["ParticleSpawner"];
    CHECK(feed["system"] == pool["id"]);
    CHECK(feed["target"].is_null());
    // A float is saved as its shortest decimal.
    CHECK(feed["lifetime"].get<double>() == 0.6);
}

TEST(UnknownComponentsAndFieldsAreWrittenBackUnchanged) {
    const std::string text = R"({
  "format": "pixie-scene",
  "version": 1,
  "seed": 3,
  "editor": {"camera": [1, 2], "zoom": 1.5},
  "entities": [
    {
      "id": "00000000000000aa",
      "name": "thing",
      "parent": "00000000000000bb",
      "layer": 4,
      "components": {
        "Transform2D": {"v": 1, "position": [1.0, 2.0], "rotation": 0.0, "scale": [1.0, 1.0], "wobble": 5},
        "Mystery": {"v": 3, "a": [1, 2], "b": {"deep": true}}
      }
    },
    {
      "id": "00000000000000bb",
      "components": {"FromAnotherGame": {"v": 1}}
    }
  ]
})";
    const ComponentTypes types = Types();
    World world;
    const SceneReport report = ReadScene(world, types, text);
    CHECK(report.ok && report.warnings.empty());
    CHECK(world.EntityCount() == 2);
    const std::string written = WriteScene(world, types);
    CHECK(nlohmann::json::parse(written) == nlohmann::json::parse(text));

    // The kept values survive the next load and save too.
    World again;
    CHECK(ReadScene(again, types, written).ok);
    CHECK(WriteScene(again, types) == written);
}

TEST(AFileThatCannotBeReadAddsNothing) {
    const ComponentTypes types = Types();
    World world;
    world.Create("already here");
    const auto fails = [&](const std::string& text) {
        const SceneReport report = ReadScene(world, types, text);
        CHECK(!report.ok && !report.error.empty());
        CHECK(world.EntityCount() == 1);
    };
    fails("{ not json");
    fails(R"({"format": "something-else", "version": 1, "entities": []})");
    fails(R"({"format": "pixie-scene", "version": 2, "entities": []})");
    fails(R"({"format": "pixie-scene", "version": 1})");
    fails(R"({"format": "pixie-scene", "version": 1, "entities": [{"id": "0"}]})");
    fails(R"({"format": "pixie-scene", "version": 1, "entities": [{"id": "1"}, {"id": "a"}, {"id": "1"}]})");
    fails(R"({"format": "pixie-scene", "version": 1, "entities": [{"id": "1", "components": []}]})");

    // An ID already in the World is refused too.
    World loaded;
    CHECK(ReadScene(loaded, types, R"({"format": "pixie-scene", "version": 1, "seed": 0, "entities": [{"id": "5"}]})").ok);
    const SceneReport twice =
        ReadScene(loaded, types, R"({"format": "pixie-scene", "version": 1, "seed": 0, "entities": [{"id": "5"}]})");
    CHECK(!twice.ok && loaded.EntityCount() == 1);
}

TEST(VersionTwoScopesAndWorldMembershipRoundTripWithUnknownData) {
    const ComponentTypes types = Types();
    const std::string text = R"({"format":"pixie-scene","version":2,"seed":17,
        "futureRoot":{"enabled":true},"sceneScopes":[
          {"id":"00000000000000aa","name":"Empty","futureScope":[1,2]},
          {"id":"00000000000000bb","name":"Room","gravity":{"mode":"inherit","future":9}}],
        "entities":[
          {"id":"00000000000000aa","name":"actor","sceneScope":"00000000000000bb","components":{}},
          {"id":"00000000000000cc","name":"corridor","sceneScope":null,"components":{}},
          {"id":"00000000000000dd","name":"outside","components":{}}]})";
    World world(3);
    CHECK(ReadScene(world, types, text).ok);
    CHECK(world.GetRandom().Seed() == 17);
    CHECK(world.Scopes().size() == 2);
    const auto members = ById(world);
    CHECK(world.ScopeOf(members.at(0xaa)) == ScopeId{0xbb});
    CHECK(world.ScopeOf(members.at(0xcc)) == ScopeId{});
    CHECK(world.ScopeOf(members.at(0xdd)) == ScopeId{});
    const auto written = nlohmann::json::parse(WriteScene(world, types));
    CHECK(written["version"] == 2);
    CHECK(written["futureRoot"]["enabled"] == true);
    CHECK(written["sceneScopes"][0]["futureScope"] == nlohmann::json::array({1, 2}));
    CHECK(written["sceneScopes"][1]["gravity"]["future"] == 9);
    CHECK(written["entities"][0]["sceneScope"] == "00000000000000bb");
    CHECK(!written["entities"][1].contains("sceneScope"));
    CHECK(!written["entities"][2].contains("sceneScope"));
    World again;
    CHECK(ReadScene(again, types, WriteScene(world, types)).ok);
    CHECK(WriteScene(again, types) == WriteScene(world, types));
}

TEST(VersionTwoSceneGravityRoundTripsInactiveValuesAndUnknownFields) {
    const ComponentTypes types = Types();
    const std::string text = R"({"format":"pixie-scene","version":2,"seed":41,
        "sceneScopes":[{"id":"aa","name":"Room","gravity":{
            "mode":"multiply","value":[30,40],"multiplier":0.75,"futureModeData":{"keep":true}}}],
        "entities":[{"id":"1","sceneScope":"aa","components":{}}]})";
    World world;
    CHECK(ReadScene(world, types, text).ok);
    CHECK(world.ResolveGravity(ScopeId{0xaa}).value == pxVec2(0, 825));
    CHECK(world.SetScopeGravity(ScopeId{0xaa}, {SceneGravityMode2D::Inherit, {30, 40}, 0.75}));
    const auto written = nlohmann::json::parse(WriteScene(world, types));
    CHECK(written["sceneScopes"][0]["gravity"]["mode"] == "inherit");
    CHECK(written["sceneScopes"][0]["gravity"]["value"] == nlohmann::json::array({30, 40}));
    CHECK(written["sceneScopes"][0]["gravity"]["multiplier"] == 0.75);
    CHECK(written["sceneScopes"][0]["gravity"]["futureModeData"]["keep"] == true);
    World again;
    CHECK(ReadScene(again, types, WriteScene(world, types)).ok);
    CHECK(WriteScene(again, types) == WriteScene(world, types));
    const auto fragment = WriteEntities(world, types, std::array{ById(world).at(1)});
    CHECK(nlohmann::json::parse(fragment)["sceneScopes"][0]["gravity"]["futureModeData"]["keep"] == true);
    World imported;
    CHECK(ReadScene(imported, types, R"({"format":"pixie-scene","version":2,"seed":41,"sceneScopes":[],"entities":[]})").ok);
    CHECK(ReadEntities(imported, types, fragment, SceneIds::Keep).ok);
    CHECK(imported.Scopes()[0].gravity == world.Scopes()[0].gravity);
}

TEST(VersionTwoInvalidSceneGravityFailsBeforeWorldMutation) {
    const ComponentTypes types = Types();
    World world(23);
    world.Create("existing");
    const auto fails = [&](std::string_view gravity) {
        const std::string text = std::string(R"({"format":"pixie-scene","version":2,"seed":91,
            "sceneScopes":[{"id":"aa","name":"Room","gravity":)") + std::string(gravity) +
            R"(}],"entities":[{"id":"1","sceneScope":"aa","components":{}}]})";
        const SceneReport result = ReadScene(world, types, text);
        CHECK(!result.ok && !result.error.empty());
        CHECK(world.EntityCount() == 1 && world.Scopes().empty() && world.GetRandom().Seed() == 23);
    };
    fails(R"({"mode":"future"})");
    fails(R"({"mode":"multiply","value":[0,0],"multiplier":-1})");
    fails(R"({"mode":"inherit","value":[1e300,0],"multiplier":1})");
    fails(R"({"mode":"multiply","value":[0,0],"multiplier":1e300})");
    fails(R"({"mode":"override","value":[0],"multiplier":1})");
    const SceneReport incomingSettings = ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":91,
        "sceneScopes":[{"id":"aa","name":"Room","gravity":{
            "mode":"override","value":[2000,0],"multiplier":1}}],
        "entities":[{"id":"2","components":{"PhysicsSettings2D":{"v":1,"unitsPerMeter":0.001}}}]})");
    CHECK(!incomingSettings.ok && world.EntityCount() == 1 && world.Scopes().empty());
    CHECK(world.GetRandom().Seed() == 23);
}

TEST(VersionTwoFragmentRejectsConflictingGravityWithoutImportingEntities) {
    const ComponentTypes types = Types();
    World world(29);
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":29,
        "sceneScopes":[{"id":"aa","name":"Room","gravity":{
            "mode":"override","value":[0,50],"multiplier":0.5,"future":7}}],"entities":[]})").ok);
    const std::string before = WriteScene(world, types);
    const std::string fragment = R"({"format":"pixie-entities","version":2,
        "sceneScopes":[{"id":"aa","name":"Room","gravity":{
            "mode":"override","value":[0,60],"multiplier":0.5,"future":7}}],
        "entities":[{"id":"b","sceneScope":"aa","components":{}}]})";
    const auto result = ReadEntities(world, types, fragment, SceneIds::Keep);
    CHECK(!result.ok && result.error.find("conflicts") != std::string::npos);
    CHECK(world.EntityCount() == 0 && world.GetRandom().Seed() == 29);
    CHECK(WriteScene(world, types) == before);
}

TEST(VersionTwoSceneRejectsDuplicatePhysicsAuthoritiesBeforeMutation) {
    const ComponentTypes types = Types();
    World world(13);
    const SceneReport result = ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":91,
        "sceneScopes":[{"id":"aa","name":"Room","gravity":{"mode":"inherit"}}],
        "entities":[{"id":"1","components":{"PhysicsSettings2D":{}}},
                    {"id":"2","components":{"PhysicsSettings2D":{}}}]})");
    CHECK(!result.ok && result.error.find("PhysicsSettings2D") != std::string::npos);
    CHECK(world.EntityCount() == 0 && world.Scopes().empty() && world.GetRandom().Seed() == 13);
}

TEST(VersionTwoInvalidScopesLeaveWorldAndRandomUnchanged) {
    const ComponentTypes types = Types();
    const std::string prefix = R"({"format":"pixie-scene","version":2,"seed":91,)";
    World world(23);
    world.Create("existing");
    const auto fails = [&](const std::string& tail) {
        const SceneReport report = ReadScene(world, types, prefix + tail);
        CHECK(!report.ok && !report.error.empty());
        CHECK(world.EntityCount() == 1);
        CHECK(world.Scopes().empty());
        CHECK(world.GetRandom().Seed() == 23);
    };
    fails(R"("sceneScopes":[{"id":"a","name":"one"},{"id":"a","name":"two"}],"entities":[]})");
    fails(R"("sceneScopes":[{"id":"0","name":"zero"}],"entities":[]})");
    fails(R"("sceneScopes":[{"id":"a","name":"room"}],"entities":[{"id":"1","sceneScope":"b"}]})");
    fails(R"("sceneScopes":[{"id":"a","name":"room"}],"entities":[{"id":"1","sceneScope":0}]})");
    fails(R"("sceneScopes":[],"entities":[{"id":"1"},{"id":"1"}]})");
}

TEST(VersionOneSceneRemainsVersionOneAfterReadAndWrite) {
    const ComponentTypes types = Types();
    const std::string text = R"({"format":"pixie-scene","version":1,"seed":7,
        "futureRoot":{"keep":true},"entities":[{"id":"1","sceneScope":"legacy-unknown"}]})";
    World world;
    CHECK(ReadScene(world, types, text).ok);
    const auto written = nlohmann::json::parse(WriteScene(world, types));
    CHECK(written["version"] == 1);
    CHECK(written["futureRoot"]["keep"] == true);
    CHECK(written["entities"][0]["sceneScope"] == "legacy-unknown");
    CHECK(world.CreateScope(ScopeId{0xaa}, "new scope"));
    bool conversionRequired = false;
    try { WriteScene(world, types); }
    catch (const std::runtime_error&) { conversionRequired = true; }
    CHECK(conversionRequired);
}

TEST(ExplicitConversionPreservesVersionOneBackupAndWritesCompleteVersionTwo) {
    const ComponentTypes types = Types();
    const std::string source = R"({"format":"pixie-scene","version":1,"seed":31,
        "futureRoot":{"keep":true},"entities":[{"id":"a","components":{}}]})";
    const auto directory = std::filesystem::temp_directory_path() /
        ("pixie-convert-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto path = directory / "start.scene";
    { std::ofstream out(path, std::ios::binary); out << source; }
    World world;
    CHECK(ReadScene(world, types, source).ok);
    CHECK(world.CreateScope(ScopeId{0xaa}, "Empty"));
    CHECK(world.CreateScope(ScopeId{0xbb}, "Room"));
    CHECK(world.SetScope(ById(world).at(0xa), ScopeId{0xbb}));
    const SceneReport converted = ConvertSceneFileV1ToV2(world, types, path);
    if (!converted.ok) std::fprintf(stderr, "conversion failed: %s\n", converted.error.c_str());
    CHECK(converted.ok);
    if (!converted.ok) return;
    std::ifstream backup(path.string() + ".v1-backup", std::ios::binary);
    std::ostringstream preserved;
    preserved << backup.rdbuf();
    CHECK(preserved.str() == source);
    backup.close();
    World reopened;
    CHECK(LoadSceneFile(reopened, types, path).ok);
    CHECK(reopened.Scopes().size() == 2);
    CHECK(reopened.ScopeOf(ById(reopened).at(0xa)) == ScopeId{0xbb});
    const auto document = nlohmann::json::parse(WriteScene(reopened, types));
    CHECK(document["version"] == 2);
    CHECK(document["sceneScopes"].size() == 2);
    CHECK(document["futureRoot"]["keep"] == true);
    CHECK(nlohmann::json::parse(WriteScene(world, types))["version"] == 2);
    std::filesystem::remove(path);
    std::filesystem::remove(path.string() + ".v1-backup");
    std::filesystem::remove(directory);
}

TEST(ExplicitConversionFailureLeavesOriginalAndWorldUntouched) {
    const ComponentTypes types = Types();
    const std::string source = R"({"format":"pixie-scene","version":1,"seed":31,"entities":[]})";
    const auto directory = std::filesystem::temp_directory_path() /
        ("pixie-convert-failure-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto path = directory / "start.scene";
    { std::ofstream out(path, std::ios::binary); out << source; }
    World world;
    CHECK(ReadScene(world, types, source).ok);
    CHECK(world.CreateScope(ScopeId{0xaa}, "Empty"));
    auto originalOnDisk = [&] {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str() == source;
    };
    Entity invalid = world.Create();
    invalid.Add<SceneId>(SceneId{0});
    CHECK(!ConvertSceneFileV1ToV2(world, types, path).ok);
    CHECK(originalOnDisk() && world.GetRandom().Seed() == 31);
    CHECK(world.Scopes().size() == 1);
    CHECK(!std::filesystem::exists(path.string() + ".v1-backup"));
    invalid.Destroy();
    { std::ofstream out(path.string() + ".v1-backup", std::ios::binary); out << "keep"; }
    CHECK(!ConvertSceneFileV1ToV2(world, types, path).ok);
    CHECK(originalOnDisk());
    std::filesystem::remove(path.string() + ".v1-backup");
    { std::ofstream out(path.string() + ".converting", std::ios::binary); out << "keep"; }
    CHECK(!ConvertSceneFileV1ToV2(world, types, path).ok);
    CHECK(originalOnDisk());
    std::filesystem::remove(path.string() + ".converting");
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
}

TEST(ExplicitConversionRejectsRegisteredProjectComponentsBeforeWritingFiles) {
    const ComponentTypes types = Types();
    const std::string source = R"({"format":"pixie-scene","version":1,"seed":31,
        "entities":[{"id":"a","components":{"ThrowOnLoad":{"marker":7}}}]})";
    const auto directory = std::filesystem::temp_directory_path() /
        ("pixie-convert-custom-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto path = directory / "start.scene";
    { std::ofstream out(path, std::ios::binary); out << source; }
    World world;
    ThrowOnLoad::attempts = 0;
    CHECK(ReadScene(world, types, source).ok);
    const int attemptsAfterV1 = ThrowOnLoad::attempts;
    const std::string before = WriteScene(world, types);
    const SceneReport result = ConvertSceneFileV1ToV2(world, types, path);
    std::ifstream in(path, std::ios::binary);
    std::ostringstream preserved;
    preserved << in.rdbuf();
    in.close();
    CHECK(!result.ok && result.error.find("ThrowOnLoad") != std::string::npos);
    CHECK(ThrowOnLoad::attempts == attemptsAfterV1 && WriteScene(world, types) == before);
    CHECK(preserved.str() == source);
    CHECK(!std::filesystem::exists(path.string() + ".converting"));
    CHECK(!std::filesystem::exists(path.string() + ".v1-backup"));
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
}

TEST(ExplicitConversionRejectsProjectComponentsAddedAfterVersionOneLoad) {
    const ComponentTypes types = Types();
    const std::string source = R"({"format":"pixie-scene","version":1,"seed":31,"entities":[{"id":"a"}]})";
    const auto directory = std::filesystem::temp_directory_path() /
        ("pixie-convert-authored-custom-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto path = directory / "start.scene";
    { std::ofstream out(path, std::ios::binary); out << source; }
    World world;
    CHECK(ReadScene(world, types, source).ok);
    ById(world).at(0xa).Add<Spinner>();
    const std::string before = WriteScene(world, types);
    const SceneReport result = ConvertSceneFileV1ToV2(world, types, path);
    std::ifstream in(path, std::ios::binary);
    std::ostringstream preserved;
    preserved << in.rdbuf();
    in.close();
    CHECK(!result.ok && result.error.find("Spinner") != std::string::npos);
    CHECK(WriteScene(world, types) == before && preserved.str() == source);
    CHECK(!std::filesystem::exists(path.string() + ".converting"));
    CHECK(!std::filesystem::exists(path.string() + ".v1-backup"));
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
}

TEST(ExplicitConversionRefusesConflictingVersionOneUnknownFields) {
    const ComponentTypes types = Types();
    const auto directory = std::filesystem::temp_directory_path() /
        ("pixie-convert-reserved-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto path = directory / "start.scene";
    for (const std::string source : {
             R"({"format":"pixie-scene","version":1,"seed":7,"sceneScopes":{"legacy":true},"entities":[]})",
             R"({"format":"pixie-scene","version":1,"seed":7,"entities":[{"id":"a","sceneScope":"legacy"}]})"}) {
        { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << source; }
        World world;
        CHECK(ReadScene(world, types, source).ok);
        CHECK(world.CreateScope(ScopeId{0xbb}, "new"));
        const SceneReport result = ConvertSceneFileV1ToV2(world, types, path);
        CHECK(!result.ok && result.error.find("conflicts") != std::string::npos);
        std::ifstream in(path, std::ios::binary);
        std::ostringstream unchanged;
        unchanged << in.rdbuf();
        in.close();
        CHECK(unchanged.str() == source);
        CHECK(world.Scopes().size() == 1 && world.GetRandom().Seed() == 7);
        CHECK(!std::filesystem::exists(path.string() + ".v1-backup"));
        CHECK(!std::filesystem::exists(path.string() + ".converting"));
    }
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
}

TEST(VersionTwoFragmentsCarryReferencedScopeDefinitionsAndRemapOnlyEntities) {
    const ComponentTypes types = Types();
    const std::string source = R"({"format":"pixie-scene","version":2,"seed":19,
        "sceneScopes":[{"id":"aa","name":"Room","futureScope":{"keep":true}},
                       {"id":"bb","name":"Empty"}],
        "entities":[{"id":"a","sceneScope":"aa","components":{"ParticleSystem":{"v":1}}},
                    {"id":"b","sceneScope":"aa","components":{"ParticleSpawner":{"v":1,"system":"a"}}}]})";
    World world;
    CHECK(ReadScene(world, types, source).ok);
    const auto originals = ById(world);
    const std::array chosen = {originals.at(0xa), originals.at(0xb)};
    const std::string fragment = WriteEntities(world, types, chosen);
    const auto document = nlohmann::json::parse(fragment);
    CHECK(document["version"] == 2);
    CHECK(document["sceneScopes"].size() == 1);
    CHECK(document["sceneScopes"][0]["futureScope"]["keep"] == true);
    CHECK(document["entities"][0]["sceneScope"] == "00000000000000aa");
    CHECK(ReadFragmentMeta(fragment).ok);

    const EntitiesReport copies = ReadEntities(world, types, fragment, SceneIds::New);
    CHECK(copies.ok && copies.entities.size() == 2);
    CHECK(copies.entities[0].Get<SceneId>().value != 0xa);
    CHECK(copies.entities[1].Get<SceneId>().value != 0xb);
    CHECK(world.ScopeOf(copies.entities[0]) == ScopeId{0xaa});
    CHECK(world.ScopeOf(copies.entities[1]) == ScopeId{0xaa});
    CHECK(copies.entities[1].Get<ParticleSpawner>().system == copies.entities[0].Id());
    CHECK(world.Scopes().size() == 2);

    World imported;
    CHECK(ReadScene(imported, types, R"({"format":"pixie-scene","version":2,"seed":37,"sceneScopes":[],"entities":[]})").ok);
    const EntitiesReport restored = ReadEntities(imported, types, fragment, SceneIds::Keep);
    CHECK(restored.ok && imported.Scopes().size() == 1);
    CHECK(imported.ScopeOf(restored.entities[0]) == ScopeId{0xaa});
    const auto saved = nlohmann::json::parse(WriteScene(imported, types));
    CHECK(saved["sceneScopes"][0]["futureScope"]["keep"] == true);
    CHECK(imported.GetRandom().Seed() == 37);
}

TEST(VersionTwoFragmentConflictsAndBadReferencesLeaveWorldUnchanged) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[{"id":"aa","name":"Room","extra":1}],"entities":[]})").ok);
    const std::string fragment = R"({"format":"pixie-entities","version":2,
        "sceneScopes":[{"id":"aa","name":"Room","extra":2}],
        "entities":[{"id":"a","sceneScope":"aa"}]})";
    auto fails = [&](const std::string& text) {
        const EntitiesReport report = ReadEntities(world, types, text, SceneIds::New);
        CHECK(!report.ok && report.entities.empty());
        CHECK(world.EntityCount() == 0 && world.Scopes().size() == 1);
        CHECK(world.GetRandom().Seed() == 43);
    };
    fails(fragment);
    auto bad = nlohmann::json::parse(fragment);
    bad["sceneScopes"][0]["extra"] = 1;
    bad["entities"][0]["sceneScope"] = "bb";
    fails(bad.dump());
    bad["entities"][0]["sceneScope"] = "aa";
    bad["sceneScopes"].push_back(bad["sceneScopes"][0]);
    fails(bad.dump());
    bad["sceneScopes"].erase(bad["sceneScopes"].begin() + 1);
    bad["entities"] = nlohmann::json::array();
    fails(bad.dump());
    fails(R"({"format":"pixie-entities","version":1,
        "entities":[{"id":"a","sceneScope":"legacy-unknown"}]})");
}

TEST(VersionTwoSceneRejectsProjectCallbacksBeforeMutation) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[{"id":"aa","name":"Existing"}],"entities":[{"id":"a","sceneScope":"aa"}]})").ok);
    const Entity existing = ById(world).at(0xa);
    Entity generated = world.Create("unsaved runtime entity");
    generated.Add<Transform2D>().position = pxPoint(8.0, 9.0);
    world.GetRandom().NextU64();
    Random expected = world.GetRandom();
    const std::string before = WriteScene(world, types);
    ThrowOnLoad::attempts = 0;
    ThrowOnLoad::failureAt = 2;
    ThrowOnLoad::consumeFrom = &world;
    const SceneReport result = ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":99,
        "sceneScopes":[{"id":"bb","name":"New"}],
        "entities":[{"id":"b","sceneScope":"bb","components":{"ThrowOnLoad":{}}},
                    {"id":"c","sceneScope":"bb","components":{"ThrowOnLoad":{}}}]})");
    ThrowOnLoad::consumeFrom = nullptr;
    ThrowOnLoad::failureAt = 0;
    CHECK(!result.ok && result.error.find("ThrowOnLoad") != std::string::npos);
    CHECK(ThrowOnLoad::attempts == 0);
    CHECK(world.EntityCount() == 2 && world.Scopes().size() == 1);
    CHECK(ById(world).at(0xa).Id() == existing.Id());
    CHECK(world.Registry().valid(generated.Id()) && generated.Get<Transform2D>().position == pxPoint(8.0, 9.0));
    CHECK(WriteScene(world, types) == before);
    CHECK(world.GetRandom().NextU64() == expected.NextU64());
}

TEST(VersionTwoFragmentRejectsProjectCallbacksBeforeMutation) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[{"id":"aa","name":"Existing"}],"entities":[{"id":"a","sceneScope":"aa"}]})").ok);
    world.GetRandom().NextU64();
    Random expected = world.GetRandom();
    const std::string before = WriteScene(world, types);
    ThrowOnLoad::attempts = 0;
    ThrowOnLoad::failureAt = 2;
    ThrowOnLoad::consumeFrom = &world;
    const EntitiesReport result = ReadEntities(world, types, R"({"format":"pixie-entities","version":2,
        "sceneScopes":[{"id":"bb","name":"New"}],
        "entities":[{"id":"b","sceneScope":"bb","components":{"ThrowOnLoad":{}}},
                    {"id":"c","sceneScope":"bb","components":{"ThrowOnLoad":{}}}]})", SceneIds::New);
    ThrowOnLoad::consumeFrom = nullptr;
    ThrowOnLoad::failureAt = 0;
    CHECK(!result.ok && result.entities.empty());
    CHECK(result.error.find("ThrowOnLoad") != std::string::npos);
    CHECK(ThrowOnLoad::attempts == 0);
    CHECK(world.EntityCount() == 1 && world.Scopes().size() == 1);
    CHECK(WriteScene(world, types) == before);
    CHECK(world.GetRandom().NextU64() == expected.NextU64());
}

TEST(VersionTwoAdmissionListsAllProjectTypesAndPreservesExistingState) {
    const ComponentTypes types = Types();
    for (const bool fragment : {false, true}) {
        World world;
        CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
            "sceneScopes":[],"entities":[{"id":"a","components":{"Transform2D":{"position":[1,2]}}}]})").ok);
        Entity existing = ById(world).at(0xa);
        ThrowOnLoad::attempts = 0;
        ThrowOnLoad::failureAt = 1;
        ThrowOnLoad::existingToMutate = &existing;
        ThrowOnLoad::consumeFrom = &world;
        ThrowOnLoad::externalEffects = 0;
        Random expected = world.GetRandom();
        const std::string before = WriteScene(world, types);
        const std::string payload = fragment
            ? R"({"format":"pixie-entities","version":2,"sceneScopes":[],
                  "entities":[{"id":"b","components":{"ThrowOnLoad":{},"Spinner":{},"FutureType":{"value":1}}}]})"
            : R"({"format":"pixie-scene","version":2,"seed":99,"sceneScopes":[],
                  "entities":[{"id":"b","components":{"ThrowOnLoad":{},"Spinner":{},"FutureType":{"value":1}}}]})";
        const std::string error = fragment ? ReadEntities(world, types, payload, SceneIds::New).error
                                           : ReadScene(world, types, payload).error;
        ThrowOnLoad::existingToMutate = nullptr;
        ThrowOnLoad::consumeFrom = nullptr;
        ThrowOnLoad::failureAt = 0;
        CHECK(error.find("Spinner, ThrowOnLoad") != std::string::npos);
        CHECK(error.find("FutureType") == std::string::npos);
        CHECK(world.EntityCount() == 1 && existing.IsAlive());
        CHECK(existing.Get<Transform2D>().position == pxPoint(1.0, 2.0));
        CHECK(ThrowOnLoad::attempts == 0 && ThrowOnLoad::externalEffects == 0);
        CHECK(WriteScene(world, types) == before);
        CHECK(world.GetRandom().NextU64() == expected.NextU64());
    }
}

TEST(EntityPreflightValidatesProspectiveReplacementWithoutMutation) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[],"entities":[{"id":"a","components":{"PhysicsSettings2D":{}}},
        {"id":"b","components":{"Transform2D":{"position":[1,2]}}}]})").ok);
    const std::string before = WriteScene(world, types);
    Random expected = world.GetRandom();
    const auto existing = ById(world).at(0xb);
    const std::string singleton = R"({"format":"pixie-entities","version":2,"sceneScopes":[],
        "entities":[{"id":"a","components":{"PhysicsSettings2D":{}}}]})";
    CHECK(ValidateEntities(world, types, singleton, SceneIds::Keep, std::array<std::uint64_t, 1>{0xa}).ok);
    CHECK(!ValidateEntities(world, types, singleton, SceneIds::Keep).ok);
    CHECK(!ValidateEntities(world, types, singleton, SceneIds::New).ok);
    const std::string ordinary = R"({"format":"pixie-entities","version":2,"sceneScopes":[],
        "entities":[{"id":"b","components":{"Transform2D":{}}}]})";
    CHECK(ValidateEntities(world, types, ordinary, SceneIds::New).ok);
    CHECK(!ValidateEntities(world, types, ordinary, SceneIds::Keep).ok);
    CHECK(!ValidateEntities(world, types, ordinary, SceneIds::New, std::array<std::uint64_t, 1>{0xb}).ok);
    CHECK(!ValidateEntities(world, types, R"({"format":"pixie-entities","version":2,
        "sceneScopes":[],"entities":[{"id":"c","components":{"Spinner":{}}}]})", SceneIds::Keep).ok);
    CHECK(!ValidateEntities(world, types, R"({"format":"pixie-entities","version":2,
        "sceneScopes":[],"entities":[{"id":"0"}]})", SceneIds::Keep).ok);
    CHECK(existing.IsAlive() && existing.Get<Transform2D>().position == pxPoint(1, 2));
    CHECK(WriteScene(world, types) == before && world.GetRandom().NextU64() == expected.NextU64());
    World legacy;
    CHECK(legacy.Registry().on_construct<SceneId>().empty());
    const std::string permissive = R"({"format":"pixie-entities","version":1,
        "entities":[{"id":"a","components":{"Transform2D":{"position":"bad"}}}]})";
    CHECK(ValidateEntities(legacy, types, permissive, SceneIds::Keep).ok);
    CHECK(legacy.Registry().on_construct<SceneId>().empty());
    CHECK(ReadEntities(legacy, types, permissive, SceneIds::Keep).ok);
    CHECK(nlohmann::json::parse(WriteScene(legacy, types))["version"] == 1);
}

TEST(ReplacementPreflightRetainsScopeAndLightingConflicts) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[{"id":"aa","name":"Original"}],
        "entities":[{"id":"a","components":{"LightingSettings2D":{}}}]})").ok);
    const auto before = WriteScene(world, types);
    const std::string lighting = R"({"format":"pixie-entities","version":2,"sceneScopes":[],
        "entities":[{"id":"b","components":{"LightingSettings2D":{}}}]})";
    CHECK(ValidateEntities(world, types, lighting, SceneIds::Keep, std::array<std::uint64_t, 1>{0xa}).ok);
    CHECK(!ValidateEntities(world, types, lighting, SceneIds::Keep).ok);
    const std::string conflict = R"({"format":"pixie-entities","version":2,
        "sceneScopes":[{"id":"aa","name":"Conflict"}],"entities":[{"id":"b","sceneScope":"aa"}]})";
    CHECK(!ValidateEntities(world, types, conflict, SceneIds::Keep, std::array<std::uint64_t, 1>{0xa}).ok);
    CHECK(WriteScene(world, types) == before);
}

TEST(ReplacementPreflightUsesDefaultUnitsAfterExcludingSettingsOwner) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[{"id":"aa","name":"Room","gravity":{"mode":"override","value":[100000000,0]}}],
        "entities":[{"id":"a","components":{"PhysicsSettings2D":{"unitsPerMeter":1000}}}]})").ok);
    const auto before = WriteScene(world, types);
    const std::string partial = R"({"format":"pixie-entities","version":2,"sceneScopes":[],
        "entities":[{"id":"a","components":{"PhysicsSettings2D":{"gravity":[0,0]}}}]})";
    CHECK(!ValidateEntities(world, types, partial, SceneIds::Keep, std::array<std::uint64_t, 1>{0xa}).ok);
    CHECK(WriteScene(world, types) == before);
}

TEST(VersionTwoSnapshotAdmissionChecksTheLiveWorldWithoutWritingIt) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[],"entities":[{"id":"a"}]})").ok);
    Entity existing = ById(world).at(0xa);
    existing.Add<Spinner>();
    Random expected = world.GetRandom();
    CHECK(ValidateV2SceneSnapshot(world, types).find("Spinner") != std::string::npos);
    CHECK(world.EntityCount() == 1 && existing.Has<Spinner>());
    CHECK(world.GetRandom().NextU64() == expected.NextU64());
    bool rejectedWrite = false;
    try { (void)WriteScene(world, types); }
    catch (const std::runtime_error& error) {
        rejectedWrite = std::string_view(error.what()).find("Spinner") != std::string_view::npos;
    }
    CHECK(rejectedWrite && world.EntityCount() == 1 && existing.Has<Spinner>());
    rejectedWrite = false;
    try { (void)WriteEntities(world, types, std::array{existing}); }
    catch (const std::runtime_error& error) {
        rejectedWrite = std::string_view(error.what()).find("Spinner") != std::string_view::npos;
    }
    CHECK(rejectedWrite && world.EntityCount() == 1 && existing.Has<Spinner>());

    World legacy;
    Entity legacyEntity = legacy.Create();
    legacyEntity.Add<SceneId>(SceneId{0xa});
    legacyEntity.Add<Spinner>();
    CHECK(ValidateV2SceneSnapshot(legacy, types).empty());
    CHECK(WriteEntities(legacy, types, std::array{legacyEntity}).find("Spinner") != std::string::npos);
}

TEST(VersionTwoRuntimeComponentHookShowsPartialRollbackBoundary) {
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "sceneScopes":[],"entities":[{"id":"a","components":{"Transform2D":{"position":[1,2]}}}]})").ok);
    Entity existing = ById(world).at(0xa);
    Random expected = world.GetRandom();
    hookWorld = &world;
    hookExisting = &existing;
    hookExternalEffects = 0;
    world.Registry().on_construct<Transform2D>().connect<&ThrowFromRuntimeComponentHook>();
    const EntitiesReport result = ReadEntities(world, types, R"({"format":"pixie-entities","version":2,
        "sceneScopes":[{"id":"bb","name":"Temporary"}],
        "entities":[{"id":"b","sceneScope":"bb","components":{"Transform2D":{"position":[3,4]}}}]})",
        SceneIds::Keep);
    world.Registry().on_construct<Transform2D>().disconnect<&ThrowFromRuntimeComponentHook>();
    hookWorld = nullptr;
    hookExisting = nullptr;
    CHECK(!result.ok && result.entities.empty());
    CHECK(result.error.find("intentional runtime component hook failure") != std::string::npos);
    CHECK(world.EntityCount() == 1 && world.Scopes().empty() && existing.IsAlive());
    CHECK(world.GetRandom().NextU64() == expected.NextU64());
    CHECK(existing.Get<Transform2D>().position == pxPoint(777.0, 888.0));
    CHECK(hookExternalEffects == 1);
}

TEST(VersionTwoFragmentProposalPreservesOutsideReferencesOrderAndLiveRandom) {
    const auto types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":43,
        "futureRoot":{"value":17},"sceneScopes":[{"id":"aa","name":"Room"},{"id":"bb","name":"Empty"}],
        "entities":[{"id":"a","sceneScope":"aa","components":{"ParticleSystem":{"v":1},"Transform2D":{"position":[1,2]}}},
        {"id":"b","components":{"ParticleSpawner":{"v":1,"system":"a"},"FutureOpaque":{"target":"a"}}}]})").ok);
    auto existing = ById(world).at(0xa);
    const std::array selected{existing};
    const auto fragment = WriteEntities(world, types, selected);
    world.GetRandom().NextU64();
    auto expected = world.GetRandom();
    const auto before = WriteScene(world, types);
    hookWorld = &world; hookExisting = &existing; hookExternalEffects = 0;
    world.Registry().on_construct<Transform2D>().connect<&ThrowFromRuntimeComponentHook>();
    const std::array<std::uint64_t, 1> replacing{0xa};
    const auto plan = PrepareSceneFragment(world, types, fragment, SceneIds::Keep, replacing, 16, 16);
    world.Registry().on_construct<Transform2D>().disconnect<&ThrowFromRuntimeComponentHook>();
    hookWorld = nullptr; hookExisting = nullptr;
    CHECK(plan.ok && hookExternalEffects == 0);
    CHECK(WriteScene(world, types) == before);
    CHECK(world.GetRandom().NextU64() == expected.NextU64());
    const auto proposal = nlohmann::json::parse(plan.text);
    const auto& doc = proposal["document"];
    CHECK(doc["entities"][0]["id"] == "000000000000000a");
    CHECK(doc["entities"][1]["components"].value("ParticleSpawner", nlohmann::json::object()).value("system", nlohmann::json{}) == "000000000000000a");
    CHECK(doc["entities"][1]["components"].value("FutureOpaque", nlohmann::json::object()).value("target", nlohmann::json{}) == "a");
    CHECK(doc["futureRoot"]["value"] == 17 && doc["sceneScopes"].size() == 2);
    World candidate;
    CHECK(ReadScene(candidate, types, doc.dump()).ok);
    const auto loaded = ById(candidate);
    CHECK(loaded.at(0xb).Get<ParticleSpawner>().system == loaded.at(0xa).Id());
    CHECK(loaded.at(0xa).Get<Transform2D>().position == pxPoint(17, 18));
    const std::array both{existing, ById(world).at(0xb)};
    const auto copied = PrepareSceneFragment(world, types, WriteEntities(world, types, both), SceneIds::New);
    CHECK(copied.ok);
    const auto copyPlan = nlohmann::json::parse(copied.text);
    CHECK(copyPlan["ids"].size() == 2 && copyPlan["ids"][0] != "000000000000000a");
    CHECK(copyPlan["document"]["entities"][3]["components"].value("ParticleSpawner", nlohmann::json::object()).value("system", nlohmann::json{}) == copyPlan["ids"][0]);
}

TEST(VersionTwoFragmentProposalRejectsBeforeLiveCallbacksAndKeepsDeletionPure) {
    const auto types = Types();
    World world;
    CHECK(ReadScene(world, types, R"({"format":"pixie-scene","version":2,"seed":3,"sceneScopes":[],
        "entities":[{"id":"a","components":{"Transform2D":{"position":[1,2]}}}]})").ok);
    const auto before = WriteScene(world, types);
    const auto denied = PrepareSceneFragment(world, types, R"({"format":"pixie-entities","version":2,
        "sceneScopes":[],"entities":[{"id":"b","components":{"Spinner":{"v":1}}}]})", SceneIds::Keep);
    CHECK(!denied.ok && denied.error.find("Spinner") != std::string::npos);
    const auto legacyDenied = PrepareSceneFragment(world, types, R"({"format":"pixie-entities","version":1,
        "entities":[{"id":"b","components":{"Spinner":{"v":1}}}]})", SceneIds::Keep);
    CHECK(!legacyDenied.ok && legacyDenied.error.find("Spinner") != std::string::npos);
    const std::array<std::uint64_t, 1> removed{0xa};
    const auto deletion = PrepareSceneFragment(world, types, "", SceneIds::Keep, removed);
    CHECK(deletion.ok && nlohmann::json::parse(deletion.text)["document"]["entities"].empty());
    CHECK(WriteScene(world, types) == before && world.EntityCount() == 1);
}

TEST(VersionOneOpaquePresentationCannotBecomeAnActiveAuthorityDuringConversion) {
    const auto types = Types();
    World empty;
    auto opaque = nlohmann::json::parse(PresentationSnapshot2D(empty))["presentation"];
    opaque["lightingState"] = "configured";
    const auto source = nlohmann::json{{"format", "pixie-scene"}, {"version", 1}, {"seed", 31},
        {"presentation", opaque}, {"entities", nlohmann::json::array()}}.dump();
    const auto folder = std::filesystem::temp_directory_path() /
        ("pixie-opaque-presentation-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(folder);
    const auto file = folder / "start.scene";
    CHECK(SaveTextFile(source, file).ok);
    World world;
    CHECK(ReadScene(world, types, source).ok && !world.Presentation());
    CHECK(nlohmann::json::parse(WriteScene(world, types))["presentation"] == opaque);
    const auto report = ConvertSceneFileV1ToV2(world, types, file);
    CHECK(!report.ok && report.error.find("presentation") != std::string::npos);
    CHECK(SceneFormatVersion(world) == 1 && !world.Presentation());
    std::ifstream in(file, std::ios::binary);
    const std::string preserved{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()}; in.close();
    CHECK(preserved == source && !std::filesystem::exists(file.string() + ".v1-backup"));
    std::filesystem::remove(file.string() + ".v1-backup");
    std::filesystem::remove(file); std::filesystem::remove(folder);
}

TEST(RejectedVersionTwoFileReadLeavesSourceUntouched) {
    const ComponentTypes types = Types();
    const std::string source = R"({"format":"pixie-scene","version":2,"seed":99,
        "sceneScopes":[],"entities":[{"id":"b","components":{"ThrowOnLoad":{}}}]})";
    const auto path = std::filesystem::temp_directory_path() /
        ("pixie-v2-refusal-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".scene");
    { std::ofstream out(path, std::ios::binary); out << source; }
    World world(43);
    ThrowOnLoad::attempts = 0;
    const SceneReport result = LoadSceneFile(world, types, path);
    std::ifstream in(path, std::ios::binary);
    std::ostringstream after;
    after << in.rdbuf();
    in.close();
    CHECK(!result.ok && result.error.find("ThrowOnLoad") != std::string::npos);
    CHECK(ThrowOnLoad::attempts == 0 && world.EntityCount() == 0 && world.GetRandom().Seed() == 43);
    CHECK(after.str() == source);
    std::filesystem::remove(path);
}

TEST(FragmentImportsRefuseDuplicateSettingsBeforeMutation) {
    const ComponentTypes types = Types();
    for (const std::string kind : {"PhysicsSettings2D", "LightingSettings2D"}) {
        World world(47);
        if (kind == "PhysicsSettings2D") world.Create().Add<PhysicsSettings2D>();
        else world.Create().Add<LightingSettings2D>();
        const std::string fragment = std::string(R"({"format":"pixie-entities","version":1,
            "entities":[{"id":"a","components":{")") + kind + R"(":{}}}]})";
        const EntitiesReport result = ReadEntities(world, types, fragment, SceneIds::Keep);
        CHECK(!result.ok && result.entities.empty());
        CHECK(result.error.find(kind) != std::string::npos);
        CHECK(world.EntityCount() == 1 && world.GetRandom().Seed() == 47);

        // Even an unrelated fragment must not pass through an already ambiguous target.
        if (kind == "PhysicsSettings2D") world.Create().Add<PhysicsSettings2D>();
        else world.Create().Add<LightingSettings2D>();
        const EntitiesReport unrelated = ReadEntities(world, types,
            R"({"format":"pixie-entities","version":1,"entities":[{"id":"b"}]})", SceneIds::Keep);
        CHECK(!unrelated.ok && unrelated.entities.empty());
        CHECK(world.EntityCount() == 2 && world.GetRandom().Seed() == 47);
    }
}

TEST(VersionOneWholeSceneReadRetainsLegacyDuplicateSettings) {
    const ComponentTypes types = Types();
    World world;
    const SceneReport read = ReadScene(world, types, R"({"format":"pixie-scene","version":1,
        "seed":9,"entities":[{"id":"a","components":{"PhysicsSettings2D":{}}},
        {"id":"b","components":{"PhysicsSettings2D":{}}}]})");
    CHECK(read.ok && world.EntityCount() == 2 && world.GetRandom().Seed() == 9);
}

TEST(AWrongValueWarnsAndKeepsTheDefault) {
    const std::string text = R"({"format": "pixie-scene", "version": 1, "seed": 0, "entities": [
        {"id": "1", "components": {
            "Transform2D": {"v": 1, "position": "here", "rotation": 1.5},
            "Shape": {"v": 1, "kind": "Hexagon"},
            "ParticleSpawner": {"v": 1, "system": "00000000000000ff"},
            "Attractor": {"v": 9}}}]})";
    const ComponentTypes types = Types();
    World world;
    const SceneReport report = ReadScene(world, types, text);
    CHECK(report.ok);
    CHECK(report.warnings.size() == 4);
    Entity entity;
    world.Each<const Transform2D>([&](Entity e, const Transform2D&) { entity = e; });
    CHECK(entity);
    CHECK(entity.Get<Transform2D>().position == pxPoint(0.0, 0.0));
    CHECK(entity.Get<Transform2D>().rotation == 1.5);
    CHECK(entity.Get<Shape>().kind == ShapeKind::Circle);
    CHECK(entity.Get<ParticleSpawner>().system == entt::null);
}

TEST(AReferenceOutsideTheSceneIsSavedAsNone) {
    const ComponentTypes types = Types();
    World world;
    Entity pool = world.Create("generated pool");
    pool.Add<ParticleSystem>();
    Entity spawner = world.Create("spawner");
    spawner.Add<ParticleSpawner>().system = pool.Id();
    AddToScene(spawner);
    std::vector<std::string> warnings;
    const auto document = nlohmann::json::parse(WriteScene(world, types, &warnings));
    CHECK(warnings.size() == 1);
    CHECK(document["entities"][0]["components"]["ParticleSpawner"]["system"].is_null());
}

TEST(DescriptorsAddAndRemoveTheirComponent) {
    const ComponentTypes types = Types();
    World world;
    Entity entity = world.Create();
    const ComponentDescriptor& transform = types.All()[static_cast<std::size_t>(types.IndexOf("Transform2D"))];
    void* added = transform.add(world, entity.Id());
    CHECK(added != nullptr && entity.Has<Transform2D>());
    static_cast<Transform2D*>(added)->position = pxPoint(4.0, 5.0);
    // Adding again returns the one already there.
    CHECK(transform.add(world, entity.Id()) == added);
    CHECK(entity.Get<Transform2D>().position == pxPoint(4.0, 5.0));
    transform.remove(world, entity.Id());
    CHECK(!entity.Has<Transform2D>());

    const ComponentDescriptor& spinner = types.All()[static_cast<std::size_t>(types.IndexOf("Spinner"))];
    CHECK(spinner.add(world, entity.Id()) != nullptr);
    world.Step(0.1, kNoInput);
    CHECK(entity.Get<Spinner>().turns == 1);
    spinner.remove(world, entity.Id());
    CHECK(!entity.Has<Spinner>());

    entity.Destroy();
    CHECK(transform.add(world, entity.Id()) == nullptr);
}

TEST(AddToSceneKeepsAnExistingId) {
    World world;
    Entity entity = world.Create();
    const std::uint64_t id = AddToScene(entity);
    CHECK(id != 0);
    CHECK(AddToScene(entity) == id);
    CHECK(NewSceneId() != NewSceneId());
    entity.Destroy();
    CHECK(AddToScene(entity) == 0);
}

TEST(SavingAndLoadingAFile) {
    const ComponentTypes types = Types();
    World world(11);
    Fill(world);
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "pixie-scene-tests";
    std::filesystem::create_directories(directory);
    const std::filesystem::path path = directory / "main.scene";
    CHECK(SaveSceneFile(world, types, path).ok);
    // Saving again replaces the file.
    CHECK(SaveSceneFile(world, types, path).ok);
    CHECK(!std::filesystem::exists(directory / "main.scene.saving"));

    World loaded;
    const SceneReport report = LoadSceneFile(loaded, types, path);
    CHECK(report.ok);
    CHECK(WriteScene(loaded, types) == WriteScene(world, types));
    CHECK(!LoadSceneFile(loaded, types, directory / "missing.scene").ok);
    std::filesystem::remove_all(directory);
}

TEST(VersionTwoSaveCannotImplicitlyConvertExistingVersionOneFile) {
    const ComponentTypes types = Types();
    const auto directory = std::filesystem::temp_directory_path() /
        ("pixie-save-version-" + std::to_string(NewSceneId()));
    std::filesystem::create_directories(directory);
    const auto path = directory / "main.scene";
    const std::string legacy = R"({"format":"pixie-scene","version":1,"seed":41,
        "futureRoot":{"preserve":true},"entities":[]})";
    const std::string current = R"({"format":"pixie-scene","version":2,"seed":41,
        "sceneScopes":[{"id":"aa","name":"Room"}],"entities":[]})";
    auto bytes = [&] {
        std::ifstream in(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    };
    CHECK(SaveTextFile(legacy, path).ok);
    World recovered;
    CHECK(ReadScene(recovered, types, current).ok);
    const SceneReport refused = SaveSceneFile(recovered, types, path);
    CHECK(!refused.ok && refused.error.find("explicit") != std::string::npos);
    CHECK(bytes() == legacy);
    CHECK(!std::filesystem::exists(directory / "main.scene.saving"));
    CHECK(!std::filesystem::exists(directory / "main.scene.v1-backup"));

    CHECK(SaveSceneFile(recovered, types, directory / "new.scene").ok);
    // An already-v2 destination remains writable through the admitted World writer.
    CHECK(SaveTextFile(current, path).ok);
    CHECK(SaveSceneFile(recovered, types, path).ok);
    CHECK(nlohmann::json::parse(bytes())["sceneScopes"][0]["id"] == "00000000000000aa");
    for (const auto& invalid : {std::string("{broken"),
            std::string(R"({"format":"pixie-scene","version":3,"entities":[]})")}) {
        CHECK(SaveTextFile(invalid, path).ok);
        CHECK(!SaveSceneFile(recovered, types, path).ok && bytes() == invalid);
    }
    // Explicit conversion is the only supported path across the existing v1 boundary.
    CHECK(SaveTextFile(legacy, path).ok);
    World source;
    CHECK(ReadScene(source, types, legacy).ok);
    std::string converted = "sentinel";
    CHECK(SaveTextFile("reserved", directory / "main.scene.v1-backup").ok);
    CHECK(!ConvertSceneFileV1ToV2(source, types, path, &converted).ok);
    CHECK(converted == "sentinel" && SceneFormatVersion(source) == 1 && bytes() == legacy);
    std::filesystem::remove(directory / "main.scene.v1-backup");
    CHECK(ConvertSceneFileV1ToV2(source, types, path, &converted).ok);
    CHECK(converted == bytes() && SceneFormatVersion(source) == 2);
    CHECK(SaveSceneFile(source, types, path).ok);
    std::ifstream backup(directory / "main.scene.v1-backup", std::ios::binary);
    CHECK(std::string(std::istreambuf_iterator<char>(backup), std::istreambuf_iterator<char>()) == legacy);
    backup.close();
    std::filesystem::remove_all(directory);
}

namespace {

Entity Named(World& world, const std::string& name) {
    Entity found;
    world.Each<const Name>([&](Entity entity, const Name& n) {
        if (n.value == name) {
            found = entity;
        }
    });
    return found;
}

} // namespace

TEST(ASnapshotBringsBackDestroyedEntities) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const std::array chosen = {Named(world, "pool"), Named(world, "spawner")};
    const std::uint64_t poolId = chosen[0].Get<SceneId>().value;
    const std::string snapshot = WriteEntities(world, types, chosen);
    for (const Entity entity : chosen) {
        entity.Destroy();
    }
    CHECK(world.EntityCount() == 2);

    const EntitiesReport report = ReadEntities(world, types, snapshot, SceneIds::Keep);
    CHECK(report.ok && report.warnings.empty() && report.entities.size() == 2);
    CHECK(world.EntityCount() == 4);
    const Entity pool = report.entities[0];
    const Entity spawner = report.entities[1];
    CHECK(pool.Get<SceneId>().value == poolId);
    CHECK(spawner.Get<ParticleSpawner>().system == pool.Id());
    CHECK(pool.Get<Attractor>().killOnContact && pool.Get<ParticleSystem>().capacity == 5000);
    // Written again, the entities are as they were.
    CHECK(WriteEntities(world, types, report.entities) == snapshot);
}

TEST(ACopyGetsNewIdsAndFollowsItsOwnReferences) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const Entity pool = Named(world, "pool");
    const Entity spawner = Named(world, "spawner");

    // Both copied: the copy of the spawner feeds the copy of the pool.
    const std::array both = {pool, spawner};
    const EntitiesReport pair = ReadEntities(world, types, WriteEntities(world, types, both), SceneIds::New);
    CHECK(pair.ok && pair.entities.size() == 2);
    const Entity poolCopy = pair.entities[0];
    const Entity spawnerCopy = pair.entities[1];
    CHECK(poolCopy.Get<SceneId>().value != pool.Get<SceneId>().value);
    CHECK(spawnerCopy.Get<SceneId>().value != spawner.Get<SceneId>().value);
    CHECK(spawnerCopy.Get<ParticleSpawner>().system == poolCopy.Id());
    CHECK(poolCopy.Get<Name>().value == "pool");

    // The spawner alone: its copy feeds the pool it fed.
    const std::array one = {spawner};
    const EntitiesReport single = ReadEntities(world, types, WriteEntities(world, types, one), SceneIds::New);
    CHECK(single.ok && single.entities.size() == 1);
    CHECK(single.entities[0].Get<ParticleSpawner>().system == pool.Id());

    // A copied Behaviour runs.
    const std::array spinners = {world.Wrap(*world.Registry().view<Spinner>().begin())};
    const EntitiesReport copied = ReadEntities(world, types, WriteEntities(world, types, spinners), SceneIds::New);
    CHECK(copied.ok && copied.entities.size() == 1 && copied.entities[0].Get<Spinner>().speed == 2.25);
    world.Step(0.1, kNoInput);
    CHECK(copied.entities[0].Get<Spinner>().turns == 1);
}

TEST(KeepingIdsThatAreInTheWorldIsRefused) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const std::array chosen = {Named(world, "pool")};
    const std::size_t before = world.EntityCount();
    const EntitiesReport report = ReadEntities(world, types, WriteEntities(world, types, chosen), SceneIds::Keep);
    CHECK(!report.ok && !report.error.empty() && report.entities.empty());
    CHECK(world.EntityCount() == before);
    CHECK(!ReadEntities(world, types, "{ not json", SceneIds::New).ok);
    CHECK(!ReadEntities(world, types, WriteScene(world, types), SceneIds::New).ok);
}

TEST(EntitiesOutsideTheSceneAreLeftOutOfASnapshot) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const std::array chosen = {Named(world, "pool"), Named(world, "generated")};
    std::vector<std::string> warnings;
    const auto document = nlohmann::json::parse(WriteEntities(world, types, chosen, &warnings));
    CHECK(document["format"] == "pixie-entities" && document["entities"].size() == 1);
    CHECK(warnings.size() == 1);
}

TEST(UnknownDataTravelsWithASnapshot) {
    const std::string text = R"({"format": "pixie-scene", "version": 1, "seed": 0, "entities": [
        {"id": "a1", "name": "odd", "layer": 2,
         "components": {"Transform2D": {"v": 1, "wobble": 5}, "Mystery": {"v": 3, "a": 1}}}]})";
    const ComponentTypes types = Types();
    World world;
    CHECK(ReadScene(world, types, text).ok);
    const std::array chosen = {Named(world, "odd")};
    const std::string snapshot = WriteEntities(world, types, chosen);
    const EntitiesReport copy = ReadEntities(world, types, snapshot, SceneIds::New);
    CHECK(copy.ok && copy.entities.size() == 1);
    const std::array copies = {copy.entities[0]};
    const auto written = nlohmann::json::parse(WriteEntities(world, types, copies))["entities"][0];
    CHECK(written["layer"] == 2);
    CHECK(written["components"]["Mystery"]["a"] == 1);
    CHECK(written["components"]["Transform2D"]["wobble"] == 5);
}

TEST(AFragmentKeepsItsMetaAndItIsReadAlone) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const std::array chosen = {Named(world, "pool"), Named(world, "spawner")};
    const std::size_t entities = world.EntityCount();
    std::vector<std::string> warnings;
    const std::string text =
        WriteEntities(world, types, chosen, R"({"name": "fountain", "made": [2026, 10, 15]})", &warnings);
    CHECK(warnings.empty());
    // The meta is written at the top, before the entities.
    const auto document = nlohmann::ordered_json::parse(text);
    CHECK(document["meta"]["name"] == "fountain" && document.begin().key() == "format");
    CHECK(text.find("\"meta\"") < text.find("\"entities\""));

    const FragmentMeta alone = ReadFragmentMeta(text);
    CHECK(alone.ok && alone.entityCount == 2 && world.EntityCount() == entities);
    CHECK(nlohmann::json::parse(alone.meta) == nlohmann::json::parse(R"({"name": "fountain", "made": [2026, 10, 15]})"));

    const EntitiesReport copy = ReadEntities(world, types, text, SceneIds::New);
    CHECK(copy.ok && copy.entities.size() == 2 && copy.meta == alone.meta);

    // Without meta, nothing is written and nothing is read back.
    const std::string bare = WriteEntities(world, types, chosen);
    CHECK(nlohmann::json::parse(bare).contains("meta") == false);
    const FragmentMeta none = ReadFragmentMeta(bare);
    CHECK(none.ok && none.meta.empty() && none.entityCount == 2);
}

TEST(MetaThatIsNotAnObjectIsLeftOut) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const std::array chosen = {Named(world, "pool")};
    for (const char* meta : {"[1, 2]", "\"name\"", "{not json"}) {
        std::vector<std::string> warnings;
        const std::string text = WriteEntities(world, types, chosen, meta, &warnings);
        CHECK(warnings.size() == 1 && !nlohmann::json::parse(text).contains("meta"));
    }
    // A document whose meta is not an object reads, with a warning.
    auto document = nlohmann::json::parse(WriteEntities(world, types, chosen));
    document["meta"] = 7;
    const FragmentMeta read = ReadFragmentMeta(document.dump());
    CHECK(read.ok && read.meta.empty() && read.warnings.size() == 1 && read.entityCount == 1);
    // What is not a set of entities fails, as ReadEntities does.
    CHECK(!ReadFragmentMeta("{}").ok && !ReadFragmentMeta(WriteScene(world, types)).ok && !ReadFragmentMeta("x").ok);
}

TEST(TheWorldsSceneIdsFollowItsEntities) {
    const ComponentTypes types = Types();
    World world;
    Fill(world);
    const Entity pool = Named(world, "pool");
    const Entity spawner = Named(world, "spawner");
    const std::uint64_t poolId = pool.Get<SceneId>().value;
    const std::array pools = {pool};
    const std::array spawners = {spawner};
    const std::string poolSet = WriteEntities(world, types, pools);
    const std::string spawnerSet = WriteEntities(world, types, spawners);
    // The first read finds the World's scene entities; later ones follow what changes.
    CHECK(!ReadEntities(world, types, poolSet, SceneIds::Keep).ok);

    // Destroyed, the pool comes back under its ID, and a copy of the spawner feeds it.
    pool.Destroy();
    const EntitiesReport back = ReadEntities(world, types, poolSet, SceneIds::Keep);
    CHECK(back.ok && back.entities.size() == 1);
    // What follows reads the entities a read added; without them it stops, not reading past the end.
    if (back.entities.size() != 1) {
        return;
    }
    CHECK(!ReadEntities(world, types, poolSet, SceneIds::Keep).ok);
    const EntitiesReport feeder = ReadEntities(world, types, spawnerSet, SceneIds::New);
    CHECK(feeder.ok && feeder.entities.size() == 1 &&
          feeder.entities[0].Get<ParticleSpawner>().system == back.entities[0].Id());

    // Without its SceneId, the spawner leaves the scene, and its ID is free.
    spawner.Remove<SceneId>();
    CHECK(ReadEntities(world, types, spawnerSet, SceneIds::Keep).ok);

    // A replaced ID: the old one is free, the new one taken.
    const Entity moved = back.entities[0];
    moved.Add<SceneId>(SceneId{NewSceneId()});
    const EntitiesReport again = ReadEntities(world, types, poolSet, SceneIds::Keep);
    CHECK(again.ok && again.entities.size() == 1);
    if (again.entities.size() != 1) {
        return;
    }
    const std::array movedOnly = {moved};
    const std::string movedSet = WriteEntities(world, types, movedOnly);
    CHECK(!ReadEntities(world, types, movedSet, SceneIds::Keep).ok);
    moved.Destroy();
    CHECK(ReadEntities(world, types, movedSet, SceneIds::Keep).ok);

    // An ID given by hand to a second entity: when one of them goes, the other still holds it.
    const Entity twin = world.Create("twin");
    twin.Add<SceneId>(SceneId{poolId});
    again.entities[0].Destroy();
    CHECK(!ReadEntities(world, types, poolSet, SceneIds::Keep).ok);
    twin.Destroy();
    CHECK(ReadEntities(world, types, poolSet, SceneIds::Keep).ok);
}

namespace {

// Reads a set of a few entities `reads` times into a World of `size` scene entities, and returns how
// long the reads took, after a first read.
double SecondsToRead(std::size_t size, int reads) {
    const ComponentTypes types = Types();
    World world;
    std::vector<Entity> chosen;
    for (int i = 0; i < 4; ++i) {
        Entity entity = world.Create("piece");
        entity.Add<Transform2D>();
        entity.Add<Shape>();
        AddToScene(entity);
        chosen.push_back(entity);
    }
    for (std::size_t i = 0; i < size; ++i) {
        AddToScene(world.Create());
    }
    const std::string text = WriteEntities(world, types, chosen);
    CHECK(ReadEntities(world, types, text, SceneIds::New).ok);

    bool ok = true;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < reads; ++i) {
        ok = ReadEntities(world, types, text, SceneIds::New).ok && ok;
    }
    const std::chrono::duration<double> taken = std::chrono::steady_clock::now() - start;
    CHECK(ok);
    return taken.count();
}

} // namespace

TEST(ReadingASetCostsWhatItHoldsNotTheWorldsSize) {
    const double small = SecondsToRead(1'000, 200);
    const double large = SecondsToRead(100'000, 200);
    std::fprintf(stderr, "200 reads of 4 entities: %.3f s into 1,000 scene entities, %.3f s into 100,000\n", small,
                 large);
    // When each read went through every scene entity, the large World's reads took about a hundred
    // times as long; the margin is for a busy machine.
    CHECK(large < 3.0 * small + 0.05);
}
