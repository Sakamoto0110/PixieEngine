#pragma once

#include "pixie/runtime/Export.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/World.hpp"

#include <optional>
#include <string>

namespace pixie {

struct LightingEnvironment2DView {
    LightingState2D state = LightingState2D::Absent;
    // Present even when disabled, so re-enabling retains the authored values.
    std::optional<EnvironmentLighting2D> environment;
};

// Does not install lighting. Canonical presentation owns migrated settings; otherwise
// the v1 adapter preserves absence, disabled state and the legacy duplicate-read choice.
PIXIE_RUNTIME_API LightingEnvironment2DView ReadEnvironmentLighting2D(const World& world);

enum class EnvironmentSelectionKind2D { LegacyWorld, Scene };

// A viewport's presentation choice, independent of editor entity selection.
// The Scene ID is retained when its definition is removed so undo can restore it.
struct EnvironmentSelection2D {
    EnvironmentSelectionKind2D kind = EnvironmentSelectionKind2D::LegacyWorld;
    ScopeId scene;
    friend bool operator==(const EnvironmentSelection2D&, const EnvironmentSelection2D&) = default;
};

enum class EnvironmentResolutionKind2D { LegacyWorld, SceneInheritedWorld, RemovedSceneFallback, SceneOverride };

struct ViewportEnvironment2D {
    LightingEnvironment2DView lighting;
    EnvironmentResolutionKind2D kind = EnvironmentResolutionKind2D::LegacyWorld;
    std::string diagnostic;
};

// A selected Scene explicitly inherits or overrides Environment values. It cannot
// enable an absent or disabled lighting capability. A missing catalog definition
// is definitive removal and falls back to World without changing the selection.
// Future temporary unload must retain the definition and use a separate state.
PIXIE_RUNTIME_API ViewportEnvironment2D ResolveViewportEnvironment2D(
    const World& world, EnvironmentSelection2D selection);

// Transitional edit access to that same v1 component. This does not create a
// second authority or install a missing component. A duplicate configuration
// rejects the edit, while reads retain the legacy first-component behavior.
// Canonical presentation also refuses this pointer: it is never a second authority.
PIXIE_RUNTIME_API LightingSettings2D* EditLegacyLightingSettings2D(World& world);

} // namespace pixie
