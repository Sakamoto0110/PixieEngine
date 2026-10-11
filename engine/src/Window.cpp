#include "pixie/Window.hpp"

#include <GLFW/glfw3.h>

namespace pixie {

Window::Window(GLFWwindow* handle) : handle_(handle) {}

FramebufferSize Window::GetFramebufferSize() const {
    FramebufferSize size;
    glfwGetFramebufferSize(handle_, &size.width, &size.height);
    return size;
}

void Window::SetTitle(const std::string& title) {
    glfwSetWindowTitle(handle_, title.c_str());
}

} // namespace pixie
