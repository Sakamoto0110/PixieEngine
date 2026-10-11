#pragma once

#include <chrono>
#include <cstdint>

namespace pixie {

// Measures updates and frames per second over windows of about one second.
class RateMeter {
public:
    using Clock = std::chrono::steady_clock;

    explicit RateMeter(Clock::time_point start);

    void CountUpdate() { ++updates_; }
    void CountFrame() { ++frames_; }

    // Closes the current window when at least one second has passed; returns true if it did.
    bool Sample(Clock::time_point now);

    double UpdatesPerSecond() const { return updatesPerSecond_; }
    double FramesPerSecond() const { return framesPerSecond_; }

private:
    Clock::time_point windowStart_;
    std::uint64_t updates_ = 0;
    std::uint64_t frames_ = 0;
    double updatesPerSecond_ = 0.0;
    double framesPerSecond_ = 0.0;
};

} // namespace pixie
