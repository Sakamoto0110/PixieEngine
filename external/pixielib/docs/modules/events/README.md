# Events

**Document ID:** EVENTS-INTRODUCTION

**Schema version:** 1

**Kind:** guide

**Lifecycle:** current

**Subject:** introduction to the events module

**Surface:** introduction

**Boundary:** events

**Authority role:** non-normative

**Mutation:** authored

**Indexing:** include

C#'s delegates and events, for C++: a callback kept by value, a handler that
invokes its callbacks in order with `+=` and `-=`, a subscription that removes
its callback when it dies, and an event that only its owner can raise. No smart
pointers and no allocation for small callables; not thread-safe.

```cpp
#include <pixie/events/pxEvent.hpp>

class Button {
public:
    pxEvent<Button, Button&> Click;  // only Button can raise it
    void Press() { Click(*this); }
};

Button button;
pxSubscription s = button.Click.Subscribe([](Button&) { /* ... */ });
button.Press();  // runs the lambda; it is removed when s dies
```

The contract is the [technical reference](REFERENCE.md).
