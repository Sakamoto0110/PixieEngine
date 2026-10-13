#pragma once

#include <utility>

#include "pxEventHandler.hpp"

// An event declared as a member of TOwner, what the C# `event` keyword gives: anyone can +=, -=,
// subscribe or forward it to a handler; only TOwner can invoke it, clear it, count it, copy it,
// assign to it or make it the target of a Forward.
//
//     class Button {
//     public:
//         pxEvent<Button, Button&> Click;
//         void Press() { Click(*this); }
//     };
template<typename TOwner, typename... TArgs>
class pxEvent : private pxEventHandler<TArgs...> {
    friend TOwner;
    using Handler = pxEventHandler<TArgs...>;

public:
    using Callback = typename Handler::Callback;

    pxEvent() noexcept = default;
    ~pxEvent() = default;

    void operator+=(Callback callback) { Handler::Add(std::move(callback)); }
    void operator+=(const Handler& callbacks) { Handler::Join(callbacks); }
    void operator-=(const Callback& callback) { Handler::Remove(callback); }
    void operator-=(const Handler& callbacks) { Handler::Remove(callbacks); }

    [[nodiscard]] pxSubscription Subscribe(Callback callback) { return Handler::Subscribe(std::move(callback)); }

    bool Forward(Handler& target) { return Handler::Forward(target); }
    bool Unforward(Handler& target) { return Handler::Unforward(target); }

private:
    pxEvent(const pxEvent&) = default;
    pxEvent(pxEvent&&) noexcept = default;
    pxEvent& operator=(const pxEvent&) = default;
    pxEvent& operator=(pxEvent&&) noexcept = default;

    using Handler::Invoke;
    using Handler::operator();
    using Handler::Clear;
    using Handler::Count;
    using Handler::IsEmpty;
};
