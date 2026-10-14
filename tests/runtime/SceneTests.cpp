#include "Check.hpp"

#include "pixie/runtime/Acceleration2D.hpp"
#include "pixie/runtime/Attractor.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Name.hpp"
#include "pixie/runtime/ParticleSpawner.hpp"
#include "pixie/runtime/ParticleSystem.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

using namespace pixie;

namespace {

const Input kNoInput;

// A Behaviour declared with fields, saved like any component.
struct Spinner : Behaviour {
    double speed = 1.0;
    int turns = 0;
    void Update(const FrameTime&, const Input&) override { ++turns; }
};

ComponentTypes Types() {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    types.Add<Spinner>("Spinner").Field<&Spinner::speed>("speed").Field<&Spinner::turns>("turns");
    return types;
}

// A World with every built-in component, a Behaviour, an Entity field, and one generated entity.
void Fill(World& world) {
    Entity pool = world.Create("pool");
    pool.Add<Transform2D>(Transform2D{pxPoint(12.5, -3.0), 0.75, pxVec2(2.0, 0.5)});
    pool.Add<Velocity2D>(Velocity2D{pxVec2(1.0, 2.0), 0.1});
    pool.Add<Acceleration2D>(Acceleration2D{pxVec2(-4.0, 9.81)});
    pool.Add<Shape>(Shape{ShapeKind::Ring, pxVec2(3.0, 4.0), pxColorRgba(10, 20, 30, 40)});
    pool.Add<ParticleSystem>().capacity = 5000;
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

    // Writing the loaded World gives the same text.
    CHECK(WriteScene(loaded, types) == text);
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
