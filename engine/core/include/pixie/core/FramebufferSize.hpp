#pragma once

namespace pixie {

/// Dimensions in physical framebuffer pixels. A zero extent can denote an unavailable framebuffer.
struct FramebufferSize {
    int width = 0;
    int height = 0;
};

} // namespace pixie
