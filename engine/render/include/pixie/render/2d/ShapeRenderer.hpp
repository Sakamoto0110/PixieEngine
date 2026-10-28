#pragma once

#include "pixie/render/View.hpp"
#include "pixie/render/gl/GlBuffer.hpp"
#include "pixie/render/gl/GlVertexArray.hpp"
#include "pixie/render/gl/ShaderProgram.hpp"
#include "pixie/render/2d/ShapeInstance.hpp"

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

    // Draws every instance in `instances` as `kind`, seen through `view`.
    void Draw(ShapeKind kind, std::span<const ShapeInstance> instances, const View& view);

    void Submit(ShapeKind kind, const ShapeInstance& instance);
    // Draws what was submitted since the last flush, then forgets it.
    void Flush(const View& view);
    // How many shapes wait for the next flush.
    std::size_t PendingCount() const;

private:
    ShaderProgram program_;
    GlVertexArray vertexArray_;
    GlBuffer instances_;
    std::array<std::vector<ShapeInstance>, kShapeKindCount> pending_;
};

} // namespace pixie
