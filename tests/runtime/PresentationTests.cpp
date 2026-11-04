#include "Check.hpp"
#include "pixie/runtime/Presentation2D.hpp"
#include "pixie/runtime/LightingEnvironment2D.hpp"
#include "pixie/runtime/ViewportCamera2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include <nlohmann/json.hpp>

using namespace pixie;

TEST(SceneEnvironmentOverrideDoesNotInstallOrEnableLighting) {
    World world;
    CHECK(world.CreateScope(ScopeId{7}, "room"));
    SceneEnvironment2D local; local.overrideWorld = true; local.value.ambientIntensity = 0.2;
    CHECK(world.SetScopeEnvironment(ScopeId{7}, local));
    auto resolved = ResolveViewportEnvironment2D(world, {EnvironmentSelectionKind2D::Scene, ScopeId{7}});
    CHECK(resolved.kind == EnvironmentResolutionKind2D::SceneOverride);
    CHECK(resolved.lighting.state == LightingState2D::Absent && !resolved.lighting.environment);
    CHECK(world.EntityCount() == 0 && !world.Presentation());
    WorldPresentation2D root; root.lighting = LightingState2D::Disabled;
    CHECK(world.SetPresentation(root));
    resolved = ResolveViewportEnvironment2D(world, {EnvironmentSelectionKind2D::Scene, ScopeId{7}});
    CHECK(resolved.lighting.state == LightingState2D::Disabled);
    CHECK(resolved.lighting.environment && resolved.lighting.environment->ambientIntensity == 0.2);
    root.lighting = LightingState2D::Configured; root.view.exposure = 2; root.renderer.maxParticleLights = 31;
    CHECK(world.SetPresentation(root));
    auto lighting = ResolveLightingSettings2D(world, 7);
    CHECK(lighting.enabled && lighting.ambientIntensity == 0.2 && lighting.exposure == 2 && lighting.maxParticleLights == 31);
    local.overrideWorld = false; CHECK(world.SetScopeEnvironment(ScopeId{7}, local));
    CHECK(ResolveLightingSettings2D(world, 7).ambientIntensity == root.environment.ambientIntensity);
    CHECK(world.Scopes()[0].environment->value.ambientIntensity == 0.2);
}

TEST(CanonicalPresentationRoundTripPreservesInactiveAndUnknownData) {
    ComponentTypes types; DescribeRuntimeComponents(types); World world;
    const std::string input = R"({"format":"pixie-scene","version":2,"seed":19,
      "presentation":{"schemaVersion":1,"lightingState":"disabled","futureRoot":17,
        "environment":{"ambientIntensity":0.3,"futureEnvironment":{"keep":true}},
        "view":{"exposure":2,"bloom":true,"futureView":13},
        "renderer":{"quality":"high","maxParticleLights":37,"futureRenderer":29},
        "legacyEffects":{"normalHeight":61,"futureLegacy":41},"camera":"11"},
      "sceneScopes":[{"id":"7","name":"room","futureScope":23,"camera":"11",
        "environment":{"mode":"inherit","futureOverride":31,"value":{"ambientIntensity":0.1,"futureValue":43}}}],
      "entities":[{"id":"11","components":{"Transform2D":{"position":[20,30]},"Camera2D":{"zoom":2}}}]})";
    CHECK(ReadScene(world, types, input).ok);
    auto encoded = nlohmann::json::parse(WriteScene(world, types));
    CHECK(encoded["presentation"]["futureRoot"] == 17);
    CHECK(encoded["presentation"]["environment"]["futureEnvironment"]["keep"] == true);
    CHECK(encoded["presentation"]["view"]["futureView"] == 13);
    CHECK(encoded["presentation"]["renderer"]["futureRenderer"] == 29);
    CHECK(encoded["presentation"]["legacyEffects"]["futureLegacy"] == 41);
    CHECK(encoded["sceneScopes"][0]["environment"]["value"]["futureValue"] == 43);
    CHECK(encoded["sceneScopes"][0]["environment"]["value"]["ambientIntensity"] == 0.1);
    CHECK(encoded["sceneScopes"][0]["environment"]["mode"] == "inherit");
    World reopened; CHECK(ReadScene(reopened, types, encoded.dump()).ok);
    CHECK(reopened.Presentation() == world.Presentation());
    CHECK(reopened.Scopes()[0].environment == world.Scopes()[0].environment);
    CHECK(reopened.Scopes()[0].camera == 0x11);
}

TEST(MixedAndInvalidPresentationRejectBeforeMutation) {
    ComponentTypes types; DescribeRuntimeComponents(types);
    for (const auto* presentation : {
        R"({"schemaVersion":1,"lightingState":"configured","view":{"exposure":-1}})",
        R"({"schemaVersion":1,"lightingState":"unknown"})",
        R"({"schemaVersion":1,"renderer":{"maxParticleLights":1.5}})",
        R"({"schemaVersion":1,"environment":{"ambient":[4294967296,0,0]}})",
        R"({"schemaVersion":1,"environment":{"fogColor":[-4294967296,0,0]}})",
        R"({"schemaVersion":1.0})",
        R"({"schemaVersion":1,"camera":"0"})"}) {
        World world(91);
        const auto input = std::string(R"({"format":"pixie-scene","version":2,"seed":17,"sceneScopes":[],"entities":[],"presentation":)") + presentation + "}";
        CHECK(!ReadScene(world, types, input).ok);
        CHECK(world.EntityCount() == 0 && world.GetRandom().Seed() == 91 && !world.Presentation());
    }
    World mixed(91);
    CHECK(!ReadScene(mixed, types, R"({"format":"pixie-scene","version":2,"seed":17,"sceneScopes":[],
       "presentation":{"schemaVersion":1},"entities":[{"id":"1","components":{"LightingSettings2D":{}}}]})").ok);
    CHECK(mixed.EntityCount() == 0 && mixed.GetRandom().Seed() == 91);
}

TEST(StableAuthoredCameraFallbackAndUndoIntent) {
    World world; Camera2D free; free.SetViewport({640, 480}); free.SetCenter({1, 2});
    auto global = world.Create(); global.Add<SceneId>(SceneId{11}); global.Add<Transform2D>().position = {20, 30};
    global.Add<CameraComponent2D>().zoom = 2;
    auto local = world.Create(); local.Add<SceneId>(SceneId{12}); local.Add<Transform2D>().position = {40, 50};
    local.Add<CameraComponent2D>().zoom = 3;
    WorldPresentation2D settings; settings.camera = 11; CHECK(world.SetPresentation(settings));
    CHECK(world.CreateScope(ScopeId{7}, "room")); CHECK(world.SetScopeCamera(ScopeId{7}, 12));
    const CameraSelection2D selection{CameraSelectionKind2D::Entity, 99, ScopeId{7}};
    auto resolved = ResolveViewportCamera2D(world, free, selection);
    CHECK(resolved.kind == CameraResolutionKind2D::SceneDefault && resolved.camera.Center() == pxPoint(40, 50));
    CHECK(!resolved.diagnostic.empty());
    local.Get<CameraComponent2D>().enabled = false;
    resolved = ResolveViewportCamera2D(world, free, selection);
    CHECK(resolved.kind == CameraResolutionKind2D::WorldDefault && resolved.camera.Zoom() == 2);
    world.Destroy(global);
    resolved = ResolveViewportCamera2D(world, free, selection);
    CHECK(resolved.kind == CameraResolutionKind2D::Free && resolved.camera.Center() == free.Center());
    global = world.Create(); global.Add<SceneId>(SceneId{11}); global.Add<Transform2D>().position = {60, 70}; global.Add<CameraComponent2D>().zoom = 4;
    resolved = ResolveViewportCamera2D(world, free, selection);
    CHECK(resolved.kind == CameraResolutionKind2D::WorldDefault && resolved.camera.Center() == pxPoint(60, 70));
    CHECK(selection.entity == 99 && selection.scene.value == 7);
    CHECK(ResolveViewportCamera2D(world, free, {}).camera.Center() == free.Center());
}
