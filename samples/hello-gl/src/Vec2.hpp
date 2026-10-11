#pragma once

// The sample's own 2D vector; the engine's primitives come from PixieLib later.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(Vec2 v, float k) { return {v.x * k, v.y * k}; }

inline Vec2 Lerp(Vec2 from, Vec2 to, float t) { return from + (to - from) * t; }
