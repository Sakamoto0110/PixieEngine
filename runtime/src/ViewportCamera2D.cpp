#include "pixie/runtime/ViewportCamera2D.hpp"
#include "pixie/runtime/Scene.hpp"
#include "pixie/runtime/Transform2D.hpp"
#include <cmath>

namespace pixie {
ViewportCamera2D ResolveViewportCamera2D(const World& world, const Camera2D& fallback, CameraSelection2D selection) {
    ViewportCamera2D result; result.camera = fallback;
    if (selection.kind == CameraSelectionKind2D::Free) return result;
    const auto camera = [&](std::uint64_t id, CameraResolutionKind2D kind) {
        if (!id) return false;
        for (const auto [entity, authored, sceneId, transform] :
             world.Registry().view<const CameraComponent2D, const SceneId, const Transform2D>().each()) {
            (void)entity;
            if (sceneId.value != id) continue;
            if (!authored.enabled || !std::isfinite(authored.zoom) || authored.zoom < Camera2D::kMinZoom || authored.zoom > Camera2D::kMaxZoom ||
                !std::isfinite(transform.position.x) || !std::isfinite(transform.position.y)) break;
            result.camera.SetCenter(transform.position); result.camera.SetZoom(authored.zoom);
            result.kind = kind; result.entity = id; return true;
        }
        result.diagnostic = "selected camera is missing, disabled or invalid; using the next fallback";
        return false;
    };
    if (selection.kind == CameraSelectionKind2D::Entity && camera(selection.entity, CameraResolutionKind2D::Entity)) return result;
    if (selection.scene.value) {
        bool found = false;
        for (const auto& scope : world.Scopes()) {
            if (scope.id != selection.scene) continue;
            found = true;
            if (camera(scope.camera, CameraResolutionKind2D::SceneDefault)) return result;
        }
        if (!found) result.diagnostic = "camera Scene was removed; using World or free camera";
    }
    if (world.Presentation()) camera(world.Presentation()->camera, CameraResolutionKind2D::WorldDefault);
    return result;
}
} // namespace pixie
