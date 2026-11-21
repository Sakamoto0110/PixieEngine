#pragma once

#include "pixie/runtime/Export.hpp"
#include "pixie/runtime/Lighting2D.hpp"
#include "pixie/runtime/World.hpp"

#include <optional>
#include <string>

namespace pixie {

enum class LightingState2D { Absent, Disabled, Configured };

// The environment part of the current v1 lighting settings. This is a read-only
// value; the LightingSettings2D component remains the sole editable authority.
struct EnvironmentLighting2D {
    pxColorRgba ambient{255, 255, 255};
    double ambientIntensity = 1.0;
    bool fog = false;
    pxColorRgba fogColor{110, 120, 140};
    double fogDensity = 0.4;
    double fogTop = 0.0;
    double fogBottom = 400.0;
    bool operator==(const EnvironmentLighting2D&) const = default;
};

struct LightingEnvironment2DView {
    LightingState2D state = LightingState2D::Absent;
    // Present even when disabled, so re-enabling retains the authored values.
    std::optional<EnvironmentLighting2D> environment;
};

// Does not install lighting. No settings means an unlit legacy scene; a present
// default-valued component is configured. With duplicate v1 components, reads
// use the same first component as legacy rendering.
PIXIE_RUNTIME_API LightingEnvironment2DView ReadEnvironmentLighting2D(const World& world);

enum class EnvironmentSelectionKind2D { LegacyWorld, Scene };

// A viewport's presentation choice, independent of editor entity selection.
// The Scene ID is retained when its definition is removed so undo can restore it.
struct EnvironmentSelection2D {
    EnvironmentSelectionKind2D kind = EnvironmentSelectionKind2D::LegacyWorld;
    ScopeId scene;
    friend bool operator==(const EnvironmentSelection2D&, const EnvironmentSelection2D&) = default;
};

enum class EnvironmentResolutionKind2D { LegacyWorld, SceneInheritedWorld, RemovedSceneFallback };

struct ViewportEnvironment2D {
    LightingEnvironment2DView lighting;
    EnvironmentResolutionKind2D kind = EnvironmentResolutionKind2D::LegacyWorld;
    std::string diagnostic;
};

// A selected Scene currently inherits the v1 World lighting; authored Scene
// Environment overrides are not part of this slice. A missing catalog definition
// is definitive removal and falls back to World without changing the selection.
// Future temporary unload must retain the definition and use a separate state.
PIXIE_RUNTIME_API ViewportEnvironment2D ResolveViewportEnvironment2D(
    const World& world, EnvironmentSelection2D selection);

// Transitional edit access to that same v1 component. This does not create a
// second authority or install a missing component. A duplicate configuration
// rejects the edit, while reads retain the legacy first-component behavior.
PIXIE_RUNTIME_API LightingSettings2D* EditLegacyLightingSettings2D(World& world);

} // namespace pixie
