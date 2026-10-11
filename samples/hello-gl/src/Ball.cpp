#include "Ball.hpp"

#include <cmath>

namespace {

// Reflects one axis: clamps the position inside [radius, limit - radius] and points the
// velocity back inside.
void Reflect(float& position, float& velocity, float radius, float limit) {
    if (position - radius < 0.0f) {
        position = radius;
        velocity = std::fabs(velocity);
    } else if (position + radius > limit) {
        position = limit - radius;
        velocity = -std::fabs(velocity);
    }
}

} // namespace

void Step(Ball& ball, float seconds, Vec2 bounds) {
    ball.previousPosition = ball.position;
    ball.position = ball.position + ball.velocity * seconds;
    Reflect(ball.position.x, ball.velocity.x, ball.radius, bounds.x);
    Reflect(ball.position.y, ball.velocity.y, ball.radius, bounds.y);
}
