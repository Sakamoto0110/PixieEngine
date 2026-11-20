#pragma once

#include <GLFW/glfw3.h>

namespace pixie::vm_gl43 {

inline void WindowHint(int hint, int value) {
    if (hint == GLFW_CONTEXT_VERSION_MINOR) {
        value = 3;
    }
    if (hint == GLFW_OPENGL_FORWARD_COMPAT) {
        value = GLFW_FALSE;
    }
    glfwWindowHint(hint, value);
}

} // namespace pixie::vm_gl43

#define glfwWindowHint ::pixie::vm_gl43::WindowHint
