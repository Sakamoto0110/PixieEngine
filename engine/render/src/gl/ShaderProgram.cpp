#include "pixie/render/gl/ShaderProgram.hpp"

#include "pixie/core/Log.hpp"

#include <cassert>
#include <format>
#include <string>
#include <utility>

namespace pixie {

namespace {

std::string InfoLog(GLuint object, bool isProgram) {
    GLint length = 0;
    if (isProgram) {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &length);
    } else {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &length);
    }
    std::string log(static_cast<std::size_t>(length > 0 ? length : 1), '\0');
    if (isProgram) {
        glGetProgramInfoLog(object, length, nullptr, log.data());
    } else {
        glGetShaderInfoLog(object, length, nullptr, log.data());
    }
    return log;
}

GLuint CompileStage(std::string_view name, GLenum stage, std::string_view source) {
    const GLuint shader = glCreateShader(stage);
    const GLchar* text = source.data();
    const auto length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &text, &length);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_TRUE) {
        return shader;
    }
    LogError(std::format("{}: {} shader: {}", name,
                         stage == GL_VERTEX_SHADER ? "vertex" : "fragment", InfoLog(shader, false)));
    glDeleteShader(shader);
    return 0;
}

} // namespace

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : id_(std::exchange(other.id_, 0)) {}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) {
        assert(id_ == 0 && "Destroy the program before replacing it");
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}

ShaderProgram::~ShaderProgram() {
    assert(id_ == 0 && "Destroy the program while the GL context is current");
}

bool ShaderProgram::Create(std::string_view name, std::string_view vertexSource,
                           std::string_view fragmentSource) {
    Destroy();
    const GLuint vertex = CompileStage(name, GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = CompileStage(name, GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return false;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE) {
        LogError(std::format("{}: link: {}", name, InfoLog(program, true)));
        glDeleteProgram(program);
        return false;
    }
    id_ = program;
    return true;
}

void ShaderProgram::Destroy() {
    if (id_ != 0) {
        glDeleteProgram(id_);
        id_ = 0;
    }
}

} // namespace pixie
