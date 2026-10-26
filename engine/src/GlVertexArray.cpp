#include "pixie/GlVertexArray.hpp"

#include <cassert>
#include <utility>

namespace pixie {

GlVertexArray::GlVertexArray(GlVertexArray&& other) noexcept : id_(std::exchange(other.id_, 0)) {}

GlVertexArray& GlVertexArray::operator=(GlVertexArray&& other) noexcept {
    if (this != &other) {
        assert(id_ == 0 && "Destroy the vertex array before replacing it");
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}

GlVertexArray::~GlVertexArray() {
    assert(id_ == 0 && "Destroy the vertex array while the GL context is current");
}

void GlVertexArray::Create() {
    Destroy();
    glCreateVertexArrays(1, &id_);
}

void GlVertexArray::Destroy() {
    if (id_ != 0) {
        glDeleteVertexArrays(1, &id_);
        id_ = 0;
    }
}

void GlVertexArray::BindBuffer(GLuint binding, GLuint buffer, std::size_t stride, bool perInstance) {
    glVertexArrayVertexBuffer(id_, binding, buffer, 0, static_cast<GLsizei>(stride));
    glVertexArrayBindingDivisor(id_, binding, perInstance ? 1 : 0);
}

void GlVertexArray::Attribute(GLuint location, GLuint binding, GLint components, GLenum type,
                              std::size_t offset, bool normalized) {
    glEnableVertexArrayAttrib(id_, location);
    glVertexArrayAttribFormat(id_, location, components, type, normalized ? GL_TRUE : GL_FALSE,
                              static_cast<GLuint>(offset));
    glVertexArrayAttribBinding(id_, location, binding);
}

void GlVertexArray::IntegerAttribute(GLuint location, GLuint binding, GLint components, GLenum type,
                                     std::size_t offset) {
    glEnableVertexArrayAttrib(id_, location);
    glVertexArrayAttribIFormat(id_, location, components, type, static_cast<GLuint>(offset));
    glVertexArrayAttribBinding(id_, location, binding);
}

} // namespace pixie
