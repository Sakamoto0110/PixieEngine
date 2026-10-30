#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Gate.hpp"
#include "pixie_capi.h"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc != 2) return 1;
    const HMODULE module = LoadLibraryA(argv[1]);
    if (!module) return 2;
    const auto check = reinterpret_cast<bool (*)()>(GetProcAddress(module, "module_check"));
    if (!check || !check()) return 3;
    if (!glfwInit()) return 4;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "boundary-gate", nullptr, nullptr);
    if (!window) return 5;
    glfwMakeContextCurrent(window);
    PixieSession* session = nullptr;
    if (pixie_session_create(64, 64, 42, &session) != PIXIE_OK) {
        char error[2048]{};
        pixie_last_error(error, sizeof(error));
        std::fprintf(stderr, "%s\n", error);
        return 6;
    }
    int32_t type = -1, count = 0;
    uint32_t entities[8]{};
    PixieValue value{};
    const bool passed = pixie_type_find(session, "ModulePrivate", &type) == PIXIE_OK &&
        pixie_entities(session, entities, 8, &count) == PIXIE_OK && count == 1 &&
        pixie_field_get(session, entities[0], type, 0, &value) == PIXIE_OK && value.x == 123;
    pixie_session_destroy(session);
    boundary_factory(nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
    FreeLibrary(module);
    std::puts(passed ? "PASS: typed World access and module-private C API reflection" : "FAIL: C API reflection");
    return passed ? 0 : 7;
}
