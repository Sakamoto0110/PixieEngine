#include "Check.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/Parenting2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Shape.hpp"
#include "pixie/runtime/Velocity2D.hpp"
#include "pixie/runtime/World.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

using namespace pixie;

namespace {

struct SimpleScene {
    World world;
    // Reverse dependency order to ensure evaluation does not follow creation order.
    Entity a = ShapeAt("A", {0, 10});
    Entity b = ShapeAt("B", {10, 0});
    Entity pivot = ShapeAt("Pivot", {20, 0});
    Entity root = ShapeAt("Root", {100, 100});
    Entity alternate = ShapeAt("Alternate", {200, 200});

    Entity ShapeAt(std::string_view name, pxPoint position) {
        auto entity = world.Create(name);
        entity.Add<Transform2D>(Transform2D{position});
        entity.Add<Shape>();
        return entity;
    }

    SimpleScene() {
        CHECK(SetParent2D(a, pivot, ReparentMode2D::KeepLocal).empty());
        CHECK(SetParent2D(b, pivot, ReparentMode2D::KeepLocal).empty());
        CHECK(SetParent2D(pivot, root, ReparentMode2D::KeepLocal).empty());
    }
};

bool Near(pxPoint actual, pxPoint expected) {
    return std::abs(actual.x - expected.x) < 1e-9 && std::abs(actual.y - expected.y) < 1e-9;
}

Entity Authored(World& world, std::uint64_t id) {
    Entity found;
    world.Each<const SceneId>([&](Entity entity, const SceneId& value) {
        if (value.value == id) found = entity;
    });
    return found;
}

} // namespace

TEST(SimpleParentingSceneFollowsMotionBeforePostUpdate) {
    SimpleScene scene;
    CHECK(Parent2D(scene.pivot) == scene.root);
    CHECK((Children2D(scene.pivot) == std::vector<Entity>{scene.a, scene.b}));
    CHECK(Near(scene.a.Get<Transform2D>().position, {120, 110}));
    CHECK(Near(scene.b.Get<Transform2D>().position, {130, 100}));

    scene.root.Add<Velocity2D>(Velocity2D{{0, 0}, std::numbers::pi / 2});
    bool observed = false;
    scene.world.AddSystem(SystemPhase::PostUpdate, "observe hierarchy", [&](World&, const FrameTime&, const Input&) {
        observed = true;
        CHECK(Near(scene.pivot.Get<Transform2D>().position, {100, 120}));
        CHECK(Near(scene.a.Get<Transform2D>().position, {90, 120}));
        CHECK(Near(scene.b.Get<Transform2D>().position, {100, 130}));
    });
    scene.world.Step(1, Input{});
    CHECK(observed);
    CHECK(Near(scene.a.Get<PreviousTransform2D>().value.position, {120, 110}));
    const double diagonal = std::sqrt(0.5);
    CHECK(Near(InterpolatedTransform2D(scene.a, 0.5).position, {100 + 10 * diagonal, 100 + 30 * diagonal}));
    CHECK(Near(InterpolatedTransform2D(scene.b, 0.5).position, {100 + 30 * diagonal, 100 + 30 * diagonal}));

    auto rootPose = scene.root.Get<Transform2D>();
    rootPose.scale = {2, 2};
    CHECK(SetLocalTransform2D(scene.root, rootPose).empty());
    CHECK(Near(scene.a.Get<Transform2D>().position, {80, 140}));
    CHECK(Near(scene.b.Get<Transform2D>().position, {100, 160}));
    auto local = LocalTransform2D(scene.a).value_or(Transform2D{});
    CHECK(Near(local.position, {0, 10}));
    local.position = {0, 5};
    CHECK(SetLocalTransform2D(scene.a, local).empty());
    CHECK(Near(scene.a.Get<Transform2D>().position, {90, 140}));
}

TEST(ParentingKeepsSelectedPoseAndDetachesSurvivingChildren) {
    SimpleScene scene;
    auto alternate = scene.alternate.Get<Transform2D>();
    alternate.scale = {2, 2};
    alternate.rotation = std::numbers::pi / 2;
    CHECK(SetLocalTransform2D(scene.alternate, alternate).empty());
    const auto before = scene.a.Get<Transform2D>();
    CHECK(SetParent2D(scene.a, scene.alternate).empty());
    CHECK(Near(scene.a.Get<Transform2D>().position, before.position));
    CHECK(Near(scene.a.Get<PreviousTransform2D>().value.position, before.position));
    CHECK(Parent2D(scene.a) == scene.alternate);
    CHECK(Near(InterpolatedTransform2D(scene.a, 0.5).position, before.position));
    CHECK(SetParent2D(scene.a, {}).empty());
    CHECK(!Parent2D(scene.a));
    CHECK(Near(scene.a.Get<Transform2D>().position, before.position));
    CHECK(SetParent2D(scene.a, scene.pivot, ReparentMode2D::KeepLocal).empty());
    CHECK(Near(scene.a.Get<Transform2D>().position, {240, 210}));

    scene.world.AddSystem(SystemPhase::PostUpdate, "remove root", [&](World&, const FrameTime&, const Input&) {
        scene.root.Destroy();
        CHECK(scene.root.IsAlive());
    });
    scene.world.Step(0.1, Input{});
    CHECK(!scene.root.IsAlive());
    CHECK(!Parent2D(scene.pivot));
    CHECK(Parent2D(scene.a) == scene.pivot);
    CHECK(Near(scene.a.Get<Transform2D>().position, {240, 210}));
    CHECK(Near(scene.a.Get<PreviousTransform2D>().value.position, {240, 210}));
}

TEST(ParentingRejectsUnsupportedChangesBeforePublishing) {
    SimpleScene scene;
    World other;
    const auto foreign = other.Create();
    foreign.Add<Transform2D>();
    const auto dead = scene.world.Create();
    dead.Destroy();
    CHECK(!SetParent2D(scene.root, scene.root).empty());
    CHECK(!SetParent2D(scene.root, scene.a).empty());
    CHECK(!SetParent2D(scene.a, foreign).empty());
    CHECK(!SetParent2D(scene.a, dead).empty());
    CHECK(scene.world.CreateScope({1}, "Separate"));
    CHECK(scene.world.SetScope(scene.alternate, {1}));
    CHECK(!SetParent2D(scene.a, scene.alternate).empty());
    CHECK(!scene.world.SetScope(scene.a, {1}));
    CHECK(scene.world.SetScope(scene.alternate, {}));
    auto invalid = scene.root.Get<Transform2D>();
    invalid.scale = {1, 0};
    CHECK(!SetLocalTransform2D(scene.root, invalid).empty());
    CHECK(scene.root.Get<Transform2D>().scale.y == 1);

    scene.alternate.Add<Velocity2D>();
    CHECK(!SetParent2D(scene.alternate, scene.pivot).empty());
    CHECK(!Parent2D(scene.alternate));
    CHECK(!Parent2D(scene.root));
    CHECK(Parent2D(scene.pivot) == scene.root);
    CHECK(Parent2D(scene.a) == scene.pivot);
    CHECK(Near(scene.a.Get<Transform2D>().position, {120, 110}));
}

TEST(ParentingSelectionMovesOnlyBranchRootsAndRejectsAnInvalidBatch) {
    SimpleScene scene;
    bool changed = false;
    const auto original = scene.a.Get<Transform2D>().position;
    CHECK(SetParents2D(std::array{scene.pivot, scene.a}, scene.alternate,
                      ReparentMode2D::KeepWorld, &changed).empty());
    CHECK(changed && Parent2D(scene.pivot) == scene.alternate);
    CHECK(Parent2D(scene.a) == scene.pivot);
    CHECK(Near(scene.a.Get<Transform2D>().position, original));

    scene.root.Add<Velocity2D>();
    CHECK(!SetParents2D(std::array{scene.b, scene.root}, scene.alternate,
                       ReparentMode2D::KeepWorld, &changed).empty());
    CHECK(!changed && Parent2D(scene.b) == scene.pivot && !Parent2D(scene.root));
    CHECK(SetParents2D(std::array{scene.pivot, scene.a}, {}, ReparentMode2D::KeepWorld, &changed).empty());
    CHECK(changed && !Parent2D(scene.pivot) && Parent2D(scene.a) == scene.pivot);
    CHECK(Near(scene.a.Get<Transform2D>().position, original));
}

TEST(RuntimeOnlyParentingRefusesAuthoredSceneAndSnapshotWrites) {
    SimpleScene scene;
    CHECK(AddToScene(scene.alternate) != 0);
    CHECK(!SetParent2D(scene.alternate, scene.root).empty());
    bool addRejected = false;
    try {
        AddToScene(scene.a);
    } catch (const std::logic_error&) {
        addRejected = true;
    }
    CHECK(addRejected && !scene.a.Has<SceneId>());

    ComponentTypes types;
    CHECK(!WriteScene(scene.world, types).empty());
    scene.a.Add<SceneId>(SceneId{NewSceneId()});
    bool sceneRejected = false;
    bool fragmentRejected = false;
    try {
        WriteScene(scene.world, types);
    } catch (const std::runtime_error&) {
        sceneRejected = true;
    }
    try {
        WriteEntities(scene.world, types, std::array{scene.a});
    } catch (const std::runtime_error&) {
        fragmentRejected = true;
    }
    CHECK(sceneRejected && fragmentRejected);
    CHECK(Parent2D(scene.a) == scene.pivot);
}

TEST(AuthoredParentingRoundTripsAndRemapsFragmentParents) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World world;
    const auto loaded = LoadSceneFile(world, types, PIXIE_TEST_PARENTING_SCENE);
    CHECK(loaded.ok && loaded.warnings.empty());
    if (!loaded.ok) return;
    CHECK(SceneFormatVersion(world) == 3);
    const auto a = Authored(world, 1);
    const auto pivot = Authored(world, 3);
    const auto root = Authored(world, 4);
    CHECK(Parent2D(a) == pivot && Parent2D(pivot) == root);
    CHECK(Near(a.Get<Transform2D>().position, {120, 110}));
    world.Step(1, Input{});
    CHECK(Near(a.Get<Transform2D>().position, {90, 120}));
    const auto text = WriteScene(world, types);
    World reopened;
    CHECK(ReadScene(reopened, types, text).ok);
    CHECK(WriteScene(reopened, types) == text);
    CHECK(Near(Authored(reopened, 1).Get<Transform2D>().position, {90, 120}));

    const auto fragment = WriteEntities(world, types, std::array{a, pivot, root});
    const auto copies = ReadEntities(world, types, fragment, SceneIds::New);
    CHECK(copies.ok && copies.entities.size() == 3);
    if (!copies.ok || copies.entities.size() != 3) return;
    CHECK(Parent2D(copies.entities[0]) == copies.entities[1]);
    CHECK(Parent2D(copies.entities[1]) == copies.entities[2]);
    CHECK(copies.entities[2].Get<SceneId>().value != 4);
    const auto onlyChild = WriteEntities(world, types, std::array{a});
    const auto externalParent = ReadEntities(world, types, onlyChild, SceneIds::New);
    CHECK(externalParent.ok && externalParent.entities.size() == 1);
    if (externalParent.ok && !externalParent.entities.empty()) CHECK(Parent2D(externalParent.entities[0]) == pivot);
    World noParent;
    CHECK(!ReadEntities(noParent, types, onlyChild, SceneIds::New).ok);
    CHECK(noParent.EntityCount() == 0);
}

TEST(AuthoredParentingRejectsMalformedGraphsAndPreservesExistingScene) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World fixture;
    if (!LoadSceneFile(fixture, types, PIXIE_TEST_PARENTING_SCENE).ok) {
        CHECK(false);
        return;
    }
    const auto source = nlohmann::json::parse(WriteScene(fixture, types));
    World target;
    const auto survivor = target.Create("Survivor");
    survivor.Add<Transform2D>();
    survivor.Add<SceneId>(SceneId{100});
    const auto before = WriteScene(target, types);
    const auto rejects = [&](const nlohmann::json& candidate) {
        CHECK(!ReadScene(target, types, candidate.dump()).ok);
        CHECK(WriteScene(target, types) == before);
    };
    auto bad = source;
    bad["entities"][0]["parent2D"] = "ff";
    rejects(bad);
    bad = source;
    bad["entities"][3]["parent2D"] = "1";
    rejects(bad);
    bad = source;
    bad["entities"][0]["components"]["Transform2D"].erase("scale");
    rejects(bad);
    bad = source;
    bad["entities"][3]["components"]["Transform2D"]["scale"] = {1, 2};
    rejects(bad);
    bad = source;
    bad["version"] = 4;
    rejects(bad);
}

TEST(ParentingRejectsRawChildPoseWritesAndAllowsDeferredEdits) {
    SimpleScene scene;
    const auto evaluated = scene.a.Get<Transform2D>();
    scene.a.Get<Transform2D>().position = {1, 2};
    bool rejected = false;
    try {
        scene.world.Step(0.1, Input{});
    } catch (const std::logic_error&) {
        rejected = true;
    }
    CHECK(rejected && scene.world.Steps() == 0);
    CHECK(Near(scene.a.Get<Transform2D>().position, {1, 2}));
    scene.a.Get<Transform2D>() = evaluated;
    scene.world.AddSystem(SystemPhase::PostUpdate, "defer hierarchy edit", [&](World& world, const FrameTime&, const Input&) {
        CHECK(!SetParent2D(scene.a, scene.alternate).empty());
        CHECK(Parent2D(scene.a) == scene.pivot);
        world.Defer([&](World&) { CHECK(SetParent2D(scene.a, scene.alternate).empty()); });
    });
    scene.world.Step(0.1, Input{});
    CHECK(Parent2D(scene.a) == scene.alternate);
    CHECK(Near(scene.a.Get<Transform2D>().position, evaluated.position));
}
