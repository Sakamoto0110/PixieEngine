#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/render/gl/Gl.hpp"
#include "pixie/core/FramebufferSize.hpp"

namespace pixie {

// Operations on the current context and currently bound framebuffer.
PIXIE_RENDER_API void ClearColor(float red, float green, float blue, float alpha = 1.0f);
PIXIE_RENDER_API void SetViewport(FramebufferSize size);


// An offscreen framebuffer with one RGBA8 color texture, for drawing into a texture that something
// else displays: an editor's viewport, a capture, a test. The texture's first row is the bottom of
// the image, as OpenGL has it; a host that draws with Y down (Dear ImGui, for example) flips it
// with its texture coordinates.
//
// Owns its GL objects: Destroy it while the context is still current.
class RenderTarget {
public:
    RenderTarget() = default;
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;
    PIXIE_RENDER_API ~RenderTarget();

    // Creates or recreates the framebuffer at `size`; returns false, after logging, if it is
    // incomplete.
    PIXIE_RENDER_API bool Create(FramebufferSize size);
    // Recreates the framebuffer only if `size` differs from the current one.
    PIXIE_RENDER_API bool Resize(FramebufferSize size);
    PIXIE_RENDER_API void Destroy();

    // Draws into the target from now on, with the viewport covering it.
    PIXIE_RENDER_API void Bind() const;
    // Draws into the window's framebuffer again.
    PIXIE_RENDER_API static void BindDefault();

    GLuint ColorTexture() const { return texture_; }
    GLuint Framebuffer() const { return framebuffer_; }
    FramebufferSize Size() const { return size_; }
    bool IsValid() const { return framebuffer_ != 0; }

private:
    GLuint framebuffer_ = 0;
    GLuint texture_ = 0;
    FramebufferSize size_;
};

} // namespace pixie
