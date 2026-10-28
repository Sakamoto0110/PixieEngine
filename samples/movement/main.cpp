#include "pixie/render/RenderTarget.hpp"
#include "pixie/render/2d/Camera2D.hpp"
#include "pixie/app/Run.hpp"
#include "pixie/app/RunOptions.hpp"
#include "pixie/render/2d/ShapeRenderer.hpp"
#include "pixie/runtime/CharacterController2D.hpp"
#include "pixie/runtime/World.hpp"
#include <array>
#include <iostream>

namespace {
struct Block { pxPoint center; pxVec2 halfSize; double rotation; };
constexpr std::array<Block, 4> kRoom = {{{{0, 190}, {440, 16}, 0},
    {{70, 136}, {160, 10}, -0.3}, {{360, 80}, {12, 110}, 0}, {{200, -10}, {80, 12}, 0}}};

class MovementRoom final : public pixie::App {
public:
    MovementRoom(bool autoplay, bool enabled)
        : collision_(32, &world_.GetTelemetry()), actor_({-310, 80}), autoplay_(autoplay) {
        auto& metrics = world_.GetTelemetry();
        metrics.SetEnabled(enabled);
        for (const auto& block : kRoom) collision_.AddBox(block.center, block.halfSize, block.rotation);
        x_ = metrics.RegisterGauge("character.x", "world_units");
        y_ = metrics.RegisterGauge("character.y", "world_units");
        grounded_ = metrics.RegisterGauge("character.grounded");
        world_.AddSystem(pixie::SystemPhase::PostUpdate, "character.move",
            [this](auto& world, const auto& time, const auto& input) {
                if (input.WasPressed(pixie::Key::R)) actor_.Teleport({-310, 80});
                previous_ = actor_.State().position;
                double horizontal = 0;
                bool jump = false;
                if (autoplay_) {
                    horizontal = world.Steps() > 60 ? 1 : 0;
                    jump = world.Steps() == 190;
                } else {
                    horizontal = (input.IsDown(pixie::Key::D) || input.IsDown(pixie::Key::Right) ? 1.0 : 0.0)
                               - (input.IsDown(pixie::Key::A) || input.IsDown(pixie::Key::Left) ? 1.0 : 0.0);
                    jump = input.WasPressed(pixie::Key::Space);
                }
                actor_.Step(collision_, horizontal, jump, time.delta);
                metricsFor(world);
            });
    }
    bool Start(pixie::Window& window) override { window_ = &window; return shapes_.Create(); }
    void Stop() override { shapes_.Destroy(); }
    void Update(const pixie::FrameTime& time, const pixie::Input& input) override { world_.Step(time.delta, input); }
    void Render(const pixie::FrameTime& time, const pixie::Input&) override {
        pixie::ClearColor(0.025f, 0.035f, 0.06f, 1);
        camera_.SetViewport(window_->GetFramebufferSize());
        for (const auto& block : kRoom) {
            pixie::ShapeInstance shape;
            shape.position = {static_cast<float>(block.center.x), static_cast<float>(block.center.y)};
            shape.scale = {static_cast<float>(block.halfSize.x), static_cast<float>(block.halfSize.y)};
            shape.rotation = static_cast<float>(block.rotation);
            shape.color = {85, 105, 140};
            shapes_.Submit(pixie::ShapeKind::Quad, shape);
        }
        const auto position = pxPoint::Lerp(previous_, actor_.State().position, time.alpha);
        const auto& settings = actor_.Settings();
        pixie::ShapeInstance body;
        body.position = {static_cast<float>(position.x), static_cast<float>(position.y)};
        body.scale = {static_cast<float>(settings.radius), static_cast<float>(settings.halfSegment)};
        body.color = actor_.State().grounded ? pxColorRgba{70, 225, 180} : pxColorRgba{255, 190, 80};
        shapes_.Submit(pixie::ShapeKind::Quad, body);
        body.scale = {static_cast<float>(settings.radius), static_cast<float>(settings.radius)};
        for (double offset : {-settings.halfSegment, settings.halfSegment}) {
            body.position.y = static_cast<float>(position.y + offset);
            shapes_.Submit(pixie::ShapeKind::Circle, body);
        }
        shapes_.Flush(camera_.MakeView());
    }
    pixie::Telemetry* GetTelemetry() override { return &world_.GetTelemetry(); }
    void Headless() { for (int i = 0; i < 600; ++i) world_.Step(1.0 / 60, {}); }
    const pixie::CapsuleMotion2D& State() const { return actor_.State(); }
private:
    void metricsFor(pixie::World& world) {
        auto& m = world.GetTelemetry();
        m.SetGauge(x_, actor_.State().position.x);
        m.SetGauge(y_, actor_.State().position.y);
        m.SetGauge(grounded_, actor_.State().grounded ? 1 : 0);
    }
    pixie::World world_;
    pixie::CollisionWorld2D collision_;
    pixie::CharacterController2D actor_;
    pxPoint previous_{-310, 80};
    bool autoplay_;
    pixie::MetricHandle x_, y_, grounded_;
    pixie::ShapeRenderer shapes_;
    pixie::Camera2D camera_;
    pixie::Window* window_ = nullptr;
};
}
int main(int argc, char** argv) {
    pixie::RunConfig config;
    config.title = "Movement room | A/D arrows move | Space jump | R reset | F3 metrics";
    bool headless = false, autoplay = false;
    auto flag = [](std::string_view value, bool& target) {
        if (value != "on" && value != "off") return false;
        target = value == "on"; return true;
    };
    const std::array extra = {
        pixie::RunOption{"--headless", "on|off", "600 fixed steps and CSV without a window",
            [&](std::string_view v) { return flag(v, headless); }},
        pixie::RunOption{"--autoplay", "on|off", "scripted movement instead of keyboard input",
            [&](std::string_view v) { return flag(v, autoplay); }}
    };
    if (!pixie::ParseRunOptions(argc, argv, "movement-demo", config, extra)) return 2;
    MovementRoom app(autoplay, config.telemetry.value_or(true));
    int result = 0;
    if (headless) app.Headless(); else result = pixie::Run(app, config);
    app.GetTelemetry()->WriteCsv(std::cout);
    std::cerr << "Final character: " << app.State().position.x << ", " << app.State().position.y
              << "; grounded=" << app.State().grounded << '\n';
    return result;
}
