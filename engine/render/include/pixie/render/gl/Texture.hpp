#pragma once

#include "pixie/render/gl/Gl.hpp"

namespace pixie {

struct Image;

// A 2D RGBA8 texture made from an Image, with a full chain of mipmaps, so it can be drawn smaller
// than it is without aliasing. Its first row is the image's first row (the top), at texture
// coordinate v = 0. Filtering is not the texture's: the SpriteRenderer chooses a sampler per draw.
//
// Owns its GL object: Destroy it while the context is still current.
class Texture {
public:
    Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;
    ~Texture();

    // Creates or recreates the texture from `image`; returns false for an empty image.
    bool Create(const Image& image);
    void Destroy();

    GLuint Id() const { return id_; }
    int Width() const { return width_; }
    int Height() const { return height_; }
    bool IsValid() const { return id_ != 0; }

private:
    GLuint id_ = 0;
    int width_ = 0;
    int height_ = 0;
};

} // namespace pixie
