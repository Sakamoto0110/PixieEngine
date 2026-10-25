#include "Check.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/ProjectApp.hpp"
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
