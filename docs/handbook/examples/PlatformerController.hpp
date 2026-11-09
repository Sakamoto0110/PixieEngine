#pragma once

#include "pixie/runtime/Behaviour.hpp"

namespace pixie { class ComponentTypes; }

// Attach alongside CharacterMotor2D and KeyboardControl.
struct PlatformerController final : pixie::Behaviour {
    bool enabled = true;
    static void Describe(pixie::ComponentTypes& types);
    void Start() override;
    void Update(const pixie::FrameTime& time, const pixie::Input& input) override;
};
