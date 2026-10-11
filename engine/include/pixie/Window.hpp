#pragma once

#include <string>

struct GLFWwindow;

namespace pixie {

struct FramebufferSize {
    int width = 0;
    int height = 0;
};

// The window the engine renders into. Created and owned by pixie::Run.
class Window {
public:
    explicit Window(GLFWwindow* handle);

    FramebufferSize GetFramebufferSize() const;
    void SetTitle(const std::string& title);

private:
    GLFWwindow* handle_;
};

} // namespace pixie
