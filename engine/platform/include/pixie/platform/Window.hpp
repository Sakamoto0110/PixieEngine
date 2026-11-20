#pragma once

#include "pixie/platform/Export.hpp"

#include "pixie/platform/Surface.hpp"
#include <memory>
#include <string>

struct GLFWwindow;

namespace pixie {
class FrameLimiter;

/// A player-created window owns its GLFW session. The handle constructor borrows an existing window.
class Window final : public Surface {
public:
    PIXIE_PLATFORM_API Window();
    PIXIE_PLATFORM_API explicit Window(GLFWwindow* handle);
    PIXIE_PLATFORM_API ~Window() override;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /// Creates an OpenGL 4.5 core window from dimensions in GLFW screen coordinates.
    /// Applies initial visibility before creation. Returns false on startup failure.
    PIXIE_PLATFORM_API bool Create(int width, int height, const std::string& title, bool visible = true);
    PIXIE_PLATFORM_API void MakeCurrent() const override;
    PIXIE_PLATFORM_API FramebufferSize GetFramebufferSize() const override;
    PIXIE_PLATFORM_API void SetTitle(const std::string& title);
    PIXIE_PLATFORM_API void Present() const;
    /// Changes the swap interval for the context current on this thread.
    PIXIE_PLATFORM_API void SetVsync(bool enabled) const;
    PIXIE_PLATFORM_API bool ShouldClose() const;
    PIXIE_PLATFORM_API void Close() const;
    PIXIE_PLATFORM_API void PollEvents() const;
    /// Waits for and dispatches events on the GLFW event thread, up to the supplied seconds.
    PIXIE_PLATFORM_API void WaitEvents(double seconds) const;
    /// Sets a frame cap in frames per second. Zero disables waiting independently of vsync.
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
