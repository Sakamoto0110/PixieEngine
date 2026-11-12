#pragma once

#include "pixie/core/FramebufferSize.hpp"

namespace pixie {

/// Context activation and physical framebuffer dimensions supplied by the platform or host.
class Surface {
public:
    virtual ~Surface() = default;
    virtual void MakeCurrent() const = 0;
    virtual FramebufferSize GetFramebufferSize() const = 0;
};

} // namespace pixie
