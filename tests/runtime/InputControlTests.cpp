#include "Check.hpp"
#include "pixie/runtime/World.hpp"
#include "pixie/runtime/ComponentTypes.hpp"
#include "pixie/runtime/InputTriggers.hpp"
#include "pixie/runtime/Scene.hpp"
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace pixie;

namespace {
struct InputProbe : Behaviour {
    std::vector<std::string>* log = nullptr;
    bool disableOnKey = false;
    bool destroyOnKey = false;
    bool tryReentry = false;
    bool reentryRejected = false;
    bool queriedDown = false;
    void Start() override { log->push_back("start"); }
    void OnKey(const KeyEvent& event, const Input& input) override {
        log->push_back("key" + std::to_string(static_cast<int>(event.action)));
        if (disableOnKey) Self().Get<KeyboardControl>().enabled = false;
        if (destroyOnKey) Self().Destroy();
        if (tryReentry) {
            try { GetWorld().Step(1.0, input); }
            catch (const std::logic_error&) { reentryRejected = true; }
        }
    }
    void OnMouseButton(const MouseButtonEvent&, const Input&) override { log->push_back("button"); }
    void OnMouseMove(const MouseMoveEvent&, const Input&) override { log->push_back("move"); }
    void OnMouseScroll(const MouseScrollEvent&, const Input&) override { log->push_back("scroll"); }
    void Update(const FrameTime&, const Input& input) override {
        if (auto* keyboard = Self().TryGet<KeyboardControl>()) queriedDown = keyboard->IsDown(input, Key::A);
        log->push_back("update");
    }
    void Stop() override { log->push_back("stop"); }
};
}

TEST(EntityInputEventsKeepInterleavedOrderAfterStartAndBeforeUpdate) {
    std::vector<std::string> log;
    World world;
    auto entity = world.Create();
    entity.Add<KeyboardControl>(); entity.Add<MouseControl>();
    entity.AddBehaviour<InputProbe>().log = &log;
    Input input;
    input.SetKey(65, InputAction::Press, InputModifiers::Shift, 30);
    input.SetCursor(pxPoint(12, 18));
    input.SetKey(65, InputAction::Release, InputModifiers::None, 30);
    input.SetMouseButton(1, InputAction::Press, InputModifiers::Control);
    input.AddScroll(pxVec2(1, 2));
    input.SetKey(65, InputAction::Press, InputModifiers::None, 30);
    input.SetKey(65, InputAction::Repeat, InputModifiers::None, 30);
    world.Step(0.1, input);
    CHECK(log == (std::vector<std::string>{"start", "key1", "move", "key0", "button", "scroll", "key1", "key2", "update"}));
    CHECK(entity.Get<InputProbe>().queriedDown);
    input.EndUpdate(); log.clear(); world.Step(0.1, input);
    CHECK(log == (std::vector<std::string>{"update"}));
    CHECK(entity.Get<InputProbe>().queriedDown);
}

TEST(EntityInputIsOptInAndDisabledQueriesAreNeutral) {
    std::vector<std::string> keyboardLog, mouseLog, absentLog;
    World world;
    auto keyboard = world.Create(); keyboard.Add<KeyboardControl>(); keyboard.AddBehaviour<InputProbe>().log = &keyboardLog;
    auto mouse = world.Create(); mouse.Add<MouseControl>(); mouse.AddBehaviour<InputProbe>().log = &mouseLog;
    auto absent = world.Create(); absent.AddBehaviour<InputProbe>().log = &absentLog;
    Input input; input.SetKey(65, true); input.SetCursor(pxPoint(20, 30)); input.SetMouseButton(0, true); input.AddScroll(pxVec2(0, 2));
    world.Step(0.1, input);
    CHECK(keyboardLog == (std::vector<std::string>{"start", "key1", "update"}));
    CHECK(mouseLog == (std::vector<std::string>{"start", "move", "button", "scroll", "update"}));
    CHECK(absentLog == (std::vector<std::string>{"start", "update"}));
    auto& keys = keyboard.Get<KeyboardControl>(); keys.enabled = false;
    auto& pointer = mouse.Get<MouseControl>(); pointer.enabled = false;
    CHECK(!keys.IsDown(input, Key::A)); CHECK(!keys.WasPressed(input, Key::A));
    CHECK(keys.Modifiers(input) == InputModifiers::None);
    CHECK(!pointer.IsDown(input, MouseButton::Left)); CHECK(!pointer.WasPressed(input, MouseButton::Left));
    CHECK(pointer.Position(input) == pxPoint(0, 0)); CHECK(pointer.Movement(input) == pxVec2(0)); CHECK(pointer.Scroll(input) == pxVec2(0));
    keyboardLog.clear(); mouseLog.clear(); input.EndUpdate(); input.SetKey(65, false); input.SetMouseButton(0, false);
    world.Step(0.1, input);
    CHECK(keyboardLog == (std::vector<std::string>{"update"})); CHECK(mouseLog == (std::vector<std::string>{"update"}));
}

TEST(EntityInputRechecksEnableAndPreservesDeferredDestructionLifecycle) {
    std::vector<std::string> disabled, destroyed;
    World world;
    auto first = world.Create(); first.Add<KeyboardControl>();
    auto& a = first.AddBehaviour<InputProbe>(); a.log = &disabled; a.disableOnKey = true;
    auto second = world.Create(); second.Add<KeyboardControl>();
    auto& b = second.AddBehaviour<InputProbe>(); b.log = &destroyed; b.destroyOnKey = true;
    Input input; input.SetKey(65, true); input.SetKey(65, false);
    world.Step(0.1, input);
    CHECK(disabled == (std::vector<std::string>{"start", "key1", "update"}));
    CHECK(!first.Get<InputProbe>().queriedDown);
    // Destroy is deferred exactly like destruction from Update: readable until the step ends.
    CHECK(destroyed == (std::vector<std::string>{"start", "key1", "key0", "update", "stop"}));
    CHECK(!second.IsAlive());
}

TEST(EntityInputRejectsReentrantWorldStepWithoutCorruptingOuterStep) {
    std::vector<std::string> log;
    World world; auto entity = world.Create(); entity.Add<KeyboardControl>();
    auto& probe = entity.AddBehaviour<InputProbe>(); probe.log = &log; probe.tryReentry = true;
    Input input; input.SetKey(65, true); world.Step(0.1, input);
    CHECK(entity.Get<InputProbe>().reentryRejected); CHECK(!world.IsStepping()); CHECK(world.Steps() == 1);
}

TEST(GenericEntityInputComponentsPersistWithoutAssumingMovement) {
    ComponentTypes types; DescribeRuntimeComponents(types);
    World source; auto entity = SceneEntityWith<KeyboardControl>(source, "device-only");
    entity.Get<KeyboardControl>().enabled = false; entity.Add<MouseControl>().enabled = false;
    World target; CHECK(ReadScene(target, types, WriteScene(source, types)).ok);
    auto keys = target.Registry().view<KeyboardControl, MouseControl>();
    CHECK(keys.begin() != keys.end());
    CHECK(!target.Registry().get<KeyboardControl>(*keys.begin()).enabled);
    CHECK(!target.Registry().get<MouseControl>(*keys.begin()).enabled);
    types.Add<InputProbe>("InputProbe").Source("src/components/InputProbe.cpp");
    CHECK(types.All().back().source == "src/components/InputProbe.cpp");
}

TEST(WorldRejectsReentryFromDeferredActionsAsWellAsInputCallbacks) {
    World world; bool rejected = false;
    world.AddSystem(SystemPhase::PreUpdate, "defer-step", [&](World& owner, const FrameTime&, const Input& input) {
        owner.Defer([&](World& current) {
            try { current.Step(0.1, input); }
            catch (const std::logic_error&) { rejected = true; }
        });
    });
    Input input; world.Step(0.1, input);
    CHECK(rejected); CHECK(world.Steps() == 1); CHECK(!world.IsStepping());
}

TEST(AuthoredInputTriggersRoundTripAndDispatchMatchingEventsInOrder) {
    ComponentTypes types;
    DescribeRuntimeComponents(types);
    const auto& triggerType = types.All().at(static_cast<std::size_t>(types.IndexOf("InputTrigger")));
    const auto& componentType = types.All().at(static_cast<std::size_t>(types.IndexOf("InputTriggers")));
    CHECK(triggerType.valueType);
    CHECK(componentType.fields.at(0).kind == FieldKind::RecordList);
    CHECK(componentType.fields.at(0).recordType == triggerType.type);

    World authored;
    constexpr TriggerActionId toggle{0x1e0987302f67d3ba};
    auto entity = SceneEntityWith<InputTriggers>(authored, "controls");
    entity.Get<InputTriggers>().bindings = {
        {{TriggerType::Key, true, static_cast<int>(Key::F1), TriggerEdge::Press, 2, toggle}, {}},
        {{TriggerType::MouseButton, true, 1, TriggerEdge::Release, 0, "ToggleHUD"}, {}},
        {{TriggerType::Scroll, false, -1, TriggerEdge::Any, 0, "ToggleHUD"}, {}},
    };

    World world;
    const auto saved = WriteScene(authored, types);
    CHECK(saved.find("action:1e0987302f67d3ba") != std::string::npos);
    CHECK(ReadScene(world, types, saved).ok);
    std::vector<InputEvent> received;
    CHECK(world.Triggers().Subscribe(toggle, "RenamedAction", [&](Entity source, const InputEvent& event) {
        CHECK(source.Has<InputTriggers>() && std::holds_alternative<KeyEvent>(event));
        received.push_back(event);
    }) != 0);
    CHECK(world.Triggers().Subscribe(TriggerActionId{123}, "RenamedAction", [](Entity, const InputEvent&) { CHECK(false); }) != 0);
    CHECK(world.Triggers().Subscribe("ToggleHUD", [&](Entity source, const InputEvent& event) {
        CHECK(source.Has<InputTriggers>());
        received.push_back(event);
    }) != 0);
    Input input;
    input.SetKey(static_cast<int>(Key::F1), InputAction::Press, InputModifiers::None);
    input.SetKey(static_cast<int>(Key::F1), InputAction::Release, InputModifiers::None);
    input.SetKey(static_cast<int>(Key::F1), InputAction::Press, InputModifiers::Control | InputModifiers::Shift, 59);
    input.SetKey(static_cast<int>(Key::F1), InputAction::Repeat, InputModifiers::Control);
    input.SetCursor(pxPoint(11, 12));
    input.SetMouseButton(1, true);
    input.AddScroll(pxVec2(0, 1));
    input.SetMouseButton(1, false);
    world.Step(0.1, input);
    CHECK(received.size() == 2);
    if (received.size() == 2) {
        const auto* key = std::get_if<KeyEvent>(&received[0]);
        const auto* button = std::get_if<MouseButtonEvent>(&received[1]);
        CHECK(key && key->Pressed() && key->scanCode == 59);
        CHECK(button && button->Released() && button->position == pxPoint(11, 12));
    }
    input.EndUpdate();
    world.Step(0.1, input);
    CHECK(received.size() == 2);
}

TEST(InputTriggerSubscriptionsCanChangeDuringDispatchAndPostFromWorkers) {
    World world;
    auto entity = world.Create();
    entity.Add<InputTriggers>().bindings = {
        {{TriggerType::Text, true, -1, TriggerEdge::Any, 0, "Text"}, {}},
    };
    const auto owner = std::this_thread::get_id();
    std::vector<char32_t> received;
    auto& dispatcher = world.Triggers();
    TriggerDispatcher::SubscriptionId first = 0;
    TriggerDispatcher::SubscriptionId removed = 0;
    first = dispatcher.Subscribe("Text", [&](Entity, const InputEvent& event) {
        CHECK(std::this_thread::get_id() == owner);
        CHECK(dispatcher.Unsubscribe(removed));
        CHECK(dispatcher.Unsubscribe(first));
        received.push_back(std::get<TextInputEvent>(event).codepoint);
        CHECK(dispatcher.Subscribe("Text", [&](Entity, const InputEvent& next) {
            CHECK(std::this_thread::get_id() == owner);
            received.push_back(std::get<TextInputEvent>(next).codepoint);
        }) != 0);
        dispatcher.Post(TextInputEvent{U'Z'});
    });
    constexpr auto lastRegistrationLine = __LINE__ + 1;
    removed = dispatcher.Subscribe("Text", [](Entity, const InputEvent&) { CHECK(false); });
    const auto sources = dispatcher.ActionSources();
    CHECK(sources.size() == 2 && sources.back().action == "Text" &&
          std::string_view(sources.back().file).ends_with("InputControlTests.cpp") && sources.back().line == lastRegistrationLine);

    std::thread left([&] { dispatcher.Post(TextInputEvent{U'A'}); });
    std::thread right([&] { dispatcher.Post(TextInputEvent{U'B'}); });
    left.join();
    right.join();
    CHECK(received.empty());
    Input input;
    input.AddTextInput(U'H');
    world.Step(0.1, input);
    CHECK(received == (std::vector<char32_t>{U'H', U'A', U'B'}) ||
          received == (std::vector<char32_t>{U'H', U'B', U'A'}));
    input.EndUpdate();
    world.Step(0.1, input);
    CHECK(received.size() == 4 && received.back() == U'Z');
    CHECK(dispatcher.ActionSources().size() == 1);
}
