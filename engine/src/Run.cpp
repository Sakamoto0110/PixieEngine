#include "pixie/Run.hpp"

#include "FrameLimiter.hpp"
#include "GlDebugOutput.hpp"
#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"
#include "pixie/RateMeter.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <format>

namespace pixie {

namespace {

using Clock = std::chrono::steady_clock;

// A frame longer than this is treated as this long, so a stall (a dragged window, a breakpoint)
// does not trigger a burst of catch-up updates.
constexpr double kMaxFrameDelta = 0.25;

double Seconds(Clock::duration duration) {
    return std::chrono::duration<double>(duration).count();
}

void OnGlfwError(int code, const char* description) {
    LogError(std::format("GLFW {}: {}", code, description));
}

const char* GlString(GLenum name) {
    const auto* value = reinterpret_cast<const char*>(glGetString(name));
    return value != nullptr ? value : "(null)";
}

// Terminates GLFW on every exit path; it also destroys any window still open.
struct GlfwSession {
    GlfwSession() = default;
    GlfwSession(const GlfwSession&) = delete;
    GlfwSession& operator=(const GlfwSession&) = delete;
    ~GlfwSession() { glfwTerminate(); }
};

GLFWwindow* CreateContextWindow(const RunConfig& config) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    return glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
}

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
    return true;
}

} // namespace

int Run(App& app, const RunConfig& config) {
    glfwSetErrorCallback(OnGlfwError);
    if (glfwInit() != GLFW_TRUE) {
        LogError("GLFW initialization failed");
        return 1;
    }
    GlfwSession session;

    GLFWwindow* handle = CreateContextWindow(config);
    if (handle == nullptr) {
        LogError("Could not create a window with an OpenGL 4.5 core context");
        return 1;
    }
    glfwMakeContextCurrent(handle);
    if (!LoadGl()) {
        return 1;
    }
    InstallGlDebugOutput();
    glfwSwapInterval(config.vsync ? 1 : 0);
    LogInfo(std::format("Update rate: {} Hz; frame cap: {}; vsync: {}", config.updateRate,
                        config.frameRateCap > 0.0 ? std::format("{} Hz", config.frameRateCap)
                                                  : std::string("none"),
                        config.vsync ? "on" : "off"));

    Window window(handle);
    if (!app.Start(window)) {
        LogError("App::Start failed");
        app.Stop();
        return 1;
    }

    const double step = 1.0 / config.updateRate;
    double accumulator = 0.0;
    double simulated = 0.0;
    std::uint64_t updateIndex = 0;
    std::uint64_t frameIndex = 0;

    const auto start = Clock::now();
    auto previous = start;
    FrameLimiter limiter(config.frameRateCap);
    RateMeter meter(start);

    while (glfwWindowShouldClose(handle) == GLFW_FALSE) {
        glfwPollEvents();

        const auto now = Clock::now();
        const double frameDelta = std::min(Seconds(now - previous), kMaxFrameDelta);
        previous = now;

        accumulator += frameDelta;
        while (accumulator >= step) {
            app.Update(FrameTime{step, 0.0, simulated, updateIndex});
            simulated += step;
            accumulator -= step;
            ++updateIndex;
            meter.CountUpdate();
        }

        const FramebufferSize size = window.GetFramebufferSize();
        if (size.width > 0 && size.height > 0) {
            glViewport(0, 0, size.width, size.height);
            app.Render(FrameTime{frameDelta, accumulator / step, simulated, frameIndex});
            glfwSwapBuffers(handle);
            ++frameIndex;
            meter.CountFrame();
        } else {
            // Minimized: there is nothing to draw, so do not spin.
            glfwWaitEventsTimeout(0.05);
        }

        limiter.Wait();

        if (meter.Sample(Clock::now())) {
            const auto rates = std::format("{:.1f} fps | {:.1f} ups", meter.FramesPerSecond(),
                                           meter.UpdatesPerSecond());
            LogInfo(rates);
            window.SetTitle(std::format("{} | {}", config.title, rates));
        }
        if (config.quitAfterSeconds > 0.0 && Seconds(now - start) >= config.quitAfterSeconds) {
            glfwSetWindowShouldClose(handle, GLFW_TRUE);
        }
    }

    app.Stop();
    glfwDestroyWindow(handle);
    return 0;
}

} // namespace pixie
