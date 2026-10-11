#include "HelloGlApp.hpp"

#include "pixie/Gl.hpp"

#include <cmath>
#include <format>
#include <numbers>
#include <random>

namespace {

constexpr int kBallCount = 16;
constexpr Color kBackground{0.08f, 0.09f, 0.12f, 1.0f};
constexpr Color kTextColor{0.95f, 0.95f, 0.95f, 1.0f};
constexpr Color kRatesColor{0.75f, 0.78f, 0.85f, 1.0f};
constexpr Vec2 kRatesPosition{12.0f, 12.0f};
constexpr float kRatesScale = 3.0f;

constexpr Color kPalette[] = {
    {0.96f, 0.36f, 0.42f, 1.0f}, {0.98f, 0.73f, 0.27f, 1.0f}, {0.42f, 0.84f, 0.51f, 1.0f},
    {0.30f, 0.67f, 0.96f, 1.0f}, {0.71f, 0.49f, 0.96f, 1.0f}, {0.98f, 0.55f, 0.82f, 1.0f},
};

} // namespace

bool HelloGlApp::Start(pixie::Window& window) {
    window_ = &window;
    if (!ballRenderer_.Create() || !titleText_.Create() || !ratesText_.Create()) {
        return false;
    }
    titleText_.SetText("hello GL");
    ratesText_.SetText("-- fps | -- ups");
    rates_ = pixie::RateMeter(pixie::RateMeter::Clock::now());

    std::mt19937 random(std::random_device{}());
    std::uniform_real_distribution<float> radius(12.0f, 36.0f);
    std::uniform_real_distribution<float> speed(120.0f, 360.0f);
    std::uniform_real_distribution<float> angle(0.0f, 2.0f * std::numbers::pi_v<float>);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::uniform_int_distribution<std::size_t> color(0, std::size(kPalette) - 1);

    const Vec2 bounds = Bounds();
    for (int i = 0; i < kBallCount; ++i) {
        Ball ball;
        ball.radius = radius(random);
        ball.position = {ball.radius + unit(random) * (bounds.x - 2.0f * ball.radius),
                         ball.radius + unit(random) * (bounds.y - 2.0f * ball.radius)};
        ball.previousPosition = ball.position;
        const float direction = angle(random);
        ball.velocity = Vec2{std::cos(direction), std::sin(direction)} * speed(random);
        ball.color = kPalette[color(random)];
        balls_.push_back(ball);
    }
    instances_.resize(balls_.size());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return true;
}

void HelloGlApp::Update(const pixie::FrameTime& time) {
    const Vec2 bounds = Bounds();
    for (Ball& ball : balls_) {
        Step(ball, static_cast<float>(time.delta), bounds);
    }
    rates_.CountUpdate();
}

void HelloGlApp::Render(const pixie::FrameTime& time) {
    glClearColor(kBackground.r, kBackground.g, kBackground.b, kBackground.a);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto alpha = static_cast<float>(time.alpha);
    for (std::size_t i = 0; i < balls_.size(); ++i) {
        const Ball& ball = balls_[i];
        instances_[i] = {Lerp(ball.previousPosition, ball.position, alpha), ball.radius,
                         ball.color};
    }

    const pixie::FramebufferSize viewport = window_->GetFramebufferSize();
    ballRenderer_.Draw(instances_, viewport);
    titleText_.DrawCentered(viewport, 0.4f, kTextColor);

    rates_.CountFrame();
    if (rates_.Sample(pixie::RateMeter::Clock::now())) {
        ratesText_.SetText(std::format("{:.1f} fps | {:.1f} ups", rates_.FramesPerSecond(),
                                       rates_.UpdatesPerSecond()));
    }
    ratesText_.DrawAt(viewport, kRatesPosition, kRatesScale, kRatesColor);
}

void HelloGlApp::Stop() {
    ratesText_.Destroy();
    titleText_.Destroy();
    ballRenderer_.Destroy();
}

Vec2 HelloGlApp::Bounds() const {
    const pixie::FramebufferSize size = window_->GetFramebufferSize();
    return {static_cast<float>(size.width), static_cast<float>(size.height)};
}
