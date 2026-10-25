#include "Check.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

using namespace pixie;

TEST(CollisionLayerNamesHaveFixedSlotsAndDoNotCreatePhysicsOrGravitySettings) {
    World world;
    auto names = DescribeCollisionLayers2D(world);
    CHECK(names.errors.empty() && names.names[0] == "Default");
    CHECK(names.names[30] == "Unnamed layer 31");
    auto& records = world.Create().Add<CollisionLayers2D>().layers;
    records = {{{31, "Debug"}, {}}, {{2, "Props"}, {}}, {{1, "World"}, {}}};
    names = DescribeCollisionLayers2D(world);
    CHECK(names.errors.empty() && names.names[30] == "Debug" && names.names[1] == "Props");
    std::reverse(records.begin(), records.end());
    CHECK(DescribeCollisionLayers2D(world).names == names.names);
    records[0].value.name.clear();
    CHECK(DescribeCollisionLayers2D(world).names[0] == "Unnamed layer 1");
    CHECK(world.Registry().view<PhysicsSettings2D>().empty());
    InstallSceneMovement2D(world);
    SyncSceneMovement2D(world);
    CHECK(SceneCollisionWorld2D(world) == nullptr);
}

TEST(CollisionLayerMetadataExposesCheckboxesAndNonAddableRecordValues) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World world;
    auto entity = world.Create();
    const auto& item = types.All()[types.IndexOf("CollisionLayer2D")];
    CHECK(item.valueType && item.add == nullptr && !item.AdditionError(world, entity.Id()).empty());
    const auto& config = types.All()[types.IndexOf("CollisionLayers2D")];
    CHECK(config.singleton != nullptr && config.fields[0].kind == FieldKind::RecordList);
    CHECK(config.fields[0].metadata.editor == FieldEditor::LayerNames);
    entity.Add<CollisionLayers2D>();
    CHECK(!config.AdditionError(world, world.Create().Id()).empty());
    const auto& collider = types.All()[types.IndexOf("Collider2D")];
    int masks = 0;
    for (const auto& field : collider.fields) if (field.name == "category" || field.name == "mask") {
        CHECK(field.kind == FieldKind::Int && field.metadata.editor == FieldEditor::LayerMask);
        CHECK(field.metadata.references == "CollisionLayers2D");
        ++masks;
    }
    CHECK(masks == 2);
    // An accidental native ECS instance of a record value is not authored as a root component.
    entity.Add<CollisionLayer2D>().name = "not a root component";
    AddToScene(entity);
    auto document = nlohmann::json::parse(WriteScene(world, types));
    CHECK(!document["entities"][0]["components"].contains("CollisionLayer2D"));
}

TEST(CollisionLayerNamesAndUnknownItemDataRoundTripWithoutRewritingLegacyFilters) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World source;
    auto names = source.Create("names");
    names.Add<CollisionLayers2D>().layers = {{{31, "Diagnostics"}, R"({"futureStyle":"cyan"})"}, {{1, "World"}, {}}};
    AddToScene(names);
    auto body = source.Create("legacy collider");
    body.Add<Transform2D>();
    auto& collider = body.Add<Collider2D>();
    collider.category = 0x40000001;
    collider.mask = 0x40000000;
    AddToScene(body);
    auto compound = source.Create("compound");
    compound.Add<Transform2D>();
    compound.Add<CompoundCollider2D>().shapes[0].value.category = 0x40000000;
    AddToScene(compound);
    World loaded;
    CHECK(ReadScene(loaded, types, WriteScene(source, types)).ok);
    CHECK(DescribeCollisionLayers2D(loaded).names[30] == "Diagnostics");
    auto config = loaded.Registry().view<CollisionLayers2D>();
    config.get<CollisionLayers2D>(*config.begin()).layers[0].value.name.clear();
    const auto document = nlohmann::json::parse(WriteScene(loaded, types));
    for (const auto& entry : document["entities"]) {
        const auto& components = entry["components"];
        if (entry["name"] == "names") CHECK(components["CollisionLayers2D"]["layers"][0]["futureStyle"] == "cyan");
        if (entry["name"] == "legacy collider") {
            CHECK(components["Collider2D"]["category"] == 0x40000001);
            CHECK(components["Collider2D"]["mask"] == 0x40000000);
        }
        if (entry["name"] == "compound") CHECK(components["CompoundCollider2D"]["shapes"][0]["category"] == 0x40000000);
    }
    CHECK(DescribeCollisionLayers2D(loaded).names[30] == "Unnamed layer 31");
}

TEST(InvalidCollisionLayerDefinitionsAreDiagnosedWithoutDiscardingBits) {
    World world;
    auto& config = world.Create().Add<CollisionLayers2D>();
    config.layers = {{{2, "A"}, {}}, {{2, "B"}, {}}, {{0, "bad"}, {}}, {{32, "bad"}, {}}};
    auto names = DescribeCollisionLayers2D(world);
    CHECK(names.names[1] == "Unnamed layer 2" && names.errors.size() == 3);
    CHECK(SceneMovementErrors2D(world).size() == 3);
    config.layers.erase(config.layers.begin());
    CHECK(DescribeCollisionLayers2D(world).names[1] == "B");
    world.Create().Add<CollisionLayers2D>();
    names = DescribeCollisionLayers2D(world);
    CHECK(names.errors.size() == 1 && names.names[0] == "Unnamed layer 1");
    CHECK(config.layers[0].value.slot == 2 && config.layers[0].value.name == "B");
}

TEST(CollisionLayerRenameDoesNotRebuildBodiesOrDisturbBilateralHighBitQueriesAndEvents) {
    World world;
    world.Create().Add<PhysicsSettings2D>().gravity = {};
    auto& names = world.Create().Add<CollisionLayers2D>().layers;
    names = {{{31, "Visitor"}, {}}, {{1, "Trigger"}, {}}};
    auto body = world.Create();
    body.Add<Transform2D>();
    body.Add<RigidBody2D>();
    auto& shape = body.Add<CompoundCollider2D>().shapes[0].value;
    shape.halfSize = {8, 8}; shape.category = 0x40000000; shape.mask = 1;
    auto sensor = world.Create();
    sensor.Add<Transform2D>();
    auto& region = sensor.Add<Collider2D>();
    region.sensor = true; region.category = 1; region.mask = 0x40000000;
    InstallSceneMovement2D(world);
    world.Step(1.0 / 60, {});
    const auto handle = SceneBody2D(world, body.Id());
    CHECK(ScenePhysicsEvents2D(world).size() == 1);
    auto* physics = ScenePhysicsWorld2D(world);
    CHECK(physics->RayCast({-20, 0}, {40, 0}, {1, 0x40000000}).has_value());
    CHECK(!physics->RayCast({-20, 0}, {40, 0}, {2, 0x40000000}));
    CHECK(!physics->RayCast({-20, 0}, {40, 0}, {1, 0}));
    CHECK(physics->OverlapBox({0, 0}, {2, 2}, {0x40000000, 1}).empty());
    CHECK(physics->OverlapBox({0, 0}, {2, 2}, {0x40000000, 1}, true).size() == 1);
    names[0].value.name.clear();
    std::reverse(names.begin(), names.end());
    world.Step(1.0 / 60, {});
    CHECK(SceneBody2D(world, body.Id()) == handle && ScenePhysicsEvents2D(world).empty());
    CHECK(shape.category == 0x40000000 && shape.mask == 1);
    CHECK(DescribeCollisionLayers2D(world).names[30] == "Unnamed layer 31");
}
