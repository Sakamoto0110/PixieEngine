#include "FrameLimiter.hpp"

#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace pixie {

namespace {

// The coarse wait ends this early; the rest is spun, because OS sleeps overshoot.
constexpr auto kSpinMargin = std::chrono::microseconds(1500);

} // namespace

FrameLimiter::FrameLimiter(double maxFramesPerSecond) {
    if (maxFramesPerSecond <= 0.0) {
        return;
    }
    enabled_ = true;
    period_ = std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>(1.0 / maxFramesPerSecond));
    deadline_ = Clock::now() + period_;
#ifdef _WIN32
    timer_ = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                    TIMER_ALL_ACCESS);
#endif
}

FrameLimiter::~FrameLimiter() {
#ifdef _WIN32
    if (timer_ != nullptr) {
        CloseHandle(timer_);
    }
#endif
}

void FrameLimiter::Wait() {
    if (!enabled_) {
        return;
    }
    SleepUntil(deadline_);
    deadline_ += period_;
    // After a stall longer than one period, restart the schedule instead of rushing to catch up.
    const auto now = Clock::now();
    if (deadline_ < now) {
        deadline_ = now + period_;
    }
}

void FrameLimiter::SleepUntil(Clock::time_point deadline) {
    const auto coarseEnd = deadline - kSpinMargin;
    auto now = Clock::now();
    if (now < coarseEnd) {
#ifdef _WIN32
        if (timer_ != nullptr) {
            // Relative due time, in 100-nanosecond units.
            const auto ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(coarseEnd - now)
                                   .count() / 100;
            LARGE_INTEGER due;
            due.QuadPart = -static_cast<LONGLONG>(ticks);
            if (SetWaitableTimerEx(timer_, &due, 0, nullptr, nullptr, nullptr, 0)) {
                WaitForSingleObject(timer_, INFINITE);
            }
        } else {
            std::this_thread::sleep_until(coarseEnd);
        }
#else
        std::this_thread::sleep_until(coarseEnd);
#endif
    }
    while (Clock::now() < deadline) {
        std::this_thread::yield();
    }
}

} // namespace pixie
