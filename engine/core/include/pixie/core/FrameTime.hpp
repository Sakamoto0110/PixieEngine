#pragma once

#include <cstdint>

namespace pixie {

// Time handed to App::Update and App::Render. There is no global timestep.
struct FrameTime {
    // Seconds covered by this call: the fixed step in Update, the frame delta in Render.
    double delta = 0.0;
    // Render only: fraction between updates; one draws the current state without interpolation.
    double alpha = 0.0;
    // Simulated seconds completed before this call.
    double elapsed = 0.0;
    // Index of this call: the update index in Update, the frame index in Render.
    std::uint64_t index = 0;
};

} // namespace pixie
