#include "GlDebugOutput.hpp"

#include "pixie/render/gl/Gl.hpp"
#include "pixie/core/Log.hpp"

#include <cstring>
#include <format>
#include <string_view>

namespace pixie {

namespace {

void GLAPIENTRY OnGlDebugMessage(GLenum, GLenum type, GLuint id, GLenum severity, GLsizei length,
                               const GLchar* message, const void*) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }
    const auto size = length >= 0 ? static_cast<std::size_t>(length) : std::strlen(message);
    const bool isError = type == GL_DEBUG_TYPE_ERROR || severity == GL_DEBUG_SEVERITY_HIGH;
    Log(isError ? LogLevel::Error : LogLevel::Warning,
        std::format("GL debug {}: {}", id, std::string_view(message, size)));
}

} // namespace

void InstallGlDebugOutput() {
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if ((flags & GL_CONTEXT_FLAG_DEBUG_BIT) == 0) {
        LogInfo("GL debug output: off (not a debug context)");
        return;
    }
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OnGlDebugMessage, nullptr);
    LogInfo("GL debug output: on");
}

} // namespace pixie
