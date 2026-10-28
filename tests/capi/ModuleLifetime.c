/* Checks that changing the default module cannot unload a live session's code. */
#include "pixie_capi.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdio.h>

int main(int argc, char** argv) {
    PixieSession* session = NULL;
    GLFWwindow* window;
    int32_t type = -1;
    int passed;
    if (argc != 4 || !glfwInit()) return 2;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(64, 64, "module-lifetime", NULL, NULL);
    if (!window) { glfwTerminate(); return 3; }
    glfwMakeContextCurrent(window);
    passed = pixie_module_load(argv[1]) == PIXIE_OK &&
        pixie_session_create(64, 64, 1, &session) == PIXIE_OK;
    if (passed) {
        /* An incompatible image must not replace the selected factory. */
        passed = pixie_module_load(argv[2]) == PIXIE_ERROR_ARGUMENT &&
            pixie_type_find(session, "BoidControls", &type) == PIXIE_OK &&
            pixie_module_load(NULL) == PIXIE_OK &&
            pixie_session_step(session) == PIXIE_OK &&
            pixie_type_find(session, "BoidControls", &type) == PIXIE_OK &&
            pixie_session_reset(session) == PIXIE_OK &&
            pixie_type_find(session, "BoidControls", &type) == PIXIE_OK;
    }
    pixie_session_destroy(session);
    session = NULL;
    if (passed) {
        passed = pixie_module_load(argv[3]) == PIXIE_OK &&
            pixie_session_create(64, 64, 1, &session) == PIXIE_ERROR_INTERNAL && session == NULL;
    }
    pixie_session_destroy(session);
    pixie_module_load(NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    puts(passed ? "PASS: live module lease and incompatible factory rejection" : "FAIL: module lifetime");
    return passed ? 0 : 1;
}
