#pragma once

#include "Color.hpp"
#include "Vec2.hpp"

#include "pixie/Gl.hpp"
#include "pixie/Window.hpp"

#include <span>

// One circle as the GPU sees it.
struct BallInstance {
    Vec2 center;
    float radius = 0.0f;
    Color color;
};

// Draws antialiased circles as instanced quads.
class BallRenderer {
public:
    bool Create();
    void Destroy();
    void Draw(std::span<const BallInstance> balls, pixie::FramebufferSize viewport);

private:
    GLuint program_ = 0;
    GLuint vertexArray_ = 0;
    GLuint instanceBuffer_ = 0;
};
