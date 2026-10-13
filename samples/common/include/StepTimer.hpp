#pragma once

#include <chrono>
#include <string_view>

namespace sample {

// Measures how long the World's steps take, and logs the average and the slowest once per second
// of wall time, so a sample's cost can be compared with the budget of one update (16.7 ms at 60
// updates per second).
class StepTimer {
public:
    explicit StepTimer(std::string_view label) : label_(label) {}

    void Begin() { start_ = Clock::now(); }
    void End();

private:
    using Clock = std::chrono::steady_clock;

    std::string_view label_;
    Clock::time_point start_{};
    Clock::time_point windowStart_ = Clock::now();
    double total_ = 0.0;
    double slowest_ = 0.0;
    int count_ = 0;
};

} // namespace sample
