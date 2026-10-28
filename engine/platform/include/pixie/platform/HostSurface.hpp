#pragma once

#include "pixie/platform/Surface.hpp"

namespace pixie {

// The caller owns the context and must make it current before every native rendering call.
// The size is the requested viewport, not necessarily the host window's framebuffer.
class HostSurface final : public Surface {
public:
    explicit HostSurface(FramebufferSize size) : size_(size) {}
    void MakeCurrent() const override {} // Preserve the host's already-current context.
    FramebufferSize GetFramebufferSize() const override { return size_; }
    void Resize(FramebufferSize size) { size_ = size; }

private:
    FramebufferSize size_;
};

} // namespace pixie
