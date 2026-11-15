#include "pixie/runtime/TriggerDispatcher.hpp"

#include "pixie/runtime/InputTriggers.hpp"

#include <pixie/events/pxEventHandler.hpp>
#include <pixie/events/pxEventInbox.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <format>
#include <map>
#include <stdexcept>

namespace pixie {

namespace {

constexpr auto legacyBit = std::uint64_t{1} << 63;
constexpr std::string_view actionPrefix = "action:";

TriggerActionId LegacyId(std::string_view name) {
    if (name.empty()) return {};
    std::uint64_t hash = 14695981039346656037ull;
    for (const unsigned char byte : name) hash = (hash ^ byte) * 1099511628211ull;
    return {hash | legacyBit};
}

struct FilterValues {
    TriggerType type;
    int code = -1;
    TriggerEdge edge = TriggerEdge::Any;
    InputModifiers modifiers = InputModifiers::None;
};

constexpr TriggerEdge Edge(InputAction action) {
    switch (action) {
    case InputAction::Release: return TriggerEdge::Release;
    case InputAction::Press: return TriggerEdge::Press;
    case InputAction::Repeat: return TriggerEdge::Repeat;
    }
    return TriggerEdge::Any;
}

FilterValues FilterOf(const KeyEvent& event) {
    return {TriggerType::Key, static_cast<int>(event.key), Edge(event.action), event.modifiers};
}
FilterValues FilterOf(const MouseButtonEvent& event) {
    return {TriggerType::MouseButton, static_cast<int>(event.button), Edge(event.action), event.modifiers};
}
FilterValues FilterOf(const MouseMoveEvent& event) {
    return {TriggerType::MouseMove, -1, TriggerEdge::Any, event.modifiers};
}
FilterValues FilterOf(const MouseScrollEvent& event) {
    return {TriggerType::Scroll, -1, TriggerEdge::Any, event.modifiers};
}
FilterValues FilterOf(const TextInputEvent& event) {
    return {TriggerType::Text, static_cast<int>(event.codepoint)};
}
FilterValues FilterOf(const InputFocusEvent& event) {
    return {TriggerType::Focus, -1, event.focused ? TriggerEdge::Press : TriggerEdge::Release};
}
FilterValues FilterOf(const FramebufferResizeEvent&) {
    return {TriggerType::FramebufferResize};
}
FilterValues FilterOf(const JoystickConnectionEvent& event) {
    return {TriggerType::JoystickConnection, event.joystick,
        event.connected ? TriggerEdge::Press : TriggerEdge::Release};
}

bool Matches(const TriggerDefinition& binding, FilterValues event) {
    if (event.code >= 0 && binding.code != -1 && event.code != binding.code) return false;
    if (event.edge != TriggerEdge::Any && binding.edge != TriggerEdge::Any && event.edge != binding.edge) return false;
    if (event.type > TriggerType::Scroll) return true;
    const auto required = static_cast<std::uint32_t>(binding.requiredModifiers);
    return (static_cast<std::uint32_t>(event.modifiers) & required) == required;
}

} // namespace

TriggerActionRef::TriggerActionRef(std::string_view saved) {
    if (saved.starts_with(actionPrefix) && saved.size() == actionPrefix.size() + 16) {
        const auto digits = saved.substr(actionPrefix.size());
        auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), id.value, 16);
        if (error == std::errc{} && end == digits.data() + digits.size() && id.value > 0 && id.value < legacyBit) return;
    }
    id = LegacyId(saved);
    legacyName.assign(saved);
}

std::string TriggerActionRef::Encode() const {
    if (!legacyName.empty()) return legacyName;
    if (id.value == 0) return {};
    return std::format("{}{:016x}", actionPrefix, id.value);
}

void ComponentTypes::AddTriggerAction(TriggerActionId id, std::string name, TriggerDispatcher::Function function,
    std::source_location source) {
    if (id.value == 0 || id.value >= legacyBit || name.empty() || !function) {
        throw std::invalid_argument("invalid compiled trigger action");
    }
    if (std::ranges::any_of(triggerActions_, [id](const TriggerAction& action) { return action.id == id; })) {
        throw std::invalid_argument("duplicate compiled trigger action ID");
    }
    triggerActions_.push_back({id, std::move(name), function, source});
}

struct TriggerDispatcher::State {
    struct Action {
        std::string legacyName;
        pxEventHandler<Entity, const InputEvent&> handlers;
    };
    using Actions = std::map<TriggerActionId, Action>;
    struct Subscription {
        ActionSource source;
        pxSubscription handle;
    };

    Actions actions;
    std::map<SubscriptionId, Subscription> subscriptions;
    SubscriptionId nextId = 1;
    pxEventInbox<InputEvent> inbox;
    bool closing = false;

    Action* Find(const TriggerActionRef& reference) {
        const auto action = actions.find(reference.id);
        if (action == actions.end()) return nullptr;
        if (reference.id.value >= legacyBit && action->second.legacyName != reference.legacyName) return nullptr;
        return &action->second;
    }
};

TriggerDispatcher::TriggerDispatcher() : state_(std::make_unique<State>()) {}
TriggerDispatcher::~TriggerDispatcher() {
    // Retire handlers while the subscription map is still usable by callable destructors.
    state_->closing = true;
    State::Actions retired;
    retired.swap(state_->actions);
}

TriggerDispatcher::SubscriptionId TriggerDispatcher::Subscribe(std::string_view action, Callback callback,
    std::source_location source) {
    TriggerActionRef reference(LegacyId(action));
    reference.legacyName.assign(action);
    return Add(std::move(reference), action, std::move(callback), source);
}

TriggerDispatcher::SubscriptionId TriggerDispatcher::Subscribe(TriggerActionId id, std::string_view name,
    Callback callback, std::source_location source) {
    if (id.value >= legacyBit) return 0;
    return Add(TriggerActionRef(id), name, std::move(callback), source);
}

TriggerDispatcher::SubscriptionId TriggerDispatcher::Add(TriggerActionRef reference, std::string_view name,
    Callback callback, std::source_location source) {
    if (state_->closing || reference.id.value == 0 || name.empty() || !callback) return 0;
    auto [event, inserted] = state_->actions.try_emplace(reference.id);
    if (inserted) event->second.legacyName = reference.legacyName;
    if (event->second.legacyName != reference.legacyName) throw std::invalid_argument("legacy trigger action ID collision");
    ActionSource code{std::move(reference), std::string(name), source.file_name(), source.line()};
    auto subscription = event->second.handlers.Subscribe(std::move(callback));
    const auto id = state_->nextId++;
    state_->subscriptions.emplace(id, State::Subscription{std::move(code), std::move(subscription)});
    return id;
}

bool TriggerDispatcher::Unsubscribe(SubscriptionId id) {
    // Detach the node before destroying the callable so reentrant removal sees a consistent map.
    auto removed = state_->subscriptions.extract(id);
    return !removed.empty();
}

void TriggerDispatcher::Post(InputEvent event) {
    state_->inbox.Post(std::move(event));
}

bool TriggerDispatcher::TryPost(InputEvent event, std::size_t capacity) {
    return state_->inbox.TryPost(std::move(event), capacity);
}

bool TriggerDispatcher::HasAction(const TriggerActionRef& reference) const {
    const auto* action = state_->Find(reference);
    return action && !action->handlers.IsEmpty();
}

std::vector<TriggerDispatcher::ActionSource> TriggerDispatcher::ActionSources() const {
    std::vector<ActionSource> sources;
    sources.reserve(state_->subscriptions.size());
    for (const auto& entry : state_->subscriptions) sources.push_back(entry.second.source);
    return sources;
}

void TriggerDispatcher::Dispatch(World& world, std::span<const InputEvent> events) {
    const auto posted = state_->inbox.Take();
    if ((events.empty() && posted.empty()) || state_->subscriptions.empty()) return;

    struct Binding {
        Entity source;
        TriggerDefinition definition;
    };
    constexpr auto count = static_cast<std::size_t>(TriggerType::JoystickConnection) + 1;
    std::array<std::vector<Binding>, count> bindings;
    // Snapshot authored values once. Actions may edit the World while this batch is dispatched.
    world.Each<const InputTriggers>([&](Entity entity, const InputTriggers& triggers) {
        for (const auto& record : triggers.bindings) {
            const auto& definition = record.value;
            const auto index = static_cast<std::size_t>(definition.type);
            if (definition.enabled && definition.action.id.value != 0 && index < bindings.size()) {
                bindings[index].push_back({entity, definition});
            }
        }
    });

    const auto dispatch = [&](std::span<const InputEvent> batch) {
        for (const auto& event : batch) {
            const auto filter = std::visit([](const auto& value) { return FilterOf(value); }, event);
            for (const auto& binding : bindings[static_cast<std::size_t>(filter.type)]) {
                if (!Matches(binding.definition, filter) || !binding.source.IsAlive()) continue;
                if (auto* action = state_->Find(binding.definition.action)) action->handlers.Invoke(binding.source, event);
            }
        }
    };
    dispatch(events);
    dispatch(posted);
}

} // namespace pixie
