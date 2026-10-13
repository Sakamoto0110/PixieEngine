#include "pixie/Run.hpp"

#include "FrameLimiter.hpp"
#include "GlDebugOutput.hpp"
#include "pixie/Gl.hpp"
#include "pixie/Log.hpp"
#include "pixie/RateMeter.hpp"
#include "pixie/Screenshot.hpp"

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

Input& InputOf(GLFWwindow* handle) {
    return *static_cast<Input*>(glfwGetWindowUserPointer(handle));
}

// GLFW reports the cursor in window coordinates; the engine works in framebuffer pixels, which
// differ when the system scales the display.
pxPoint CursorInPixels(GLFWwindow* handle, double x, double y) {
    int windowWidth = 0;
    int windowHeight = 0;
    int pixelWidth = 0;
    int pixelHeight = 0;
    glfwGetWindowSize(handle, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(handle, &pixelWidth, &pixelHeight);
    const double sx = windowWidth > 0 ? static_cast<double>(pixelWidth) / windowWidth : 1.0;
    const double sy = windowHeight > 0 ? static_cast<double>(pixelHeight) / windowHeight : 1.0;
    return pxPoint(x * sx, y * sy);
}

void ConnectInput(GLFWwindow* handle, Input& input) {
    glfwSetWindowUserPointer(handle, &input);
    glfwSetKeyCallback(handle, [](GLFWwindow* window, int key, int, int action, int) {
        InputOf(window).SetKey(key, action != GLFW_RELEASE);
    });
    glfwSetMouseButtonCallback(handle, [](GLFWwindow* window, int button, int action, int) {
        InputOf(window).SetMouseButton(button, action != GLFW_RELEASE);
    });
    glfwSetCursorPosCallback(handle, [](GLFWwindow* window, double x, double y) {
        InputOf(window).SetCursor(CursorInPixels(window, x, y));
    });
    glfwSetScrollCallback(handle, [](GLFWwindow* window, double x, double y) {
        InputOf(window).AddScroll(pxVec2(x, y));
    });
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(handle, &x, &y);
    input.SetCursor(CursorInPixels(handle, x, y));
    input.EndUpdate();
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

    Input input;
    ConnectInput(handle, input);

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
            app.Update(FrameTime{step, 0.0, simulated, updateIndex}, input);
            input.EndUpdate();
            simulated += step;
            accumulator -= step;
            ++updateIndex;
            meter.CountUpdate();
        }

        const bool quitting =
            config.quitAfterSeconds > 0.0 && Seconds(now - start) >= config.quitAfterSeconds;
        const FramebufferSize size = window.GetFramebufferSize();
        if (size.width > 0 && size.height > 0) {
            glViewport(0, 0, size.width, size.height);
            app.Render(FrameTime{frameDelta, accumulator / step, simulated, frameIndex}, input);
            if (quitting && !config.screenshotPath.empty()) {
                SaveFramebufferPng(config.screenshotPath, size);
            }
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
        if (quitting) {
            glfwSetWindowShouldClose(handle, GLFW_TRUE);
        }
    }

    app.Stop();
    glfwDestroyWindow(handle);
    return 0;
}

} // namespace pixie
