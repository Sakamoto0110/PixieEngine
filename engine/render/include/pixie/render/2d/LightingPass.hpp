#pragma once

#include <cstdint>

namespace pixie {

// Which of a lit frame's targets the 2D renderers draw into (LightRenderer): the albedo, as unlit
// drawing does; the receiver target, where an instance's color holds its receiver (light layers in
// red, unlit in green, its own alpha in blue); the emission target, where it holds its emissive
// tint in RGB and intensity / 16 in alpha, and the output is linear radiance; or the normal target
// (sprites only), where a sprite's texture is its normal map and its color holds the map's strength
// in red, whether it has one in green and its own alpha in blue.
enum class LightingPass : std::uint8_t { Color, Receivers, Emission, Normals };

} // namespace pixie
