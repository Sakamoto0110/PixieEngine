#include "StepTimer.hpp"

#include "pixie/Log.hpp"

#include <algorithm>
#include <format>

namespace sample {

void StepTimer::End() {
    const auto now = Clock::now();
    const double milliseconds = std::chrono::duration<double, std::milli>(now - start_).count();
    total_ += milliseconds;
    slowest_ = std::max(slowest_, milliseconds);
    ++count_;
    if (now - windowStart_ >= std::chrono::seconds(1)) {
        pixie::LogInfo(std::format("{}: {} steps, {:.3f} ms average, {:.3f} ms slowest", label_,
                                   count_, total_ / count_, slowest_));
        windowStart_ = now;
        total_ = 0.0;
        slowest_ = 0.0;
        count_ = 0;
    }
}

} // namespace sample
