#include "pixie/render/RenderTarget.hpp"

#include "pixie/core/Log.hpp"

#include <cassert>
#include <format>

namespace pixie {

void ClearColor(float red, float green, float blue, float alpha) {
    glClearColor(red, green, blue, alpha);
    glClear(GL_COLOR_BUFFER_BIT);
}

void SetViewport(FramebufferSize size) {
    glViewport(0, 0, size.width, size.height);
}


RenderTarget::~RenderTarget() {
    assert(framebuffer_ == 0 && "Destroy the render target while the GL context is current");
}

bool RenderTarget::Create(FramebufferSize size) {
    Destroy();
    if (size.width <= 0 || size.height <= 0) {
        LogError(std::format("RenderTarget: invalid size {}x{}", size.width, size.height));
        return false;
    }
    glCreateTextures(GL_TEXTURE_2D, 1, &texture_);
    glTextureStorage2D(texture_, 1, GL_RGBA8, size.width, size.height);
    glTextureParameteri(texture_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &framebuffer_);
    glNamedFramebufferTexture(framebuffer_, GL_COLOR_ATTACHMENT0, texture_, 0);
    const GLenum status = glCheckNamedFramebufferStatus(framebuffer_, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LogError(std::format("RenderTarget: framebuffer incomplete (0x{:X})", status));
        Destroy();
        return false;
    }
    size_ = size;
    return true;
}

bool RenderTarget::Resize(FramebufferSize size) {
    if (IsValid() && size.width == size_.width && size.height == size_.height) {
        return true;
    }
    return Create(size);
}

void RenderTarget::Destroy() {
    if (framebuffer_ != 0) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }
    if (texture_ != 0) {
        glDeleteTextures(1, &texture_);
        texture_ = 0;
    }
    size_ = {};
}

void RenderTarget::Bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, size_.width, size_.height);
}

void RenderTarget::BindDefault() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace pixie
