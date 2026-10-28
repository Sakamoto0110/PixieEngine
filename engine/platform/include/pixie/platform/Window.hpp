#pragma once

#include "pixie/platform/Export.hpp"

#include "pixie/platform/Surface.hpp"
#include <memory>
#include <string>

struct GLFWwindow;

namespace pixie {
class FrameLimiter;

// The player window owns its GLFW session; an explicitly supplied handle is borrowed.
class Window final : public Surface {
public:
    PIXIE_PLATFORM_API Window();
    PIXIE_PLATFORM_API explicit Window(GLFWwindow* handle);
    PIXIE_PLATFORM_API ~Window() override;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    PIXIE_PLATFORM_API bool Create(int width, int height, const std::string& title);
    PIXIE_PLATFORM_API void MakeCurrent() const override;
    PIXIE_PLATFORM_API FramebufferSize GetFramebufferSize() const override;
    PIXIE_PLATFORM_API void SetTitle(const std::string& title);
    PIXIE_PLATFORM_API void Present() const;
    PIXIE_PLATFORM_API void SetVsync(bool enabled) const;
    PIXIE_PLATFORM_API bool ShouldClose() const;
    PIXIE_PLATFORM_API void Close() const;
    PIXIE_PLATFORM_API void PollEvents() const;
    PIXIE_PLATFORM_API void WaitEvents(double seconds) const;
    PIXIE_PLATFORM_API void SetFrameRateCap(double rate);
    PIXIE_PLATFORM_API void WaitForFrame();
    // App connects the player's debug/game input routing to the platform event source.
    GLFWwindow* Handle() const { return handle_; }

private:
    GLFWwindow* handle_ = nullptr;
    bool ownsSession_ = false;
    std::unique_ptr<FrameLimiter> limiter_;
};

} // namespace pixie
