#pragma once

#include "pixie/platform/Surface.hpp"

namespace pixie {

/// Borrows the host's already-current context for native rendering and teardown.
/// The stored size is the requested viewport in physical pixels.
class HostSurface final : public Surface {
public:
    explicit HostSurface(FramebufferSize size) : size_(size) {}
    void MakeCurrent() const override {} // Preserve the host's already-current context.
    FramebufferSize GetFramebufferSize() const override { return size_; }
    /// Records a new requested size. The host still owns any underlying resize.
    void Resize(FramebufferSize size) { size_ = size; }

private:
    FramebufferSize size_;
};

} // namespace pixie
