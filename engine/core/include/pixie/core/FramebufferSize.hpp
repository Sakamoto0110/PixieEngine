#pragma once

namespace pixie {

// Dimensions in physical framebuffer pixels, independent of a window or rendering API.
struct FramebufferSize {
    int width = 0;
    int height = 0;
};

} // namespace pixie
