#pragma once

#include "pixie/GlBuffer.hpp"
#include "pixie/GlVertexArray.hpp"
#include "pixie/ShaderProgram.hpp"
#include "pixie/Window.hpp"
#include "pixie/Telemetry.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace pixie {
class DebugConsole;

// Last screen layer of the native player. Owns its GL objects, not a game's World or UI toolkit.
// Destroy while the player's context is current, just like the other engine renderers.
class DebugOverlay {
public:
    DebugOverlay() = default;
    ~DebugOverlay();
    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    bool Create();
    void Destroy();
    void Draw(const DebugConsole& console, FramebufferSize size, double fps, double ups, double seconds,
              std::span<const MetricSnapshot> host = {}, std::span<const MetricSnapshot> app = {},
              bool hostEnabled = false, bool appEnabled = false);

private:
    // stb_easy_font emits four vertices of this layout per glyph quad.
    struct Vertex {
        float x, y, z;
        std::array<unsigned char, 4> color;
    };
    using Color = std::array<unsigned char, 4>;
    void Rect(float x, float y, float width, float height, Color color);
    void Text(std::string_view text, float x, float y, Color color, float scale = 2.0f);

    ShaderProgram program_;
    GlBuffer buffer_;
    GlVertexArray array_;
    std::vector<Vertex> vertices_;
    std::array<Vertex, 8192> glyphs_{};
};
} // namespace pixie
