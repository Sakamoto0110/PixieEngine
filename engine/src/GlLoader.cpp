#include "pixie/GlLoader.hpp"

#include "GlDebugOutput.hpp"
#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"

#include <format>

namespace pixie {

namespace {

const char* GlString(GLenum name) {
    const auto* value = reinterpret_cast<const char*>(glGetString(name));
    return value != nullptr ? value : "(null)";
}

} // namespace

bool LoadGl() {
    // Under the core profile, GLEW must not rely on the removed extension string.
    glewExperimental = GL_TRUE;
    const GLenum result = glewInit();
    if (result != GLEW_OK) {
        LogError(std::format("GLEW: {}",
                             reinterpret_cast<const char*>(glewGetErrorString(result))));
        return false;
    }
    // glewInit leaves a spurious GL_INVALID_ENUM under the core profile; clear it so the
    // debug output does not report an error that is not ours.
    while (glGetError() != GL_NO_ERROR) {
    }
    LogInfo(std::format("GL_VERSION: {}", GlString(GL_VERSION)));
    LogInfo(std::format("GL_RENDERER: {}", GlString(GL_RENDERER)));
    LogInfo(std::format("GL_VENDOR: {}", GlString(GL_VENDOR)));
    InstallGlDebugOutput();
    return true;
}

} // namespace pixie
