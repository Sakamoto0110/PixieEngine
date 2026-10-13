#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

#include "pxCallback.hpp"
#include "pxSubscription.hpp"

// Moving a handler, or assigning to it, from inside one of its own callbacks is a bug, reported
// here. Define it before the include to report it some other way.
#ifndef PX_EVENTS_ASSERT
#define PX_EVENTS_ASSERT(cond, msg) assert((cond) && (msg))
#endif

// Callbacks invoked in order, what a C# multicast delegate is, with the same += and -=. It owns
// its callbacks by value: a copy of the handler copies them.
//
// While an Invoke runs, a callback may add or remove callbacks, itself included: one added runs
// from the next Invoke on, one removed before its turn does not run. A callback may also destroy
// the handler, as in `delete this`: the Invoke stops there, and the callback must not touch its
// own captures afterwards. Not thread-safe: using one handler from more than one thread needs a
// lock around it.
//
// Forward links two handlers live: invoking this one invokes the target too, with whatever
// callbacks the target has by then. The link ends with either handler and follows both when they
// move.
//
// Ported from pxEvents.hpp of 2023 (indev, b961405).
template<typename... TArgs>
class pxEventHandler : private pxEventsDetail::SubscriptionList {
public:
    using Callback = pxCallback<TArgs...>;

    pxEventHandler() noexcept : SubscriptionList(&RemoveById) {}

    // The copy gets the callbacks; the subscriptions and the forwards stay with the original.
    pxEventHandler(const pxEventHandler& o) : pxEventHandler() { Join(o); }

    pxEventHandler(pxEventHandler&& o) noexcept : pxEventHandler() { TakeFrom(o); }

    pxEventHandler& operator=(const pxEventHandler& o);
    pxEventHandler& operator=(pxEventHandler&& o) noexcept;

    ~pxEventHandler() {
        for (DispatchFrame* f = m_frames; f != nullptr; f = f->outer)
            f->destroyed = true;
        DetachAll();
    }

    void operator+=(Callback callback) { Add(std::move(callback)); }
    void operator+=(const pxEventHandler& o) { Join(o); }
    void operator-=(const Callback& callback) { Remove(callback); }
    void operator-=(const pxEventHandler& o) { Remove(o); }

    // Adds a callback at the end. The same callback can be added more than once, as in C#.
    void Add(Callback callback) {
        if (callback)
            Append(std::move(callback));
    }

    // Adds copies of the callbacks of o, as they are now.
    void Join(const pxEventHandler& o);

    // Removes the last occurrence of the callback, as C# does.
    bool Remove(const Callback& callback);

    // Removes the last run of callbacks equal to all of o's, in order, as C# does.
    bool Remove(const pxEventHandler& o);

    // Adds a callback that stays while the returned subscription lives.
    [[nodiscard]] pxSubscription Subscribe(Callback callback) {
        pxSubscription subscription;
        if (callback)
            Link(subscription, Append(std::move(callback)));
        return subscription;
    }

    // Invokes target at this point of the list, as if target.Invoke were a callback here. A link
    // that would close a loop (target already reaches this handler) is refused with false.
    bool Forward(pxEventHandler& target);

    // Ends the last link from this handler to target.
    bool Unforward(pxEventHandler& target);

    void Clear() noexcept {
        for (std::size_t i = 0; i < Size(); ++i)
            if (At(i).alive)
                Kill(At(i));
        Compact();
    }

    void Invoke(TArgs... args) { Dispatch(args...); }
    void operator()(TArgs... args) { Dispatch(args...); }

    std::size_t Count() const noexcept { return m_alive; }
    bool IsEmpty() const noexcept { return m_alive == 0; }

private:
    struct Slot {
        std::uint64_t id;
        Callback callback;
        bool alive;

        Slot(std::uint64_t i, Callback&& c) noexcept : id(i), callback(std::move(c)), alive(true) {}
    };

    // The callback that Forward adds. The target keeps the subscription of it, so the link ends
    // with the target, and fixes the pointer here when it moves (Retarget).
    struct ForwardBinding {
        pxEventHandler* target;
        void operator()(pxEventArg<TArgs>... args) const { target->Dispatch(args...); }
    };

    // One per running Invoke, on its stack, so the handler can tell each of them that it died.
    struct DispatchFrame {
        DispatchFrame* outer;
        bool destroyed;
    };

    // Ends an Invoke, also when a callback throws.
    struct DispatchScope {
        pxEventHandler& handler;
        DispatchFrame& frame;

        ~DispatchScope() {
            if (frame.destroyed)
                return;
            handler.m_frames = frame.outer;
            --handler.m_depth;
        }
    };

    std::size_t Size() const noexcept { return m_slots.size() + m_pending.size(); }

    Slot& At(std::size_t i) noexcept {
        return i < m_slots.size() ? m_slots[i] : m_pending[i - m_slots.size()];
    }

    const Slot& At(std::size_t i) const noexcept {
        return i < m_slots.size() ? m_slots[i] : m_pending[i - m_slots.size()];
    }

    std::uint64_t Append(Callback&& callback) {
        // During an Invoke, or while an earlier merge is still owed, new callbacks wait in
        // m_pending; ids keep growing along m_slots and then m_pending.
        std::vector<Slot>& target = (m_depth > 0 || !m_pending.empty()) ? m_pending : m_slots;
        const std::uint64_t id = m_nextId++;
        target.emplace_back(id, std::move(callback));
        ++m_alive;
        return id;
    }

    static bool IsForward(const Slot& slot) noexcept {
        return slot.callback.template Target<ForwardBinding>() != nullptr;
    }

    // Ids grow along m_slots and then m_pending.
    Slot* FindById(std::uint64_t id) noexcept {
        for (std::vector<Slot>* slots : { &m_slots, &m_pending }) {
            auto it = std::lower_bound(slots->begin(), slots->end(), id,
                [](const Slot& slot, std::uint64_t value) { return slot.id < value; });
            if (it != slots->end() && it->id == id)
                return &*it;
        }
        return nullptr;
    }

    static pxEventHandler& SourceOf(const pxSubscription& link) noexcept {
        return static_cast<pxEventHandler&>(*link.m_list);
    }

    // The slot behind one of the links in m_incoming, while both ends are alive.
    static Slot* SlotOf(const pxSubscription& link) noexcept {
        if (!link.IsActive())
            return nullptr;
        Slot* slot = SourceOf(link).FindById(link.m_id);
        return slot != nullptr && slot->alive ? slot : nullptr;
    }

    bool Reaches(const pxEventHandler& to) const noexcept;

    // A dead slot stays where it is until Compact, so no Invoke sees the vector move.
    void Kill(Slot& slot) noexcept {
        slot.alive = false;
        --m_alive;
        m_dirty = true;
    }

    void Dispatch(pxEventArg<TArgs>... args);
    void Compact() noexcept;
    void TakeFrom(pxEventHandler& o) noexcept;

    static void RemoveById(SubscriptionList& list, std::uint64_t id) noexcept;

    // m_slots never grows nor shrinks while an Invoke runs (m_depth > 0), so the callable being
    // called stays where it is.
    std::vector<Slot> m_slots;
    std::vector<Slot> m_pending;
    std::size_t m_alive = 0;
    std::uint64_t m_nextId = 1;
    int m_depth = 0;
    bool m_dirty = false;
    DispatchFrame* m_frames = nullptr;
    // The links that invoke this handler, one subscription in each source. Last, so they end
    // before the rest of the handler.
    std::vector<pxSubscription> m_incoming;
};

template<typename... TArgs>
pxEventHandler<TArgs...>& pxEventHandler<TArgs...>::operator=(const pxEventHandler& o) {
    if (this == &o)
        return *this;
    PX_EVENTS_ASSERT(m_depth == 0, "pxEventHandler: assigned to from inside one of its own callbacks");

    std::vector<Slot> slots;
    slots.reserve(o.m_alive);
    std::uint64_t id = m_nextId;
    for (std::size_t i = 0; i < o.Size(); ++i) {
        const Slot& slot = o.At(i);
        if (slot.alive && !IsForward(slot))
            slots.emplace_back(id++, Callback(slot.callback));
    }

    // Nothing throws from here on. The old callbacks die last, with the handler consistent.
    DetachAll();
    slots.swap(m_slots);
    std::vector<Slot> pending = std::move(m_pending);
    m_pending.clear();
    m_alive = m_slots.size();
    m_nextId = id;
    m_dirty = false;
    return *this;
}

template<typename... TArgs>
pxEventHandler<TArgs...>& pxEventHandler<TArgs...>::operator=(pxEventHandler&& o) noexcept {
    if (this == &o)
        return *this;
    PX_EVENTS_ASSERT(m_depth == 0, "pxEventHandler: assigned to from inside one of its own callbacks");

    // The old callbacks, and the links into the old ones, die last, with the handler consistent.
    std::vector<Slot> slots = std::move(m_slots);
    std::vector<Slot> pending = std::move(m_pending);
    std::vector<pxSubscription> incoming = std::move(m_incoming);
    m_incoming.clear();
    DetachAll();
    TakeFrom(o);
    return *this;
}

template<typename... TArgs>
void pxEventHandler<TArgs...>::Join(const pxEventHandler& o) {
    // The size comes first, so joining a handler with itself doubles it once, as in C#.
    const std::size_t size = o.Size();
    for (std::size_t i = 0; i < size; ++i) {
        const Slot& slot = o.At(i);
        if (slot.alive && !IsForward(slot))
            Append(Callback(slot.callback));
    }
}

template<typename... TArgs>
bool pxEventHandler<TArgs...>::Forward(pxEventHandler& target) {
    if (&target == this || target.Reaches(*this))
        return false;
    // Links whose source died or dropped them go first, so m_incoming does not grow forever.
    std::erase_if(target.m_incoming, [](const pxSubscription& link) { return SlotOf(link) == nullptr; });
    // Linked where it lives, in the vector, instead of moved in from a temporary.
    pxSubscription& link = target.m_incoming.emplace_back();
    try {
        Link(link, Append(Callback(ForwardBinding{ &target })));
    } catch (...) {
        target.m_incoming.pop_back();
        throw;
    }
    return true;
}

template<typename... TArgs>
bool pxEventHandler<TArgs...>::Unforward(pxEventHandler& target) {
    for (std::size_t i = target.m_incoming.size(); i-- > 0;) {
        pxSubscription& link = target.m_incoming[i];
        if (SlotOf(link) == nullptr || &SourceOf(link) != this)
            continue;
        link.Unsubscribe();
        target.m_incoming.erase(target.m_incoming.begin() + static_cast<std::ptrdiff_t>(i));
        return true;
    }
    return false;
}

template<typename... TArgs>
bool pxEventHandler<TArgs...>::Reaches(const pxEventHandler& to) const noexcept {
    // Forward refuses loops, so this walk always ends.
    for (std::size_t i = 0; i < Size(); ++i) {
        const Slot& slot = At(i);
        if (!slot.alive)
            continue;
        const ForwardBinding* link = slot.callback.template Target<ForwardBinding>();
        if (link != nullptr && (link->target == &to || link->target->Reaches(to)))
            return true;
    }
    return false;
}

template<typename... TArgs>
bool pxEventHandler<TArgs...>::Remove(const Callback& callback) {
    for (std::size_t i = Size(); i-- > 0;) {
        Slot& slot = At(i);
        if (slot.alive && slot.callback == callback) {
            Kill(slot);
            Compact();
            return true;
        }
    }
    return false;
}

template<typename... TArgs>
bool pxEventHandler<TArgs...>::Remove(const pxEventHandler& o) {
    std::vector<std::size_t> mine;
    std::vector<std::size_t> theirs;
    for (std::size_t i = 0; i < Size(); ++i)
        if (At(i).alive)
            mine.push_back(i);
    for (std::size_t i = 0; i < o.Size(); ++i)
        if (o.At(i).alive)
            theirs.push_back(i);
    if (theirs.empty() || theirs.size() > mine.size())
        return false;

    for (std::size_t start = mine.size() - theirs.size() + 1; start-- > 0;) {
        bool match = true;
        for (std::size_t j = 0; j < theirs.size() && match; ++j)
            match = At(mine[start + j]).callback == o.At(theirs[j]).callback;
        if (!match)
            continue;
        for (std::size_t j = 0; j < theirs.size(); ++j)
            Kill(At(mine[start + j]));
        Compact();
        return true;
    }
    return false;
}

template<typename... TArgs>
void pxEventHandler<TArgs...>::Dispatch(pxEventArg<TArgs>... args) {
    Compact();

    DispatchFrame frame{ m_frames, false };
    m_frames = &frame;
    ++m_depth;
    {
        DispatchScope scope{ *this, frame };
        // Callbacks added from here on are in m_pending, past this size.
        const std::size_t size = m_slots.size();
        for (std::size_t i = 0; i < size; ++i) {
            Slot& slot = m_slots[i];
            if (!slot.alive)
                continue;
            slot.callback.Call(args...);
            if (frame.destroyed)
                return;
        }
    }
    Compact();
}

// Drops the dead slots and merges the pending ones, only when no Invoke runs. Each removed
// callable is destroyed while the handler is consistent, so its destructor may += or -= it.
template<typename... TArgs>
void pxEventHandler<TArgs...>::Compact() noexcept {
    while (m_depth == 0 && (m_dirty || !m_pending.empty())) {
        if (m_dirty) {
            m_dirty = false;
            std::size_t kept = 0;
            for (std::size_t i = 0; i < m_slots.size(); ++i) {
                if (!m_slots[i].alive)
                    continue;
                if (i != kept)
                    std::swap(m_slots[kept], m_slots[i]);
                ++kept;
            }
            // Counting as an Invoke keeps m_slots where it is while the destructors run.
            ++m_depth;
            for (std::size_t i = kept; i < m_slots.size(); ++i)
                m_slots[i].callback = Callback();
            --m_depth;
            m_slots.erase(m_slots.begin() + static_cast<std::ptrdiff_t>(kept), m_slots.end());
        }

        if (!m_pending.empty()) {
            std::vector<Slot> incoming;
            incoming.swap(m_pending);
            try {
                m_slots.reserve(m_slots.size() + incoming.size());
            } catch (...) {
                incoming.swap(m_pending); // merged at the next chance
                return;
            }
            for (Slot& slot : incoming)
                if (slot.alive)
                    m_slots.push_back(std::move(slot));
            // The dead ones in `incoming` die here, out of the handler.
        }
    }
}

template<typename... TArgs>
void pxEventHandler<TArgs...>::TakeFrom(pxEventHandler& o) noexcept {
    PX_EVENTS_ASSERT(o.m_depth == 0, "pxEventHandler: moved from inside one of its own callbacks");
    m_slots = std::move(o.m_slots);
    m_pending = std::move(o.m_pending);
    o.m_slots.clear();
    o.m_pending.clear();
    m_alive = std::exchange(o.m_alive, 0);
    m_nextId = o.m_nextId; // o keeps counting from there: an id is never reused
    m_dirty = std::exchange(o.m_dirty, false);
    AdoptFrom(o);

    // The links into o now invoke this handler.
    m_incoming = std::move(o.m_incoming);
    o.m_incoming.clear();
    for (const pxSubscription& link : m_incoming)
        if (Slot* slot = SlotOf(link))
            slot->callback.template Target<ForwardBinding>()->target = this;
}

template<typename... TArgs>
void pxEventHandler<TArgs...>::RemoveById(SubscriptionList& list, std::uint64_t id) noexcept {
    pxEventHandler& self = static_cast<pxEventHandler&>(list);
    Slot* slot = self.FindById(id);
    if (slot != nullptr && slot->alive) {
        self.Kill(*slot);
        self.Compact();
    }
}
