#include "pixie/platform/Window.hpp"

#include "FrameLimiter.hpp"
#include "pixie/core/Log.hpp"
#include <GLFW/glfw3.h>
#include <format>

namespace pixie {

Window::Window() = default;
Window::Window(GLFWwindow* handle) : handle_(handle) {}

Window::~Window() {
    if (ownsSession_) {
        if (handle_) glfwDestroyWindow(handle_);
        glfwTerminate();
    }
}

bool Window::Create(int width, int height, const std::string& title) {
    if (handle_ || ownsSession_) return false;
    glfwSetErrorCallback([](int code, const char* description) {
        LogError(std::format("GLFW {}: {}", code, description));
    });
    if (glfwInit() != GLFW_TRUE) {
        LogError("GLFW initialization failed");
        return false;
    }
    ownsSession_ = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    handle_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!handle_) LogError("Could not create a window with an OpenGL 4.5 core context");
    return handle_ != nullptr;
}

void Window::MakeCurrent() const { glfwMakeContextCurrent(handle_); }
void Window::Present() const { glfwSwapBuffers(handle_); }
void Window::SetVsync(bool enabled) const { glfwSwapInterval(enabled ? 1 : 0); }
bool Window::ShouldClose() const { return glfwWindowShouldClose(handle_) != GLFW_FALSE; }
void Window::Close() const { glfwSetWindowShouldClose(handle_, GLFW_TRUE); }
void Window::PollEvents() const { glfwPollEvents(); }
void Window::WaitEvents(double seconds) const { glfwWaitEventsTimeout(seconds); }
void Window::SetFrameRateCap(double rate) {
    if (limiter_) limiter_->SetRate(rate);
    else limiter_ = std::make_unique<FrameLimiter>(rate);
}
void Window::WaitForFrame() { if (limiter_) limiter_->Wait(); }


FramebufferSize Window::GetFramebufferSize() const {
    FramebufferSize size;
    glfwGetFramebufferSize(handle_, &size.width, &size.height);
    return size;
}

void Window::SetTitle(const std::string& title) {
    glfwSetWindowTitle(handle_, title.c_str());
}

} // namespace pixie
