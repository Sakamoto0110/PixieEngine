#pragma once

#include "pixie/core/FrameTime.hpp"

#include <algorithm>
#include <cstdint>

namespace pixie {

// Turns frames of wall time into fixed update steps: the accumulator behind pixie::Run's loop, for
// any host that drives the engine itself (an editor viewport, a tool, a test). Each Advance adds a
// frame's time and runs every step that became due; RenderTime then gives the frame's FrameTime,
// with alpha the fraction of a step left over.
class FixedStepper {
public:
    // A frame longer than this counts as this long, so a stall (a dragged window, a breakpoint)
    // does not trigger a burst of catch-up steps.
    static constexpr double kMaxFrameDelta = 0.25;

    /// Supply a finite positive update rate, in steps per second.
    explicit FixedStepper(double updateRate = 60.0) : step_(1.0 / updateRate) {}

    double StepSeconds() const { return step_; }

    // Adds `frameDelta` seconds and calls update(FrameTime) once per fixed step now due.
    template<typename Update>
    void Advance(double frameDelta, Update&& update) {
        accumulator_ += std::clamp(frameDelta, 0.0, kMaxFrameDelta);
        while (accumulator_ >= step_) {
            update(FrameTime{step_, 0.0, simulated_, steps_});
            simulated_ += step_;
            accumulator_ -= step_;
            ++steps_;
        }
    }

    /// Reports leftover time divided by the fixed step. Passes frame delta and index through.
    FrameTime RenderTime(double frameDelta, std::uint64_t frameIndex) const {
        return FrameTime{frameDelta, accumulator_ / step_, simulated_, frameIndex};
    }

    /// Clears leftover time while retaining completed step count and simulated seconds.
    void Drop() { accumulator_ = 0.0; }

    std::uint64_t Steps() const { return steps_; }
    double Simulated() const { return simulated_; }

private:
    double step_;
    double accumulator_ = 0.0;
    double simulated_ = 0.0;
    std::uint64_t steps_ = 0;
};

} // namespace pixie
