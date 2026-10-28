#pragma once

#include "pixie/app/App.hpp"
#include "pixie/runtime/ProjectSession.hpp"

#include <chrono>
#include <memory>

namespace pixie {

// Runs a Project in pixie::Run's window: the player side of a game. Each fixed update is one step of
// the project's session; each frame draws it. Once per second it logs how long the steps took. With a
// project folder open in the session (OpenProject, before Run), it plays the start scene.
class ProjectApp final : public App {
public:
    ProjectApp(std::unique_ptr<Project> project, SessionSettings settings)
        : session_(std::move(project), settings) {}

    bool Start(Window& window) override;
    void Update(const FrameTime& time, const Input& input) override;
    void Render(const FrameTime& time, const Input& input) override;
    void Stop() override;
    Telemetry* GetTelemetry() override { return session_.GetTelemetry(); }

    ProjectSession& Session() { return session_; }

private:
    ProjectSession session_;
    Window* window_ = nullptr;
    std::chrono::steady_clock::time_point windowStart_ = std::chrono::steady_clock::now();
    double totalMilliseconds_ = 0.0;
    double slowestMilliseconds_ = 0.0;
    int steps_ = 0;
};

} // namespace pixie
