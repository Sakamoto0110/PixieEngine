#include "pixie/RateMeter.hpp"

namespace pixie {

RateMeter::RateMeter(Clock::time_point start) : windowStart_(start) {}

bool RateMeter::Sample(Clock::time_point now) {
    const double seconds = std::chrono::duration<double>(now - windowStart_).count();
    if (seconds < 1.0) {
        return false;
    }
    updatesPerSecond_ = static_cast<double>(updates_) / seconds;
    framesPerSecond_ = static_cast<double>(frames_) / seconds;
    updates_ = 0;
    frames_ = 0;
    windowStart_ = now;
    return true;
}

} // namespace pixie
