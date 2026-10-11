#include "Shader.hpp"

#include "pixie/Log.hpp"

#include <format>
#include <string>

namespace {

GLuint CompileStage(GLenum stage, std::string_view source) {
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
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength > 0 ? logLength : 1), '\0');
    glGetShaderInfoLog(shader, logLength, nullptr, log.data());
    pixie::LogError(std::format("{} shader: {}",
                                stage == GL_VERTEX_SHADER ? "Vertex" : "Fragment", log));
    glDeleteShader(shader);
    return 0;
}

} // namespace

GLuint CompileProgram(std::string_view vertexSource, std::string_view fragmentSource) {
    const GLuint vertex = CompileStage(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = CompileStage(GL_FRAGMENT_SHADER, fragmentSource);
    if (vertex == 0 || fragment == 0) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok == GL_TRUE) {
        return program;
    }
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::string log(static_cast<std::size_t>(logLength > 0 ? logLength : 1), '\0');
    glGetProgramInfoLog(program, logLength, nullptr, log.data());
    pixie::LogError(std::format("Program link: {}", log));
    glDeleteProgram(program);
    return 0;
}
