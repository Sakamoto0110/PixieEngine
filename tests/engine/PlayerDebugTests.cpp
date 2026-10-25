#include "Check.hpp"
#include "pixie/Run.hpp"
#include "pixie/Gl.hpp"
#include "pixie/Telemetry.hpp"

#include <GLFW/glfw3.h>
#include <array>
#include <chrono>

namespace {
using Clock = std::chrono::steady_clock;

void Key(GLFWwindow* window, int key, int action, int modifiers = 0) {
    const auto callback = glfwSetKeyCallback(window, nullptr);
    glfwSetKeyCallback(window, callback);
    CHECK(callback != nullptr);
    if (callback) callback(window, key, 0, action, modifiers);
}

void Character(GLFWwindow* window, unsigned int character) {
    const auto callback = glfwSetCharCallback(window, nullptr);
    glfwSetCharCallback(window, callback);
    CHECK(callback != nullptr);
    if (callback) callback(window, character);
}

void Chord(GLFWwindow* window) {
    Key(window, GLFW_KEY_F3, GLFW_PRESS);
    Key(window, GLFW_KEY_C, GLFW_PRESS);
    Character(window, 'c');
    Key(window, GLFW_KEY_C, GLFW_RELEASE);
    Key(window, GLFW_KEY_F3, GLFW_RELEASE);
}

void Command(GLFWwindow* window, const char* command) {
    for (const unsigned char* character = reinterpret_cast<const unsigned char*>(command); *character; ++character) {
        Character(window, *character);
    }
    Key(window, GLFW_KEY_ENTER, GLFW_PRESS);
    Key(window, GLFW_KEY_ENTER, GLFW_RELEASE);
}

class DebugPlayer : public pixie::App {
public:
    bool Start(pixie::Window&) override {
        start_ = Clock::now();
        phaseStart_ = start_;
        return true;
    }
    void Update(const pixie::FrameTime&, const pixie::Input&) override { ++updates_; }
    void Render(const pixie::FrameTime&, const pixie::Input& input) override {
        auto* window = glfwGetCurrentContext();
        glClearColor(0.05f, 0.12f, 0.18f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        if (!injected_) {
            // Exercise the callbacks installed by Run, including cancelling an already held key.
            Key(window, GLFW_KEY_A, GLFW_PRESS);
            CHECK(input.IsDown(pixie::Key::A));
            Chord(window);
            CHECK(!input.IsDown(pixie::Key::A) && input.WasReleased(pixie::Key::A));
            Key(window, GLFW_KEY_B, GLFW_PRESS);
            const auto mouse = glfwSetMouseButtonCallback(window, nullptr);
            glfwSetMouseButtonCallback(window, mouse);
            if (mouse) mouse(window, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
            const auto scroll = glfwSetScrollCallback(window, nullptr);
            glfwSetScrollCallback(window, scroll);
            if (scroll) scroll(window, 0, 5);
            const auto cursor = glfwSetCursorPosCallback(window, nullptr);
            glfwSetCursorPosCallback(window, cursor);
            if (cursor) cursor(window, 120, 70);
            CHECK(!input.IsDown(pixie::Key::B) && !input.WasPressed(pixie::Key::C));
            CHECK(!input.IsDown(pixie::MouseButton::Left) && input.Scroll() == pxVec2(0.0));
            CHECK(input.CursorMoved() == pxVec2(0.0));
            Command(window, "-vsync false");
            Command(window, "-fps-cap true 30");
            Key(window, GLFW_KEY_ESCAPE, GLFW_PRESS);
            CHECK(!input.WasPressed(pixie::Key::Escape));
            Key(window, GLFW_KEY_B, GLFW_REPEAT);
            CHECK(!input.IsDown(pixie::Key::B));
            Key(window, GLFW_KEY_B, GLFW_RELEASE);
            Key(window, GLFW_KEY_B, GLFW_PRESS);
            CHECK(input.WasPressed(pixie::Key::B));
            Key(window, GLFW_KEY_B, GLFW_RELEASE);
            Key(window, GLFW_KEY_F3, GLFW_PRESS);
            Key(window, GLFW_KEY_F3, GLFW_RELEASE);
            injected_ = true;
            phaseStart_ = Clock::now();
            return;
        }
        ++frames_;
        const double elapsed = std::chrono::duration<double>(Clock::now() - phaseStart_).count();
        if (elapsed < 0.8) return;
        const double fps = frames_ / elapsed;
        std::fprintf(stderr, "[rates] phase=%d mode=%s fps=%.1f updates=%d\n", phase_, names_[phase_], fps, updates_);
        if (phase_ == 0) CHECK(fps <= 40);
        if (phase_ == 1) CHECK(fps <= 75);
        if (phase_ == 4) CHECK(fps <= 21);
        ++phase_;
        frames_ = 0;
        phaseStart_ = Clock::now();
        if (phase_ == 5) { glfwSetWindowShouldClose(window, GLFW_TRUE); return; }
        Chord(window);
        Command(window, commands_[phase_]);
        Key(window, GLFW_KEY_ESCAPE, GLFW_PRESS);
    }
    void Stop() override { stopped_ = true; }
    bool stopped_ = false;
    int phase_ = 0;
    int updates_ = 0;
private:
    static constexpr std::array<const char*, 5> commands_ = {"", "-fps-cap true 60", "-fps-cap false",
        "-vsync true", "-fps-cap true 15"};
    static constexpr std::array<const char*, 5> names_ = {"vsync-off cap-30", "vsync-off cap-60",
        "vsync-off uncapped", "vsync-on uncapped", "vsync-on cap-15"};
    Clock::time_point start_, phaseStart_;
    bool injected_ = false;
    int frames_ = 0;
};
}

TEST(NativePlayerRoutesDebugInputAndAppliesCommandsDuringTheRun) {
    // A real native window and swap exercise VSync and live cap changes. Input is injected through
    // the installed GLFW callbacks; no desktop automation or test-only product hooks are used.
    DebugPlayer player;
    pixie::RunConfig config;
    config.title = "Pixie player debug verification";
    config.width = 640;
    config.height = 240;
    config.vsync = false;
    config.frameRateCap = 30;
    config.quitAfterSeconds = 8;
    CHECK(pixie::Run(player, config) == 0);
    CHECK(player.stopped_ && player.phase_ == 5 && player.updates_ > 30);
}

namespace {
class MetricsPlayer final : public pixie::App {
public:
    MetricsPlayer() : counter(metrics.RegisterCounter("native.updates")) {}
    pixie::Telemetry* GetTelemetry() override { return &metrics; }
    void Update(const pixie::FrameTime&,const pixie::Input&) override { metrics.Increment(counter); }
    void Render(const pixie::FrameTime&,const pixie::Input&) override {
        glClearColor(0.03f,0.06f,0.09f,1);glClear(GL_COLOR_BUFFER_BIT);
        auto* window=glfwGetCurrentContext();
        if(phase==0) {
            CHECK(!metrics.Enabled());
            Chord(window);Command(window,"-telemetry true");Key(window,GLFW_KEY_ESCAPE,GLFW_PRESS);
            Key(window,GLFW_KEY_F3,GLFW_PRESS);Key(window,GLFW_KEY_F3,GLFW_RELEASE);
            phase=1;return;
        }
        if(++frames<6) return;
        frames=0;
        if(phase==1) {
            CHECK(metrics.Enabled() && metrics.Snapshot()[0].counter>0);
            held=metrics.Snapshot()[0].counter;
            Chord(window);Command(window,"-telemetry false");Key(window,GLFW_KEY_ESCAPE,GLFW_PRESS);
            phase=2;
        } else if(phase==2) {
            CHECK(!metrics.Enabled() && metrics.Snapshot()[0].counter==held);
            Chord(window);Command(window,"-telemetry true");Key(window,GLFW_KEY_ESCAPE,GLFW_PRESS);
            phase=3;
        } else {
            CHECK(metrics.Enabled() && metrics.Snapshot()[0].counter>held);
            phase=4;glfwSetWindowShouldClose(window,GLFW_TRUE);
        }
    }
    pixie::Telemetry metrics;
    pixie::MetricHandle counter;
    std::uint64_t held=0;
    int frames=0,phase=0;
};
}

TEST(NativePlayerTelemetryCommandsControlAppCollectionWithoutHidingDisplay) {
    MetricsPlayer app;
    pixie::RunConfig config;config.title="Native telemetry verification";config.width=900;config.height=540;
    config.vsync=false;config.frameRateCap=60;config.quitAfterSeconds=3;
    CHECK(pixie::Run(app,config)==0);
    CHECK(app.phase==4);
}
