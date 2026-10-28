#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/render/gl/Gl.hpp"

#include <cstddef>
#include <span>

namespace pixie {

// A GL buffer object. Owns it: Destroy it while the context is still current (in App::Stop).
class GlBuffer {
public:
    GlBuffer() = default;
    GlBuffer(const GlBuffer&) = delete;
    GlBuffer& operator=(const GlBuffer&) = delete;
    PIXIE_RENDER_API GlBuffer(GlBuffer&& other) noexcept;
    PIXIE_RENDER_API GlBuffer& operator=(GlBuffer&& other) noexcept;
    PIXIE_RENDER_API ~GlBuffer();

    PIXIE_RENDER_API void Create();
    PIXIE_RENDER_API void Destroy();

    // Replaces the whole content, for data rewritten every frame. The previous storage is orphaned,
    // so the GPU may keep reading it while the new one fills.
    PIXIE_RENDER_API void Upload(std::span<const std::byte> data);

    template<typename T>
    void Upload(std::span<const T> items) {
        Upload(std::as_bytes(items));
    }

    GLuint Id() const { return id_; }

private:
    GLuint id_ = 0;
};

} // namespace pixie
