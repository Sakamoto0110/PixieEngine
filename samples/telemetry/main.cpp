#include "pixie/Run.hpp"
#include "pixie/RunOptions.hpp"
#include "pixie/Gl.hpp"
#include "pixie/ShapeRenderer.hpp"
#include "pixie/runtime/World.hpp"
#include <array>
#include <cmath>
#include <iostream>

namespace {
class MetricsDemo final : public pixie::App {
public:
    explicit MetricsDemo(bool enabled) {
        auto& metrics=world_.GetTelemetry();
        metrics.SetEnabled(enabled);
        updates_=metrics.RegisterCounter("demo.updates","updates");
        elapsed_=metrics.RegisterGauge("demo.simulated_time","s");
        world_.AddSystem(pixie::SystemPhase::PostUpdate,"demo.measure",[this](auto& world,const auto&,const auto&) {
            world.GetTelemetry().Increment(updates_);
            world.GetTelemetry().SetGauge(elapsed_,world.Elapsed());
        });
    }
    bool Start(pixie::Window& window) override { window_=&window;return shapes_.Create(); }
    void Update(const pixie::FrameTime& time,const pixie::Input& input) override { world_.Step(time.delta,input); }
    void Render(const pixie::FrameTime&,const pixie::Input&) override {
        glClearColor(0.035f,0.05f,0.085f,1);glClear(GL_COLOR_BUFFER_BIT);
        camera_.SetViewport(window_->GetFramebufferSize());
        pixie::ShapeInstance shape;
        shape.position={static_cast<float>(180*std::sin(world_.Elapsed())),70};
        shape.scale={45,45};shape.rotation=static_cast<float>(world_.Elapsed());shape.color={50,220,210};
        shapes_.Submit(pixie::ShapeKind::Quad,shape);shapes_.Flush(camera_);
    }
    void Stop() override { shapes_.Destroy(); }
    pixie::Telemetry* GetTelemetry() override { return &world_.GetTelemetry(); }
    void Headless() { for(int i=0;i<600;++i) world_.Step(1.0/60,{}); }
private:
    pixie::World world_;
    pixie::MetricHandle updates_,elapsed_;
    pixie::ShapeRenderer shapes_;
    pixie::Camera2D camera_;
    pixie::Window* window_=nullptr;
};
}

int main(int argc,char** argv) {
    pixie::RunConfig config;config.title="Telemetry demo";
    bool headless=true;
    const std::array extra={pixie::RunOption{"--headless","on|off","600 headless steps and CSV, or native F3 metrics (default on)",
        [&](std::string_view value){if(value!="on" && value!="off") return false;headless=value=="on";return true;}}};
    if(!pixie::ParseRunOptions(argc,argv,"telemetry-demo",config,extra)) return 2;
    MetricsDemo app(config.telemetry.value_or(true));
    int result=0;
    if(headless) app.Headless();else result=pixie::Run(app,config);
    app.GetTelemetry()->WriteCsv(std::cout);
    return result;
}
