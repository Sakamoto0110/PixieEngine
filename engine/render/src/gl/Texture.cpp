#include "pixie/render/gl/Texture.hpp"

#include "pixie/core/Image.hpp"

#include <bit>
#include <cassert>
#include <utility>

namespace pixie {

Texture::Texture(Texture&& other) noexcept
    : id_(std::exchange(other.id_, 0)), width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)) {}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        assert(id_ == 0 && "Destroy the texture before replacing it");
        id_ = std::exchange(other.id_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
    }
    return *this;
}

Texture::~Texture() {
    assert(id_ == 0 && "Destroy the texture while the GL context is current");
}

bool Texture::Create(const Image& image) {
    Destroy();
    if (image.width <= 0 || image.height <= 0 || image.pixels.empty()) {
        return false;
    }
    const auto largest = static_cast<unsigned>(image.width > image.height ? image.width : image.height);
    const auto levels = static_cast<GLsizei>(std::bit_width(largest));
    glCreateTextures(GL_TEXTURE_2D, 1, &id_);
    glTextureStorage2D(id_, levels, GL_RGBA8, image.width, image.height);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTextureSubImage2D(id_, 0, 0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());
    glGenerateTextureMipmap(id_);
    width_ = image.width;
    height_ = image.height;
    return true;
}

void Texture::Destroy() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

} // namespace pixie
