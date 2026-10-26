#include "Check.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/ProjectApp.hpp"
#include "pixie/runtime/SceneMovement2D.hpp"
#include <memory>

TEST(WorldTelemetryCountsNamedSystemsWithoutChangingUpdateOrder) {
    pixie::World a(5),b(5);
    std::string first,second;
    a.AddSystem(pixie::SystemPhase::PreUpdate,"first",[&](auto&,const auto&,const auto&){first+='p';});
    a.AddSystem(pixie::SystemPhase::PostUpdate,"last",[&](auto&,const auto&,const auto&){first+='q';});
    b.AddSystem(pixie::SystemPhase::PreUpdate,"first",[&](auto&,const auto&,const auto&){second+='p';});
    b.AddSystem(pixie::SystemPhase::PostUpdate,"last",[&](auto&,const auto&,const auto&){second+='q';});
    a.GetTelemetry().SetEnabled(true);
    for(int i=0;i<4;++i) {a.Step(1.0/60,{});b.Step(1.0/60,{});}
    CHECK(first=="pqpqpqpq" && first==second && a.Elapsed()==b.Elapsed());
    for(const auto& m:a.GetTelemetry().Snapshot()) CHECK(m.timing.samples==4);
    for(const auto& m:b.GetTelemetry().Snapshot()) CHECK(m.timing.samples==0);
    a.GetTelemetry().Reset();
    a.Step(1.0/60,{});
    for(const auto& m:a.GetTelemetry().Snapshot()) CHECK(m.timing.samples==1);
}

namespace {
class EmptyMetricsProject final : public pixie::Project {
public:
    std::string_view Name() const override { return "telemetry lifetime"; }
    void Build(pixie::World&,pixie::Camera2D&) override {}
};
}

TEST(ProjectTelemetryIsAbsentBeforeStartAndAfterStop) {
    pixie::ProjectApp app(std::make_unique<EmptyMetricsProject>(),{});
    CHECK(app.GetTelemetry()==nullptr && app.Session().GetTelemetry()==nullptr);
    app.Stop();
    CHECK(app.GetTelemetry()==nullptr && app.Session().GetTelemetry()==nullptr);
}

namespace {
double Gauge(pixie::World& world, std::string_view name) {
    for (const auto& metric : world.GetTelemetry().Snapshot())
        if (metric.name == name) return metric.gauge;
    CHECK(false);
    return -1;
}
}

TEST(PhysicsTelemetryDistinguishesRebuildsFromSteadyStateAndDoesNotChangeMotion) {
    pixie::World enabled, disabled;
    auto setup = [](pixie::World& world) {
        auto entity = world.Create("measured box");
        entity.Add<pixie::Transform2D>();
        entity.Add<pixie::Collider2D>().halfSize = {16, 16};
        entity.Add<pixie::RigidBody2D>();
        pixie::InstallSceneMovement2D(world);
        return entity;
    };
    auto first = setup(enabled), second = setup(disabled);
    enabled.GetTelemetry().SetEnabled(true);
    enabled.Step(1.0 / 60, {}); disabled.Step(1.0 / 60, {});
    CHECK(Gauge(enabled, "physics.scene.rebuilds") == 1);
    CHECK(Gauge(enabled, "physics.scene.sync_calls") == 2);
    CHECK(Gauge(enabled, "physics.scene.pending_commands") == 0);
    CHECK(Gauge(enabled, "physics.scene.bodies_published") == 1);
    const auto handle = pixie::SceneBody2D(enabled, first.Id());
    for (int i = 0; i < 120; ++i) {
        enabled.Step(1.0 / 60, {}); disabled.Step(1.0 / 60, {});
        CHECK(Gauge(enabled, "physics.scene.rebuilds") == 0);
        CHECK(pixie::SceneBody2D(enabled, first.Id()) == handle);
        CHECK(first.Get<pixie::Transform2D>().position == second.Get<pixie::Transform2D>().position);
    }
    first.Get<pixie::Collider2D>().halfSize = {20, 16};
    enabled.Step(1.0 / 60, {});
    CHECK(Gauge(enabled, "physics.scene.rebuilds") == 1);
    CHECK(pixie::SceneBody2D(enabled, first.Id()) != handle);
    auto& t = enabled.GetTelemetry();
    CHECK(t.Timing(t.RegisterTiming("physics.scene.sync")).samples == enabled.Steps() * 2);
    CHECK(t.Timing(t.RegisterTiming("physics.scene.publish")).samples == enabled.Steps());
    CHECK(t.Timing(t.RegisterTiming("physics.scene.events.publish")).samples == enabled.Steps());
    CHECK(t.Timing(t.RegisterTiming("physics.scene.rebuild")).samples == 3); // World creation and two body creations.
    for (const auto& metric : disabled.GetTelemetry().Snapshot()) {
        if (metric.kind == pixie::MetricKind::Timing) CHECK(metric.timing.samples == 0);
        if (metric.kind == pixie::MetricKind::Gauge) CHECK(metric.gauge == 0);
    }
    t.SetEnabled(false);
    const auto samples = t.Timing(t.RegisterTiming("physics.scene.sync")).samples;
    enabled.Step(1.0 / 60, {});
    CHECK(t.Timing(t.RegisterTiming("physics.scene.sync")).samples == samples);
    t.SetEnabled(true);
    t.Reset();
    enabled.Step(1.0 / 60, {});
    CHECK(Gauge(enabled, "physics.scene.rebuilds") == 0 && Gauge(enabled, "physics.scene.sync_calls") == 2);
}

TEST(PhysicsTelemetryReportsMotorCountAndRetainsInvalidGeometryCleanup) {
    pixie::World world;
    auto actor = world.Create("measured motor");
    actor.Add<pixie::Transform2D>(); actor.Add<pixie::CharacterMotor2D>();
    pixie::InstallSceneMovement2D(world);
    world.GetTelemetry().SetEnabled(true);
    world.Step(1.0 / 60, {});
    CHECK(Gauge(world, "physics.scene.rebuilds") == 1 && Gauge(world, "physics.scene.motor_count") == 1);
    CHECK(world.GetTelemetry().Timing(world.GetTelemetry().RegisterTiming("physics.scene.motors")).samples == 1);
    actor.Get<pixie::CharacterMotor2D>().radius = -1;
    world.Step(1.0 / 60, {});
    CHECK(Gauge(world, "physics.scene.motor_count") == 0);
    CHECK(pixie::CharacterMotion2D(world, actor.Id()) == nullptr);
}
