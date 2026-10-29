#include "pixie/app/Run.hpp"

#include "DebugConsole.hpp"
#include "DebugOverlay.hpp"
#include "pixie/core/FixedStepper.hpp"
#include "pixie/render/RenderTarget.hpp"
#include "pixie/render/gl/GlLoader.hpp"
#include "pixie/core/Log.hpp"
#include "pixie/core/RateMeter.hpp"
#include "pixie/render/Screenshot.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <format>

namespace pixie {

namespace {

using Clock = std::chrono::steady_clock;

double Seconds(Clock::duration duration) {
    return std::chrono::duration<double>(duration).count();
}

struct PlayerInput {
    explicit PlayerInput(const RunConfig& config) : debug(config.vsync, config.frameRateCap, config.telemetry) {}
    Input game;
    DebugConsole debug;
};

PlayerInput& InputOf(GLFWwindow* handle) {
    return *static_cast<PlayerInput*>(glfwGetWindowUserPointer(handle));
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

void ConnectInput(GLFWwindow* handle, PlayerInput& input) {
    glfwSetWindowUserPointer(handle, &input);
    glfwSetKeyCallback(handle, [](GLFWwindow* window, int key, int, int action, int mods) {
        auto& state = InputOf(window);
        const bool wasOpen = state.debug.IsOpen();
        const auto code = static_cast<Key>(key);
        if (!state.debug.KeyEvent(code, action != GLFW_RELEASE, action == GLFW_REPEAT)) {
            // A repeat from a key first pressed in the console must not become a new game press.
            if (action != GLFW_REPEAT || state.game.IsDown(code)) {
                state.game.SetKey(key, action != GLFW_RELEASE);
            }
        }
        if (wasOpen != state.debug.IsOpen()) {
            double x = 0.0, y = 0.0;
            glfwGetCursorPos(window, &x, &y);
            state.game.SetCursor(CursorInPixels(window, x, y));
            state.game.ReleaseAll();
        }
        if (wasOpen && key == GLFW_KEY_V && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL) != 0) {
            if (const char* text = glfwGetClipboardString(window)) state.debug.Paste(text);
        }
    });
    glfwSetCharCallback(handle, [](GLFWwindow* window, unsigned int codepoint) {
        InputOf(window).debug.Character(codepoint);
    });
    glfwSetMouseButtonCallback(handle, [](GLFWwindow* window, int button, int action, int) {
        auto& state = InputOf(window);
        if (!state.debug.IsOpen()) state.game.SetMouseButton(button, action != GLFW_RELEASE);
    });
    glfwSetCursorPosCallback(handle, [](GLFWwindow* window, double x, double y) {
        auto& state = InputOf(window);
        if (!state.debug.IsOpen()) state.game.SetCursor(CursorInPixels(window, x, y));
    });
    glfwSetScrollCallback(handle, [](GLFWwindow* window, double x, double y) {
        auto& state = InputOf(window);
        if (!state.debug.IsOpen()) state.game.AddScroll(pxVec2(x, y));
    });
    glfwSetWindowFocusCallback(handle, [](GLFWwindow* window, int focused) {
        if (focused == GLFW_FALSE) {
            auto& state = InputOf(window);
            state.game.ReleaseAll();
            state.debug.FocusLost();
        }
    });
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(handle, &x, &y);
    input.game.SetCursor(CursorInPixels(handle, x, y));
    input.game.EndUpdate();
}

} // namespace

int Run(App& app, const RunConfig& config) {
    // Callback state outlives the window, including its destruction.
    PlayerInput input(config);
    Window window;
    if (!window.Create(config.width, config.height, config.title)) return 1;
    const Surface& surface = window;
    surface.MakeCurrent();
    GLFWwindow* handle = window.Handle();
#ifdef PIXIE_DEVELOPMENT
    // GLFW uses screen coordinates; capture dimensions are framebuffer pixels.
    for (int attempt = 0; config.exactFramebufferSize && attempt < 3; ++attempt) {
        const auto pixels = surface.GetFramebufferSize();
        if (pixels.width == config.width && pixels.height == config.height) break;
        int width = 0, height = 0;
        glfwGetWindowSize(handle, &width, &height);
        if (pixels.width <= 0 || pixels.height <= 0) break;
        glfwSetWindowSize(handle, std::max(1, width * config.width / pixels.width),
                         std::max(1, height * config.height / pixels.height));
        window.PollEvents();
    }
    if (config.exactFramebufferSize) {
        const auto pixels = surface.GetFramebufferSize();
        if (pixels.width != config.width || pixels.height != config.height) {
            LogError("The requested capture framebuffer size is unavailable");
            return 1;
        }
    }
#endif
    if (!LoadGl()) {
        return 1;
    }
    window.SetVsync(config.vsync);
    LogInfo(std::format("Update rate: {} Hz; frame cap: {}; vsync: {}", config.updateRate,
                        config.frameRateCap > 0.0 ? std::format("{} Hz", config.frameRateCap)
                                                  : std::string("none"),
                        config.vsync ? "on" : "off"));

#ifdef PIXIE_DEVELOPMENT
    if (config.quitAfterSteps) {
        input.game.SetCursor(pxPoint(-1.0, -1.0));
        input.game.EndUpdate();
    } else
#endif
    ConnectInput(handle, input);

    if (!app.Start(window)) {
        LogError("App::Start failed");
        app.Stop();
        return 1;
    }

    DebugOverlay overlay;
    if (!overlay.Create()) {
        app.Stop();
        return 1;
    }

    FixedStepper stepper(config.updateRate);
    std::uint64_t frameIndex = 0;

    const auto start = Clock::now();
    auto previous = start;
    window.SetFrameRateCap(config.frameRateCap);
    RateMeter meter(start);
    bool vsync = config.vsync;
    double frameCap = config.frameRateCap;
    Telemetry telemetry;
    const auto updateMetric = telemetry.RegisterTiming("host.update");
    const auto renderMetric = telemetry.RegisterTiming("host.render");
    const auto observerMetric = telemetry.RegisterTiming("host.observer");
    std::vector<MetricSnapshot> hostMetrics, appMetrics;
    auto nextMetrics = start;

    while (!window.ShouldClose()) {
        window.PollEvents();
        const auto& settings = input.debug.CurrentSettings();
        telemetry.SetEnabled(settings.telemetry.value_or(false));
        if (auto* metrics = app.GetTelemetry(); metrics && settings.telemetry) metrics->SetEnabled(*settings.telemetry);
        if (settings.vsync != vsync) {
            vsync = settings.vsync;
            window.SetVsync(vsync);
            LogInfo(std::format("VSync: {}", vsync ? "on" : "off"));
        }
        if (settings.frameRateCap != frameCap) {
            frameCap = settings.frameRateCap;
            window.SetFrameRateCap(frameCap);
            LogInfo(frameCap > 0 ? std::format("Frame cap: {} Hz", frameCap) : "Frame cap: none");
        }

        const auto now = Clock::now();
        const double frameDelta = Seconds(now - previous);
        previous = now;

        double advanceDelta = frameDelta;
#ifdef PIXIE_DEVELOPMENT
        if (config.quitAfterSteps) {
            advanceDelta = stepper.Steps() < *config.quitAfterSteps ? stepper.StepSeconds() : 0.0;
        }
#endif
        stepper.Advance(advanceDelta, [&](const FrameTime& time) {
            {
                const TelemetryScope measure(telemetry,updateMetric);
                app.Update(time, input.game);
            }
            input.game.EndUpdate();
            meter.CountUpdate();
        });

        bool quitting =
            config.quitAfterSeconds > 0.0 && Seconds(now - start) >= config.quitAfterSeconds;
#ifdef PIXIE_DEVELOPMENT
        if (config.quitAfterSteps) quitting = stepper.Steps() >= *config.quitAfterSteps;
#endif
        const FramebufferSize size = surface.GetFramebufferSize();
        if (size.width > 0 && size.height > 0) {
            SetViewport(size);
            const double drawnDelta = std::min(frameDelta, FixedStepper::kMaxFrameDelta);
            {
                const TelemetryScope measure(telemetry,renderMetric);
                auto renderTime = stepper.RenderTime(drawnDelta, frameIndex);
#ifdef PIXIE_DEVELOPMENT
                if (config.quitAfterSteps) renderTime.alpha = 1.0;
#endif
                app.Render(renderTime, input.game);
            }
            {
                const TelemetryScope observer(telemetry,observerMetric);
                auto* metrics = app.GetTelemetry(); // Reacquire after callbacks: sessions can rebuild their World.
                if (input.debug.ShowsRates() && now >= nextMetrics) {
                    hostMetrics = telemetry.Snapshot();
                    appMetrics = metrics ? metrics->Snapshot() : std::vector<MetricSnapshot>{};
                    nextMetrics = now+std::chrono::milliseconds(500);
                }
                overlay.Draw(input.debug,size,meter.FramesPerSecond(),meter.UpdatesPerSecond(),Seconds(now-start),
                             hostMetrics,appMetrics,telemetry.Enabled(),metrics && metrics->Enabled());
            }
            if (quitting && !config.screenshotPath.empty()) {
                SaveFramebufferPng(config.screenshotPath, size);
            }
            window.Present();
            ++frameIndex;
            meter.CountFrame();
        } else {
            // Minimized: there is nothing to draw, so do not spin.
            window.WaitEvents(0.05);
        }

        window.WaitForFrame();

        if (meter.Sample(Clock::now())) {
            const auto rates = std::format("{:.1f} fps | {:.1f} ups", meter.FramesPerSecond(),
                                           meter.UpdatesPerSecond());
            LogInfo(rates);
            window.SetTitle(std::format("{} | {}", config.title, rates));
        }
        if (quitting) {
            window.Close();
        }
    }

    overlay.Destroy();
#ifdef PIXIE_DEVELOPMENT
    if (config.quitAfterSteps) LogInfo(std::format("Completed {} fixed steps; capture alpha: 1", stepper.Steps()));
#endif
    app.Stop();
    return 0;
}

} // namespace pixie
