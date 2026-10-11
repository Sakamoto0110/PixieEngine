#pragma once

#include <chrono>

namespace pixie {

// Holds frames to a maximum rate by waiting until each frame's deadline.
class FrameLimiter {
public:
    using Clock = std::chrono::steady_clock;

    // A rate of 0 disables the limiter.
    explicit FrameLimiter(double maxFramesPerSecond);
    ~FrameLimiter();

    FrameLimiter(const FrameLimiter&) = delete;
    FrameLimiter& operator=(const FrameLimiter&) = delete;

    // Waits until the next frame may start.
    void Wait();

private:
    void SleepUntil(Clock::time_point deadline);

    Clock::duration period_{};
    Clock::time_point deadline_{};
    bool enabled_ = false;
    void* timer_ = nullptr;
};

} // namespace pixie
