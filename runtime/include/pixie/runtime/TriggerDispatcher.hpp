#pragma once

#include "pixie/platform/Input.hpp"
#include "pixie/runtime/Entity.hpp"
#include "pixie/runtime/Export.hpp"

#include <cstdint>
#include <cstddef>
#include <compare>
#include <functional>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

namespace pixie {

class World;

struct TriggerActionId {
    std::uint64_t value = 0;
    constexpr auto operator<=>(const TriggerActionId&) const = default;
};

/// Stores a compiled action's persistent ID or a compatibility reference from an older scene.
struct TriggerActionRef {
    TriggerActionRef() = default;
    TriggerActionRef(TriggerActionId id) : id(id) {}
    TriggerActionRef(const char* legacyName) : TriggerActionRef(std::string_view(legacyName)) {}
    PIXIE_RUNTIME_API explicit TriggerActionRef(std::string_view saved);
    PIXIE_RUNTIME_API std::string Encode() const;
    TriggerActionId id;
    std::string legacyName;
};

/// Resolves authored bindings by persistent action ID on the World thread during a fixed step.
class TriggerDispatcher {
public:
    using SubscriptionId = std::uint64_t;
    using Callback = std::function<void(Entity source, const InputEvent& event)>;
    using Function = void (*)(Entity source, const InputEvent& event);
    struct ActionSource {
        TriggerActionRef reference;
        std::string action;
        std::string file;
        std::uint_least32_t line;
    };

    /// Registers a callback under a persistent ID. The name is display metadata.
    PIXIE_RUNTIME_API SubscriptionId Subscribe(TriggerActionId id, std::string_view name, Callback callback,
        std::source_location source = std::source_location::current());
    /// Compatibility overload for earlier project modules and scenes with named bindings.
    /// The subscription belongs to this World and is released when the World is destroyed.
    /// The default location points to the registration. Pass a location captured at the
    /// callback definition to point the Editor there instead.
    PIXIE_RUNTIME_API SubscriptionId Subscribe(std::string_view action, Callback callback,
        std::source_location source = std::source_location::current());
    PIXIE_RUNTIME_API bool Unsubscribe(SubscriptionId id);

    /// Thread-safe while the owning World stays alive. Posts a trigger-only event for a later step.
    /// Does not mutate the host's Input or invoke callbacks on the posting thread.
    PIXIE_RUNTIME_API void Post(InputEvent event);
    /// Posts only if the total pending batch is below capacity.
    [[nodiscard]] PIXIE_RUNTIME_API bool TryPost(InputEvent event, std::size_t capacity);
    /// Queries a registered action on the World thread.
    PIXIE_RUNTIME_API bool HasAction(const TriggerActionRef& action) const;
    /// Lists active callback registrations from the World thread, in registration order.
    PIXIE_RUNTIME_API std::vector<ActionSource> ActionSources() const;

private:
    friend class World;
    TriggerDispatcher();
    ~TriggerDispatcher();
    SubscriptionId Add(TriggerActionRef reference, std::string_view name, Callback callback, std::source_location source);
    void Dispatch(World& world, std::span<const InputEvent> events);

    struct State;
    std::unique_ptr<State> state_;
};

} // namespace pixie
