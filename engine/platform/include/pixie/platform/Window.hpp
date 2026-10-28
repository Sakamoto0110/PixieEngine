#pragma once

#include "pixie/platform/Surface.hpp"
#include <memory>
#include <string>

struct GLFWwindow;

namespace pixie {
class FrameLimiter;

// The player window owns its GLFW session; an explicitly supplied handle is borrowed.
class Window final : public Surface {
public:
    Window();
    explicit Window(GLFWwindow* handle);
    ~Window() override;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Create(int width, int height, const std::string& title);
    void MakeCurrent() const override;
    FramebufferSize GetFramebufferSize() const override;
    void SetTitle(const std::string& title);
    void Present() const;
    void SetVsync(bool enabled) const;
    bool ShouldClose() const;
    void Close() const;
    void PollEvents() const;
    void WaitEvents(double seconds) const;
    void SetFrameRateCap(double rate);
    void WaitForFrame();
    // App connects the player's debug/game input routing to the platform event source.
    GLFWwindow* Handle() const { return handle_; }

private:
    GLFWwindow* handle_ = nullptr;
    bool ownsSession_ = false;
    std::unique_ptr<FrameLimiter> limiter_;
};

} // namespace pixie
