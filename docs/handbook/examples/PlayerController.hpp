#pragma once

#include "pixie/runtime/Behaviour.hpp"

namespace pixie { class ComponentTypes; }

// Free top-down movement; collision-aware characters use PlatformerController.
struct PlayerController final : pixie::Behaviour {
    bool enabled = true;
    double speed = 180.0;

    static void Describe(pixie::ComponentTypes& types);
    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;
};
