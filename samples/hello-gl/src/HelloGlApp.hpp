#pragma once

#include "Ball.hpp"
#include "BallRenderer.hpp"
#include "TextRenderer.hpp"

#include "pixie/app/App.hpp"
#include "pixie/core/RateMeter.hpp"

#include <vector>

// Balls bouncing off the window edges, with "hello GL" in the middle and the measured rates in
// the top-left corner.
class HelloGlApp final : public pixie::App {
public:
    bool Start(pixie::Window& window) override;
    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Render(const pixie::FrameTime& time, const pixie::Input& input) override;
    void Stop() override;

private:
    Vec2 Bounds() const;

    pixie::Window* window_ = nullptr;
    std::vector<Ball> balls_;
    std::vector<BallInstance> instances_;
    BallRenderer ballRenderer_;
    TextRenderer titleText_;
    TextRenderer ratesText_;
    pixie::RateMeter rates_{pixie::RateMeter::Clock::now()};
};
