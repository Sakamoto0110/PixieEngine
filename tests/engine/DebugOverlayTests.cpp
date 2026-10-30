#include "Check.hpp"
#include "DebugConsole.hpp"
#include "DebugOverlay.hpp"
#include "pixie/render/gl/GlLoader.hpp"

#include <GLFW/glfw3.h>
#include <array>
#include <vector>

TEST(DebugOverlayDrawsAboveTheGameAndRestoresGlState) {
    CHECK(glfwInit() == GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(640, 240, "debug-overlay-test", nullptr, nullptr);
    CHECK(window != nullptr);
    if (!window) { glfwTerminate(); return; }
    glfwMakeContextCurrent(window);
    if (!pixie::LoadGl()) { CHECK(false); glfwDestroyWindow(window); glfwTerminate(); return; }
    {
        pixie::DebugOverlay overlay;
        CHECK(overlay.Create());
        pixie::DebugConsole console(false, 0);
        console.KeyEvent(pixie::Key::F3, true);
        console.KeyEvent(pixie::Key::F3, false);
        console.KeyEvent(pixie::Key::F3, true);
        console.KeyEvent(pixie::Key::C, true);
        console.KeyEvent(pixie::Key::F3, false);
        console.Paste("-fps-cap true 60");
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glClearColor(0, 0.5f, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        GLuint target = 0, array = 0;
        glCreateFramebuffers(1, &target);
        glCreateVertexArrays(1, &array);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target);
        glBindVertexArray(array);
        glViewport(3, 4, 11, 13);
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, 1, 1);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_RASTERIZER_DISCARD);
        glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ZERO, GL_ONE);
        glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_SUBTRACT);
        std::array<GLint, 2> beforeModes{};
        glGetIntegerv(GL_POLYGON_MODE, beforeModes.data());
        overlay.Draw(console, {width, height}, 61.2, 60, 0.2);

        GLint value = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &value);
        CHECK(static_cast<GLuint>(value) == target);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &value);
        CHECK(static_cast<GLuint>(value) == array);
        std::array<GLint, 4> viewport{};
        glGetIntegerv(GL_VIEWPORT, viewport.data());
        CHECK((viewport == std::array<GLint, 4>{3, 4, 11, 13}));
        std::array<GLboolean, 4> mask{};
        glGetBooleanv(GL_COLOR_WRITEMASK, mask.data());
        CHECK((mask == std::array<GLboolean, 4>{GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE}));
        CHECK(glIsEnabled(GL_SCISSOR_TEST) && glIsEnabled(GL_DEPTH_TEST));
        CHECK(glIsEnabled(GL_CULL_FACE) && glIsEnabled(GL_RASTERIZER_DISCARD));
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &value);
        CHECK(value == GL_FUNC_REVERSE_SUBTRACT);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &value);
        CHECK(value == GL_FUNC_SUBTRACT);
        std::array<GLint, 2> modes{};
        glGetIntegerv(GL_POLYGON_MODE, modes.data());
        std::fprintf(stderr, "polygon mode before=%x/%x after=%x/%x\n", beforeModes[0], beforeModes[1], modes[0], modes[1]);
        CHECK(modes == beforeModes);

        // Read the actual window, not the app's custom draw target. The top contains UI, while the
        // bottom retains the game's green clear. Bright pixels prove that glyphs also rendered.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);
        std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        const auto top = static_cast<std::size_t>((height - 5) * width + 5) * 4;
        CHECK(pixels[top + 1] < 50 && pixels[1] > 100);
        int bright = 0;
        for (int y = height - 90; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const auto index = static_cast<std::size_t>(y * width + x) * 4;
                if (pixels[index] > 170 && pixels[index + 1] > 170) ++bright;
            }
        }
        CHECK(bright > 100);
        CHECK(glGetError() == GL_NO_ERROR);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindVertexArray(0);
        glDeleteFramebuffers(1, &target);
        glDeleteVertexArrays(1, &array);
        overlay.Destroy();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
