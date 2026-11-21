#pragma once

#include "pixie/runtime/ComponentTypes.hpp"

#include <vector>

namespace pixie {

// Persisted via component reflection. Keep the enumerators and Inspector choices in the same order.
enum class TriggerType { Key, MouseButton, MouseMove, Scroll, Text, Focus, FramebufferResize, JoystickConnection };
enum class TriggerEdge { Any, Release, Press, Repeat };

/// Authored binding. The action ID resolves against the callbacks registered on the current World.
struct TriggerDefinition {
    TriggerType type = TriggerType::Key;
    bool enabled = true;
    int code = -1;
    TriggerEdge edge = TriggerEdge::Press;
    int requiredModifiers = 0;
    TriggerActionRef action;
};

struct InputTriggers {
    std::vector<ComponentRecord<TriggerDefinition>> bindings;
};

} // namespace pixie
