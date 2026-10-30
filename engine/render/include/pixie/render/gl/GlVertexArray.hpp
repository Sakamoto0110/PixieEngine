#pragma once

#include "pixie/render/Export.hpp"

#include "pixie/render/gl/Gl.hpp"

#include <cstddef>

namespace pixie {

// A GL vertex array object, configured with direct state access. Owns it: Destroy it while the
// context is still current (in App::Stop).
class GlVertexArray {
public:
    GlVertexArray() = default;
    GlVertexArray(const GlVertexArray&) = delete;
    GlVertexArray& operator=(const GlVertexArray&) = delete;
    PIXIE_RENDER_API GlVertexArray(GlVertexArray&& other) noexcept;
    PIXIE_RENDER_API GlVertexArray& operator=(GlVertexArray&& other) noexcept;
    PIXIE_RENDER_API ~GlVertexArray();

    PIXIE_RENDER_API void Create();
    PIXIE_RENDER_API void Destroy();

    // Reads binding point `binding` from `buffer`, one element of `stride` bytes per vertex, or per
    // instance when `perInstance` is set.
    PIXIE_RENDER_API void BindBuffer(GLuint binding, GLuint buffer, std::size_t stride, bool perInstance);
    // Attribute `location` reads `components` values of `type` at `offset` within the element of
    // `binding`. Integer types are normalized to [0, 1] when `normalized` is set.
    PIXIE_RENDER_API void Attribute(GLuint location, GLuint binding, GLint components, GLenum type,
                   std::size_t offset, bool normalized = false);
    // Attribute `location` reads `components` integers of `type` at `offset`, which the shader
    // receives as integers (an `int` or `uint` input), not converted to floats.
    PIXIE_RENDER_API void IntegerAttribute(GLuint location, GLuint binding, GLint components, GLenum type, std::size_t offset);

    GLuint Id() const { return id_; }

private:
    GLuint id_ = 0;
};

} // namespace pixie
