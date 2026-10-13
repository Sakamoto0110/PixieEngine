# Events Technical Reference

**Document ID:** EVENTS-REFERENCE

**Schema version:** 1

**Kind:** reference

**Lifecycle:** current

**Subject:** the current contract of pxCallback, pxEventHandler, pxSubscription, and pxEvent

**Surface:** technical-reference

**Boundary:** events

**Authority role:** normative

**Mutation:** authored

**Indexing:** include

This reference owns the behavior of the C++ events. Numbers in parentheses,
such as (4.2), are the questions answered in
[PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md), section 4.6.

## Architecture

The `pixie::events` CMake target, headers only, C++20, in the global namespace,
one concept per file under `cpp/events/include/pixie/events/`: `pxCallback.hpp`,
`pxEventHandler.hpp`, `pxSubscription.hpp`, and `pxEvent.hpp`. It is a separate
module, so that whoever only wants the primitives does not pull it (1.10). It
was ported from the `pxEvents.hpp` of 2023 (branch `indev`, `b961405`); the
2023 `EventArgs`, `NOT_USE_STL`, `Dump`, and logger were left out (4.6).

Ownership is by value, without smart pointers (1.8): a handler owns copies of
its callbacks, and a callback owns its callable. Nothing here is thread-safe
(4.4): a handler used from more than one thread needs a lock around it.

## `pxCallback<TArgs...>`

One callable kept by value, what a C# delegate with one target is.

- **Construction**: from any callable invocable with the arguments (a function,
  a pointer to function, a lambda, a functor), and from an instance with one of
  its member functions, `{ &button, &Button::OnClick }`. A null pointer, a null
  instance, or a null member gives an empty callback. The callable must be
  copyable.
- **Storage**: a callable up to four pointers in size, suitably aligned and
  nothrow-movable, lives inside the callback with no allocation (4.7); a bigger
  one lives on the heap, owned by it. A copy copies the callable; a move leaves
  the source empty.
- **Arguments**: a value argument reaches each callable as a `const`
  reference, so no callback can change it for the next one (4.3); a reference
  argument reaches it as declared. A callable that asks for `T&` on an event of
  `T` does not compile; an event that lets callbacks change its argument is
  declared with a reference. An rvalue-reference argument type is refused.
- **Calling**: `operator()` calls the callable; an empty callback does nothing.
  `explicit operator bool` tells whether it holds one. A callable's return
  value is discarded.
- **Identity** (`==`), that of a C# delegate: two callbacks are equal when both
  are empty, or when they hold the same type and either are copies of the same
  callback (the same origin) or hold equal callables: the same function, the
  same instance and member, or a callable with no state. A lambda with captures
  equals only the copies of its own callback, as in C#. Pointers to virtual
  members compare as expected on GCC, Clang, and MSVC, though the standard
  leaves it unspecified.

## `pxEventHandler<TArgs...>`

Callbacks invoked in order, what a C# multicast delegate is.

- **`+=` and `Add`** append a callback at the end; an empty one is ignored. The
  same callback can be added more than once and then runs more than once
  (4.1). `+=` returns nothing (4.5).
- **`+=` with a handler, and `Join`**, append copies of its callbacks as they
  are now; joining a handler with itself doubles it once.
- **`-=` and `Remove`** remove the last occurrence of the callback, as C#, and
  return whether one was found. With a handler, they remove the last run of
  callbacks equal to all of that handler's, in order.
- **`Invoke` and `operator()`** call the callbacks in order. `Count()` is the
  number of live callbacks, `IsEmpty()` whether it is zero, and `Clear()`
  removes them all.
- **Changes during an `Invoke`**: a callback added during an `Invoke` runs from
  the next one on, as in C#; a callback removed before its turn does not run,
  unlike C#, because the common case is a target that stopped existing (4.2).
  The list never moves under the running callback, and a callback that removes
  itself does not make the next one be skipped. A callback may destroy the
  handler, as in `delete this`: the `Invoke` stops there, and the callback must
  not touch its own captures afterwards. If a callback throws, the exception
  leaves the `Invoke` and the handler stays usable.
- **Copy and move**: a copy gets the callbacks; the subscriptions and the
  forwards stay with the original. A move takes the callbacks, the
  subscriptions, and the links that invoke it. Moving or assigning to a handler
  from inside one of its own callbacks is a bug, reported through
  `PX_EVENTS_ASSERT`, which is `assert` unless defined before the include.
- **`Subscribe(callback)`** adds a callback and returns a `pxSubscription`
  (`[[nodiscard]]`) that removes it when it dies.
- **`Forward(target)`** links two handlers live (4.6, 4.8): invoking this one
  invokes `target`, at the link's place in the list, with whatever callbacks
  `target` has at that moment. A link to itself, or one that would close a loop
  (`target` already reaches this handler), is refused with `false`, so no
  `Invoke` recurses forever. **`Unforward(target)`** ends the last link from
  this handler to `target`. A link ends when either handler dies, and follows
  both when they move. A copy of a handler, and `+=` with a handler, do not
  carry its links.

## `pxSubscription`

A move-only handle that keeps one callback subscribed while it lives and
removes it when it dies or is assigned over. `Unsubscribe()` removes it now;
`Detach()` lets go and leaves the callback subscribed until a `-=` or the end of
the handler; `IsActive()` is true while it is linked to a living handler. If the
handler dies first, the subscription only becomes inactive. A handler and its
subscriptions form an intrusive list: the handler tells each one when it dies
or moves, with no allocation and no shared ownership.

## `pxEvent<TOwner, TArgs...>`

An event declared as a member of `TOwner`, what C#'s `event` keyword gives.
Anyone can `+=`, `-=`, `Subscribe`, `Forward` it to a handler, and `Unforward`
it. Only `TOwner` can invoke it, clear it, count it, copy it, move it, assign to
it, or make it the target of a `Forward`.

## Non-goals

- No thread safety (4.4).
- No `EventArgs` type; the arguments are the template's (4.6).
- No C# counterpart: C# has `event`.

## Validation expectations

The tests in `cpp/tests/pxCallback.test.cpp`, `pxEventHandler.test.cpp`,
`pxSubscription.test.cpp`, and `pxEvent.test.cpp` run on g++ (also with the
address and undefined-behavior sanitizers), Clang, and MSVC in CI. They catch
the three bugs of the 2023 code and failed on each of 23 deliberately broken
copies when they were written, listed in section 4.6 of
[PXL-ARCHIVE-001](../../archive/decision-notes-2026-10-12.md).
