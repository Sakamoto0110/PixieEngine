#pragma once

#include <cstdint>

namespace pixie {

// Which of a lit frame's targets the 2D renderers draw into (LightRenderer): the albedo, as unlit
// drawing does; the receiver target, where an instance's color holds its receiver (light layers in
// red, unlit in green, its own alpha in blue); or the emission target, where it holds its emissive
// tint in RGB and intensity / 16 in alpha, and the output is linear radiance.
enum class LightingPass : std::uint8_t { Color, Receivers, Emission };

} // namespace pixie
