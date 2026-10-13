#include <type_traits>
#include <utility>

#include <pixie/events/pxEvent.hpp>

#include "check.hpp"

namespace {
    class Button {
    public:
        pxEvent<Button, Button&, int> Click;

        void Press(int times) { Click(*this, times); }
        // Only the owner can make its event the target of a Forward.
        bool Relay(pxEventHandler<Button&, int>& source) { return source.Forward(Click); }
        bool HasListeners() const { return !Click.IsEmpty(); }
        void Reset() { Click.Clear(); }
    };

    int total = 0;
    void Count(Button&, int times) { total += times; }

    using ClickEvent = pxEvent<Button, Button&, int>;
}

// Outside the owner, the event only takes += -= and Subscribe, as a C# event.
static_assert(!std::is_invocable_v<ClickEvent&, Button&, int>);
static_assert(!std::is_copy_constructible_v<ClickEvent> && !std::is_copy_assignable_v<ClickEvent>);
static_assert(!std::is_move_constructible_v<ClickEvent> && !std::is_move_assignable_v<ClickEvent>);
static_assert(!std::is_convertible_v<ClickEvent&, pxEventHandler<Button&, int>&>);
// The owner is still copyable and movable, with its event.
static_assert(std::is_copy_constructible_v<Button> && std::is_move_assignable_v<Button>);

void TestEvent() {
    Button button;
    PX_CHECK(!button.HasListeners());
    button.Click += &Count;
    int seen = 0;
    pxSubscription s = button.Click.Subscribe([&seen](Button&, int times) { seen = times; });
    button.Press(3);
    PX_CHECK(total == 3 && seen == 3);

    button.Click -= &Count;
    button.Press(2);
    PX_CHECK(total == 3 && seen == 2);

    // Moving the owner moves the event, and the subscription follows it.
    Button other = std::move(button);
    s.Unsubscribe();
    PX_CHECK(!other.HasListeners());

    pxEventHandler<Button&, int> handlers;
    handlers += &Count;
    other.Click += handlers;
    other.Press(1);
    PX_CHECK(total == 4);
    other.Click -= handlers;
    PX_CHECK(!other.HasListeners());
    other.Click += &Count;
    other.Reset();
    PX_CHECK(!other.HasListeners());

    // Forwarded out to a handler, and in from one by the owner.
    pxEventHandler<Button&, int> outside;
    outside += &Count;
    PX_CHECK(other.Click.Forward(outside));
    other.Press(5);
    PX_CHECK(total == 9);
    PX_CHECK(other.Click.Unforward(outside));
    pxEventHandler<Button&, int> source;
    other.Click += &Count;
    PX_CHECK(other.Relay(source));
    source(other, 10);
    PX_CHECK(total == 19);
}
