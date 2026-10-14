#include "pixie/runtime/ProjectApp.hpp"

#include "pixie/Log.hpp"

#include <algorithm>
#include <format>

namespace pixie {

bool ProjectApp::Start(Window& window) {
    window_ = &window;
    if (!session_.Start(window.GetFramebufferSize())) {
        return false;
    }
    // A player plays: a project opens in edit mode, for an editor.
    if (session_.HasProject() && !session_.Play()) {
        LogError(std::format("{}: {}", session_.ProjectName(), session_.Error()));
        return false;
    }
    LogInfo(std::format("{}: started", session_.ProjectName()));
    return true;
}

void ProjectApp::Update(const FrameTime& time, const Input& input) {
    session_.Resize(window_->GetFramebufferSize());
    session_.Update(time.delta, input);

    const double milliseconds = session_.LastStepMilliseconds();
    totalMilliseconds_ += milliseconds;
    slowestMilliseconds_ = std::max(slowestMilliseconds_, milliseconds);
    ++steps_;
    const auto now = std::chrono::steady_clock::now();
    if (now - windowStart_ >= std::chrono::seconds(1)) {
        LogInfo(std::format("{} step: {} steps, {:.3f} ms average, {:.3f} ms slowest",
                            session_.ProjectName(), steps_, totalMilliseconds_ / steps_,
                            slowestMilliseconds_));
        windowStart_ = now;
        totalMilliseconds_ = 0.0;
        slowestMilliseconds_ = 0.0;
        steps_ = 0;
    }
}

void ProjectApp::Render(const FrameTime& time, const Input&) {
    session_.Resize(window_->GetFramebufferSize());
    session_.Render(time.alpha);
}

void ProjectApp::Stop() {
    session_.Stop();
}

} // namespace pixie
