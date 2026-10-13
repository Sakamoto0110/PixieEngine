#pragma once

#include "pixie/Camera2D.hpp"
#include "pixie/GlBuffer.hpp"
#include "pixie/GlVertexArray.hpp"
#include "pixie/ShaderProgram.hpp"
#include "pixie/ShapeInstance.hpp"

#include <array>
#include <span>
#include <vector>

namespace pixie {

// Draws triangles, circles, rings, and quads as instances: one draw call per kind, however many
// shapes. Shapes are either drawn at once from a span, or submitted during a frame and flushed
// together, kind by kind in ShapeKind order.
class ShapeRenderer {
public:
    // Needs a current GL context; returns false if the shaders do not build.
    bool Create();
    void Destroy();

    // Draws every instance in `instances` as `kind`, seen through `camera`.
    void Draw(ShapeKind kind, std::span<const ShapeInstance> instances, const Camera2D& camera);

    void Submit(ShapeKind kind, const ShapeInstance& instance);
    // Draws what was submitted since the last flush, then forgets it.
    void Flush(const Camera2D& camera);

private:
    ShaderProgram program_;
    GlVertexArray vertexArray_;
    GlBuffer instances_;
    std::array<std::vector<ShapeInstance>, kShapeKindCount> pending_;
};

} // namespace pixie
