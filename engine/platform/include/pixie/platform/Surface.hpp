#pragma once

#include "pixie/core/FramebufferSize.hpp"

namespace pixie {

// A context supplied by a platform or host. Render consumes only the current context and size.
class Surface {
public:
    virtual ~Surface() = default;
    virtual void MakeCurrent() const = 0;
    virtual FramebufferSize GetFramebufferSize() const = 0;
};

} // namespace pixie
