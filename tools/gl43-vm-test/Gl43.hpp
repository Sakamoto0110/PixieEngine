#pragma once

#include <GL/glew.h>
#include <string>

// Private bindings for the existing renderer's 2D textures and draw framebuffers.
namespace pixie::vm_gl43 {

inline GLint Current(GLenum parameter) {
    GLint value = 0;
    glGetIntegerv(parameter, &value);
    return value;
}

template <typename Operation>
void WithTexture(GLuint texture, Operation operation) {
    const auto previous = Current(GL_TEXTURE_BINDING_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    operation();
    glBindTexture(GL_TEXTURE_2D, previous);
}

template <typename Operation>
void WithDrawFramebuffer(GLuint framebuffer, Operation operation) {
    const auto previous = Current(GL_DRAW_FRAMEBUFFER_BINDING);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
    operation();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previous);
}

template <typename Operation>
void WithVertexArray(GLuint array, Operation operation) {
    const auto previous = Current(GL_VERTEX_ARRAY_BINDING);
    glBindVertexArray(array);
    operation();
    glBindVertexArray(previous);
}

inline void ShaderSource(GLuint shader, GLsizei count, const GLchar* const* text, const GLint* lengths) {
    // ShaderProgram submits one complete source at a time.
    if (count == 1 && lengths && lengths[0] >= 0) {
        std::string source(text[0], static_cast<std::size_t>(lengths[0]));
        if (source.starts_with("#version 450 core")) {
            source.replace(9, 3, "430");
            const auto* data = source.data();
            glShaderSource(shader, 1, &data, lengths);
            return;
        }
    }
    glShaderSource(shader, count, text, lengths);
}

inline void NamedBufferData(GLuint buffer, GLsizeiptr size, const void* data, GLenum usage) {
    const auto previous = Current(GL_COPY_WRITE_BUFFER_BINDING);
    glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
    glBufferData(GL_COPY_WRITE_BUFFER, size, data, usage);
    glBindBuffer(GL_COPY_WRITE_BUFFER, previous);
}

inline void CreateTextures(GLenum target, GLsizei count, GLuint* textures) {
    glGenTextures(count, textures);
    const auto previous = Current(GL_TEXTURE_BINDING_2D);
    for (GLsizei index = 0; index < count; ++index) {
        glBindTexture(target, textures[index]);
    }
    glBindTexture(target, previous);
}

inline void TextureStorage2D(GLuint texture, GLsizei levels, GLenum format, GLsizei width, GLsizei height) {
    WithTexture(texture, [&] { glTexStorage2D(GL_TEXTURE_2D, levels, format, width, height); });
}

inline void TextureParameteri(GLuint texture, GLenum name, GLint value) {
    WithTexture(texture, [&] { glTexParameteri(GL_TEXTURE_2D, name, value); });
}

inline void TextureSubImage2D(GLuint texture, GLint level, GLint x, GLint y, GLsizei width,
                             GLsizei height, GLenum format, GLenum type, const void* data) {
    WithTexture(texture, [&] { glTexSubImage2D(GL_TEXTURE_2D, level, x, y, width, height, format, type, data); });
}

inline void GenerateTextureMipmap(GLuint texture) {
    WithTexture(texture, [] { glGenerateMipmap(GL_TEXTURE_2D); });
}

inline void BindTextureUnit(GLuint unit, GLuint texture) {
    const auto previous = Current(GL_ACTIVE_TEXTURE);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(previous);
}

inline void NamedFramebufferTexture(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level) {
    WithDrawFramebuffer(framebuffer, [&] { glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, texture, level); });
}

inline GLenum CheckNamedFramebufferStatus(GLuint framebuffer, GLenum target) {
    // The renderer checks only GL_FRAMEBUFFER, which is equivalent to GL_DRAW_FRAMEBUFFER here.
    const GLenum binding = target == GL_READ_FRAMEBUFFER ? GL_READ_FRAMEBUFFER_BINDING : GL_DRAW_FRAMEBUFFER_BINDING;
    const GLenum bindTarget = target == GL_READ_FRAMEBUFFER ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
    const auto previous = Current(binding);
    glBindFramebuffer(bindTarget, framebuffer);
    const GLenum status = glCheckFramebufferStatus(bindTarget);
    glBindFramebuffer(bindTarget, previous);
    return status;
}

inline void NamedFramebufferDrawBuffer(GLuint framebuffer, GLenum drawBuffer) {
    WithDrawFramebuffer(framebuffer, [&] { glDrawBuffer(drawBuffer); });
}

inline void NamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei count, const GLenum* drawBuffers) {
    WithDrawFramebuffer(framebuffer, [&] { glDrawBuffers(count, drawBuffers); });
}

inline void ClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer, GLint index, const GLfloat* value) {
    WithDrawFramebuffer(framebuffer, [&] { glClearBufferfv(buffer, index, value); });
}

inline void VertexArrayVertexBuffer(GLuint array, GLuint binding, GLuint buffer, GLintptr offset, GLsizei stride) {
    WithVertexArray(array, [&] { glBindVertexBuffer(binding, buffer, offset, stride); });
}

inline void VertexArrayBindingDivisor(GLuint array, GLuint binding, GLuint divisor) {
    WithVertexArray(array, [&] { glVertexBindingDivisor(binding, divisor); });
}

inline void EnableVertexArrayAttrib(GLuint array, GLuint attribute) {
    WithVertexArray(array, [&] { glEnableVertexAttribArray(attribute); });
}

inline void VertexArrayAttribFormat(GLuint array, GLuint attribute, GLint size, GLenum type,
                                   GLboolean normalized, GLuint offset) {
    WithVertexArray(array, [&] { glVertexAttribFormat(attribute, size, type, normalized, offset); });
}

inline void VertexArrayAttribIFormat(GLuint array, GLuint attribute, GLint size, GLenum type, GLuint offset) {
    WithVertexArray(array, [&] { glVertexAttribIFormat(attribute, size, type, offset); });
}

inline void VertexArrayAttribBinding(GLuint array, GLuint attribute, GLuint binding) {
    WithVertexArray(array, [&] { glVertexAttribBinding(attribute, binding); });
}

inline void CreateQueries(GLenum, GLsizei count, GLuint* queries) {
    glGenQueries(count, queries);
}

} // namespace pixie::vm_gl43

#undef glShaderSource
#undef glCreateBuffers
#undef glNamedBufferData
#undef glCreateTextures
#undef glTextureStorage2D
#undef glTextureParameteri
#undef glTextureSubImage2D
#undef glGenerateTextureMipmap
#undef glBindTextureUnit
#undef glCreateFramebuffers
#undef glNamedFramebufferTexture
#undef glCheckNamedFramebufferStatus
#undef glNamedFramebufferDrawBuffer
#undef glNamedFramebufferDrawBuffers
#undef glClearNamedFramebufferfv
#undef glCreateVertexArrays
#undef glVertexArrayVertexBuffer
#undef glVertexArrayBindingDivisor
#undef glEnableVertexArrayAttrib
#undef glVertexArrayAttribFormat
#undef glVertexArrayAttribIFormat
#undef glVertexArrayAttribBinding
#undef glCreateSamplers
#undef glCreateQueries

#define glShaderSource ::pixie::vm_gl43::ShaderSource
#define glCreateBuffers glGenBuffers
#define glNamedBufferData ::pixie::vm_gl43::NamedBufferData
#define glCreateTextures ::pixie::vm_gl43::CreateTextures
#define glTextureStorage2D ::pixie::vm_gl43::TextureStorage2D
#define glTextureParameteri ::pixie::vm_gl43::TextureParameteri
#define glTextureSubImage2D ::pixie::vm_gl43::TextureSubImage2D
#define glGenerateTextureMipmap ::pixie::vm_gl43::GenerateTextureMipmap
#define glBindTextureUnit ::pixie::vm_gl43::BindTextureUnit
#define glCreateFramebuffers glGenFramebuffers
#define glNamedFramebufferTexture ::pixie::vm_gl43::NamedFramebufferTexture
#define glCheckNamedFramebufferStatus ::pixie::vm_gl43::CheckNamedFramebufferStatus
#define glNamedFramebufferDrawBuffer ::pixie::vm_gl43::NamedFramebufferDrawBuffer
#define glNamedFramebufferDrawBuffers ::pixie::vm_gl43::NamedFramebufferDrawBuffers
#define glClearNamedFramebufferfv ::pixie::vm_gl43::ClearNamedFramebufferfv
#define glCreateVertexArrays glGenVertexArrays
#define glVertexArrayVertexBuffer ::pixie::vm_gl43::VertexArrayVertexBuffer
#define glVertexArrayBindingDivisor ::pixie::vm_gl43::VertexArrayBindingDivisor
#define glEnableVertexArrayAttrib ::pixie::vm_gl43::EnableVertexArrayAttrib
#define glVertexArrayAttribFormat ::pixie::vm_gl43::VertexArrayAttribFormat
#define glVertexArrayAttribIFormat ::pixie::vm_gl43::VertexArrayAttribIFormat
#define glVertexArrayAttribBinding ::pixie::vm_gl43::VertexArrayAttribBinding
#define glCreateSamplers glGenSamplers
#define glCreateQueries ::pixie::vm_gl43::CreateQueries
