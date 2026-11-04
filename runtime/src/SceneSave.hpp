#pragma once

#include "pixie/runtime/Scene.hpp"

namespace pixie::detail {

// Internal destination-version policy for text already produced by admitted scene serialization.
// Public callers use SaveSceneFile(World, ComponentTypes, path), which preserves v2 admission.
SceneReport SaveSceneText(std::string_view text, const std::filesystem::path& path);

} // namespace pixie::detail
