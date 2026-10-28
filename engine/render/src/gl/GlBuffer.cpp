#include "pixie/render/gl/GlBuffer.hpp"

#include <cassert>
#include <utility>

namespace pixie {

GlBuffer::GlBuffer(GlBuffer&& other) noexcept : id_(std::exchange(other.id_, 0)) {}

GlBuffer& GlBuffer::operator=(GlBuffer&& other) noexcept {
    if (this != &other) {
        assert(id_ == 0 && "Destroy the buffer before replacing it");
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}

GlBuffer::~GlBuffer() {
    assert(id_ == 0 && "Destroy the buffer while the GL context is current");
}

void GlBuffer::Create() {
    Destroy();
    glCreateBuffers(1, &id_);
}

void GlBuffer::Destroy() {
    if (id_ != 0) {
        glDeleteBuffers(1, &id_);
        id_ = 0;
    }
}

void GlBuffer::Upload(std::span<const std::byte> data) {
    glNamedBufferData(id_, static_cast<GLsizeiptr>(data.size()), data.data(), GL_STREAM_DRAW);
}

} // namespace pixie
