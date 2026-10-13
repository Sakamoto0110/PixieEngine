#pragma once

#include "pixie/Gl.hpp"
#include "pixie/Window.hpp"

namespace pixie {

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
    ~RenderTarget();

    // Creates or recreates the framebuffer at `size`; returns false, after logging, if it is
    // incomplete.
    bool Create(FramebufferSize size);
    // Recreates the framebuffer only if `size` differs from the current one.
    bool Resize(FramebufferSize size);
    void Destroy();

    // Draws into the target from now on, with the viewport covering it.
    void Bind() const;
    // Draws into the window's framebuffer again.
    static void BindDefault();

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
