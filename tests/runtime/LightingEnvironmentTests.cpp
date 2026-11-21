#include "Check.hpp"

#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/LightingDrawing.hpp"
#include "pixie/runtime/LightingEnvironment2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/World.hpp"

#include <nlohmann/json.hpp>

using namespace pixie;

TEST(EnvironmentLightingReadDoesNotInstallMissingSettings) {
    World world;
    const auto view = ReadEnvironmentLighting2D(world);
    CHECK(view.state == LightingState2D::Absent);
    CHECK(!view.environment.has_value());
    CHECK(EditLegacyLightingSettings2D(world) == nullptr);
    CHECK(FindLightingSettings(world) == nullptr);
    CHECK(FindActiveLightingSettings(world) == nullptr);
    CHECK(world.EntityCount() == 0);
    CHECK(world.Registry().view<const LightingSettings2D>().empty());
}

TEST(EnvironmentLightingReadKeepsConfiguredDefaultsDistinctFromAbsence) {
    World world;
    auto entity = world.Create();
    auto& settings = entity.Add<LightingSettings2D>();
    const auto view = ReadEnvironmentLighting2D(world);
    CHECK(view.state == LightingState2D::Configured);
    CHECK(view.environment.has_value());
    if (view.environment) {
        CHECK(view.environment->ambient == settings.ambient);
        CHECK(view.environment->ambientIntensity == settings.ambientIntensity);
        CHECK(view.environment->fog == settings.fog);
        CHECK(view.environment->fogColor == settings.fogColor);
        CHECK(view.environment->fogDensity == settings.fogDensity);
        CHECK(view.environment->fogTop == settings.fogTop);
        CHECK(view.environment->fogBottom == settings.fogBottom);
    }
    CHECK(FindLightingSettings(world) == &settings);
    CHECK(FindActiveLightingSettings(world) == &settings);
    CHECK(EditLegacyLightingSettings2D(world) == &settings);
    settings.fog = true;
    const auto edited = ReadEnvironmentLighting2D(world);
    CHECK(edited.environment.has_value());
    if (edited.environment) CHECK(edited.environment->fog);
    CHECK(world.EntityCount() == 1);
}

TEST(DisabledLightingRetainsAuthoredEnvironmentAndRejectsActiveRendering) {
    World world;
    auto entity = world.Create();
    auto& settings = entity.Add<LightingSettings2D>();
    settings.ambient = {30, 40, 50};
    settings.ambientIntensity = 0.23;
    settings.fog = true;
    settings.fogDensity = 0.72;
    settings.bloom = true;
    settings.enabled = false;

    const auto view = ReadEnvironmentLighting2D(world);
    CHECK(view.state == LightingState2D::Disabled);
    CHECK(view.environment.has_value());
    if (view.environment) {
        CHECK(view.environment->ambient == settings.ambient);
        CHECK(view.environment->ambientIntensity == 0.23);
        CHECK(view.environment->fog && view.environment->fogDensity == 0.72);
    }
    CHECK(FindLightingSettings(world) == &settings);
    CHECK(FindActiveLightingSettings(world) == nullptr);
    settings.enabled = true;
    CHECK(ReadEnvironmentLighting2D(world).state == LightingState2D::Configured);
    CHECK(FindActiveLightingSettings(world) == &settings);
    CHECK(settings.bloom && settings.fog && settings.ambientIntensity == 0.23);
}

TEST(VersionOneLightingWithoutEnabledPreservesAppearanceAndDisabledRoundTrips) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    World world;
    constexpr auto legacy = R"({"format":"pixie-scene","version":1,"seed":1,"entities":[
        {"id":"1","components":{"LightingSettings2D":{"v":1,"ambientIntensity":0.23,
        "fog":true,"fogDensity":0.72,"bloom":true}}}]})";
    CHECK(ReadScene(world, types, legacy).ok);
    auto settingsView = world.Registry().view<LightingSettings2D>();
    CHECK(settingsView.size() == 1);
    if (settingsView.empty()) return;
    auto& settings = settingsView.get<LightingSettings2D>(*settingsView.begin());
    CHECK(settings.enabled);
    CHECK(ReadEnvironmentLighting2D(world).state == LightingState2D::Configured);
    CHECK(FindActiveLightingSettings(world) == &settings);

    settings.enabled = false;
    const auto saved = nlohmann::json::parse(WriteScene(world, types));
    CHECK(saved["version"] == 1);
    const auto& component = saved["entities"][0]["components"]["LightingSettings2D"];
    CHECK(component["enabled"] == false);
    CHECK(component["ambientIntensity"] == 0.23);
    CHECK(component["fog"] == true && component["fogDensity"] == 0.72);
    CHECK(component["bloom"] == true);

    World restored;
    CHECK(ReadScene(restored, types, saved.dump()).ok);
    const auto restoredView = restored.Registry().view<const LightingSettings2D>();
    CHECK(restoredView.size() == 1);
    if (restoredView.empty()) return;
    const auto& restoredSettings = restoredView.get<const LightingSettings2D>(*restoredView.begin());
    CHECK(restoredSettings == settings);
    CHECK(ReadEnvironmentLighting2D(restored).state == LightingState2D::Disabled);
    CHECK(FindActiveLightingSettings(restored) == nullptr);
}

TEST(EnvironmentLightingReadsTheLegacyRendererChoiceWithDuplicates) {
    World world;
    auto first = world.Create();
    auto& firstSettings = first.Add<LightingSettings2D>();
    firstSettings.ambient = {30, 40, 50};
    firstSettings.fogDensity = 0.2;
    auto second = world.Create();
    auto& secondSettings = second.Add<LightingSettings2D>();
    secondSettings.ambient = {180, 190, 200};
    secondSettings.fogDensity = 0.8;

    const LightingSettings2D* chosen = FindLightingSettings(world);
    CHECK(chosen == &firstSettings || chosen == &secondSettings);
    auto view = ReadEnvironmentLighting2D(world);
    CHECK(view.state == LightingState2D::Configured);
    CHECK(view.environment.has_value());
    if (view.environment) {
        CHECK(view.environment->ambient == chosen->ambient);
        CHECK(view.environment->fogDensity == chosen->fogDensity);
    }

    if (chosen == &firstSettings) firstSettings.enabled = false;
    else secondSettings.enabled = false;
    CHECK(ReadEnvironmentLighting2D(world).state == LightingState2D::Disabled);
    CHECK(FindActiveLightingSettings(world) == nullptr);

    CHECK(EditLegacyLightingSettings2D(world) == nullptr);
    CHECK(!firstSettings.fog && !firstSettings.bloom);
    CHECK(!secondSettings.fog && !secondSettings.bloom);
    CHECK(world.Registry().view<const LightingSettings2D>().size() == 2);
}

TEST(ViewportEnvironmentSelectionRetainsIntentAcrossRemovalAndUndo) {
    World world;
    const ScopeId room{71};
    const EnvironmentSelection2D selection{EnvironmentSelectionKind2D::Scene, room};
    CHECK(world.CreateScope(room, "room"));
    auto selected = ResolveViewportEnvironment2D(world, selection);
    CHECK(selected.kind == EnvironmentResolutionKind2D::SceneInheritedWorld);
    CHECK(selected.lighting.state == LightingState2D::Absent);
    CHECK(selected.diagnostic.empty());
    CHECK(world.EntityCount() == 0);

    auto& lighting = world.Create().Add<LightingSettings2D>();
    lighting.enabled = false;
    selected = ResolveViewportEnvironment2D(world, selection);
    CHECK(selected.kind == EnvironmentResolutionKind2D::SceneInheritedWorld);
    CHECK(selected.lighting.state == LightingState2D::Disabled);
    lighting.enabled = true;
    CHECK(ResolveViewportEnvironment2D(world, selection).lighting.state == LightingState2D::Configured);

    CHECK(world.RemoveScope(room));
    selected = ResolveViewportEnvironment2D(world, selection);
    CHECK(selected.kind == EnvironmentResolutionKind2D::RemovedSceneFallback);
    CHECK(selected.lighting.state == LightingState2D::Configured);
    CHECK(!selected.diagnostic.empty());
    CHECK(selection.scene == room);
    CHECK(world.CreateScope(room, "restored room"));
    selected = ResolveViewportEnvironment2D(world, selection);
    CHECK(selected.kind == EnvironmentResolutionKind2D::SceneInheritedWorld);
    CHECK(selected.diagnostic.empty());
}
