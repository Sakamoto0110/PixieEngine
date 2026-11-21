#pragma once
#include "pixie/runtime/World.hpp"
#include "pixie/render/2d/Camera2D.hpp"

namespace pixie {
enum class CameraSelectionKind2D { Free, Defaults, Entity };
struct CameraSelection2D {
    CameraSelectionKind2D kind = CameraSelectionKind2D::Free;
    std::uint64_t entity = 0;
    ScopeId scene;
    bool operator==(const CameraSelection2D&) const = default;
};
enum class CameraResolutionKind2D { Free, Entity, SceneDefault, WorldDefault };
struct ViewportCamera2D {
    Camera2D camera;
    CameraResolutionKind2D kind = CameraResolutionKind2D::Free;
    std::uint64_t entity = 0;
    std::string diagnostic;
};
PIXIE_RUNTIME_API ViewportCamera2D ResolveViewportCamera2D(
    const World& world, const Camera2D& fallback, CameraSelection2D selection);
} // namespace pixie
