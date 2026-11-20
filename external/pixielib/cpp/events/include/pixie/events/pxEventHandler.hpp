#pragma once

#include "pxCallback.hpp"
#include "pxSubscription.hpp"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <set>
#include <span>
#include <utility>
#include <vector>

#ifndef PX_EVENTS_ASSERT
#define PX_EVENTS_ASSERT(cond, msg) assert((cond) && (msg))
#endif

// An ordered multicast event. Registry operations and invocations can run concurrently. Each
// callable's execution is serialized, and callbacks run without the registry lock. Mutating a
// borrowed argument or a target shared by several callbacks still needs caller synchronization.
template<typename... TArgs>
class pxEventHandler {
public:
    using Callback = pxCallback<TArgs...>;

    pxEventHandler() noexcept = default;
    pxEventHandler(const pxEventHandler& other) { Join(other); }
    pxEventHandler(pxEventHandler&& other) noexcept : m_state(other.TakeState()) {}
    ~pxEventHandler() { if (const auto state = ReadState()) state->Close(); }

    pxEventHandler& operator=(const pxEventHandler& other) {
        if (this != &other) WriteState()->Replace(CopyCallbacks(other.ReadState()));
        return *this;
    }

    pxEventHandler& operator=(pxEventHandler&& other) noexcept {
        if (this != &other) {
            if (auto state = ReadState()) state->AssertIdle();
            if (auto retired = ExchangeState(other.TakeState())) retired->Close();
        }
        return *this;
    }

    void operator+=(Callback callback) { Add(std::move(callback)); }
    void operator+=(const pxEventHandler& other) { Join(other); }
    void operator-=(const Callback& callback) { Remove(callback); }
    void operator-=(const pxEventHandler& other) { Remove(other); }

    void Add(Callback callback) { if (callback) Subscribe(std::move(callback)).Detach(); }

    void Join(const pxEventHandler& other) {
        auto copies = CopyCallbacks(other.ReadState());
        if (!copies.empty()) WriteState()->Append(std::move(copies));
    }

    bool Remove(const Callback& callback) {
        return RemoveMatching([&](const Slots& slots) -> std::span<const SlotPtr> {
            for (std::size_t i = slots.size(); i-- > 0;) {
                if (slots[i]->Matches(callback)) return std::span(slots).subspan(i, 1);
            }
            return {};
        });
    }

    bool Remove(const pxEventHandler& other) {
        const auto state = other.ReadState();
        if (!state) return false;
        const auto pattern = state->ReadCallbacks().slots;
        return RemoveMatching([&](const Slots& slots) -> std::span<const SlotPtr> {
            if (pattern.empty() || pattern.size() > slots.size()) return {};
            for (std::size_t start = slots.size() - pattern.size() + 1; start-- > 0;) {
                if (std::equal(pattern.begin(), pattern.end(), slots.begin() + static_cast<std::ptrdiff_t>(start),
                               [](const SlotPtr& a, const SlotPtr& b) { return a->Matches(*b); })) {
                    return std::span(slots).subspan(start, pattern.size());
                }
            }
            return {};
        });
    }

    [[nodiscard]] pxSubscription Subscribe(Callback callback) {
        if (!callback) return {};
        const auto state = WriteState();
        auto slot = std::make_shared<Slot>(std::move(callback));
        const std::lock_guard lock(state->mutex);
        if (state->closed) return {};
        state->Insert(slot);
        return pxSubscription({state, slot->id, state->epoch});
    }

    bool Forward(pxEventHandler& target) {
        if (this == &target) return false;
        return Link(WriteState(), target.WriteState());
    }

    bool Unforward(pxEventHandler& target) {
        const auto source = ReadState();
        const auto destination = target.ReadState();
        if (!source || !destination) return false;
        const std::lock_guard lock(source->mutex);
        const auto found = std::find_if(source->slots.rbegin(), source->slots.rend(), [&](const SlotPtr& slot) {
            return slot->forward.lock() == destination;
        });
        if (found == source->slots.rend()) return false;
        (*found)->alive = false;
        source->slots.erase(std::prev(found.base()));
        ++source->revision;
        return true;
    }

    void Clear() noexcept { if (const auto state = ReadState()) state->Clear(false); }
    std::size_t Count() const noexcept {
        const auto state = ReadState();
        if (!state) return 0;
        const std::lock_guard lock(state->mutex);
        return static_cast<std::size_t>(std::count_if(state->slots.begin(), state->slots.end(),
            [](const SlotPtr& slot) { return slot->IsLive(); }));
    }
    bool IsEmpty() const noexcept { return Count() == 0; }

    void Invoke(TArgs... args) { if (const auto state = ReadState()) Dispatch(state, args...); }
    void operator()(TArgs... args) { if (const auto state = ReadState()) Dispatch(state, args...); }

private:
    struct State;
    using StatePtr = std::shared_ptr<State>;

    struct Slot {
        explicit Slot(Callback value) : callback(std::move(value)) {}
        explicit Slot(const StatePtr& target) : forward(target), isForward(true) {}

        bool IsLive() const {
            if (!alive) return false;
            if (!isForward) return true;
            const auto target = forward.lock();
            return target && !target->closed;
        }

        Callback Copy() const {
            const std::lock_guard lock(callMutex);
            return callback;
        }
        bool Matches(const Callback& value) const {
            const std::lock_guard lock(callMutex);
            return callback == value;
        }
        bool Matches(const Slot& other) const {
            if (this == &other) return true;
            const std::scoped_lock lock(callMutex, other.callMutex);
            return callback == other.callback;
        }
        void Call(pxEventArg<TArgs>... args) {
            const std::lock_guard lock(callMutex);
            if (alive) callback.Call(args...);
        }

        mutable std::recursive_mutex callMutex;
        Callback callback;
        std::weak_ptr<State> forward;
        const bool isForward = false;
        std::atomic<bool> alive{true};
        std::uint64_t id = 0;
    };

    using SlotPtr = std::shared_ptr<Slot>;
    using Slots = std::vector<SlotPtr>;

    struct State final : pxEventsDetail::SubscriptionOwner {
        mutable std::mutex mutex;
        Slots slots;
        std::atomic<bool> closed{false};
        std::uint64_t epoch = 0;
        std::uint64_t nextId = 1;
        std::uint64_t revision = 0;
        std::uint64_t dispatchCutoff = 0;
        std::size_t dispatches = 0;

        void Insert(SlotPtr slot) {
            slot->id = nextId;
            slots.push_back(std::move(slot));
            ++nextId;
            ++revision;
        }

        void Append(Slots copies) {
            const std::lock_guard lock(mutex);
            if (closed) return;
            slots.reserve(slots.size() + copies.size());
            for (auto& slot : copies) Insert(std::move(slot));
        }

        void Replace(Slots replacement) {
            const std::lock_guard lock(mutex);
            if (closed) return;
            for (auto& slot : replacement) slot->id = nextId++;
            for (const auto& slot : slots) slot->alive = false;
            slots.swap(replacement);
            ++epoch;
            ++revision;
        }

        void AssertIdle() const noexcept {
            const std::lock_guard lock(mutex);
            PX_EVENTS_ASSERT(dispatches == 0, "pxEventHandler: moved during one of its invocations");
        }

        void Clear(bool close) noexcept {
            Slots retired;
            {
                const std::lock_guard lock(mutex);
                if (close) closed = true;
                for (const auto& slot : slots) slot->alive = false;
                retired.swap(slots);
                ++revision;
            }
        }

        void Close() noexcept { Clear(true); }

        bool KeepsSubscriptions(std::uint64_t expected) const noexcept override {
            const std::lock_guard lock(mutex);
            return !closed && epoch == expected;
        }

        void Remove(std::uint64_t id, std::uint64_t expected) noexcept override {
            SlotPtr retired;
            {
                const std::lock_guard lock(mutex);
                if (closed || epoch != expected) return;
                const auto found = std::lower_bound(slots.begin(), slots.end(), id,
                    [](const SlotPtr& slot, std::uint64_t value) { return slot->id < value; });
                if (found == slots.end() || (*found)->id != id) return;
                retired = std::move(*found);
                retired->alive = false;
                slots.erase(found);
                ++revision;
            }
        }

        struct Snapshot { Slots slots; std::uint64_t revision; };
        Snapshot ReadCallbacks() const {
            const std::lock_guard lock(mutex);
            Slots result;
            for (const auto& slot : slots) if (!slot->isForward) result.push_back(slot);
            return {std::move(result), revision};
        }

        bool RemoveMatching(std::span<const SlotPtr> matching, std::uint64_t expected) {
            const std::lock_guard lock(mutex);
            if (revision != expected) return false;
            if (matching.empty()) return true;
            for (const auto& slot : matching) slot->alive = false;
            // The caller's snapshot keeps these callbacks alive until after the lock is released.
            std::erase_if(slots, [](const SlotPtr& slot) { return !slot->alive; });
            ++revision;
            return true;
        }

        Slots BeginDispatch() {
            const std::lock_guard lock(mutex);
            if (dispatches == 0) dispatchCutoff = nextId;
            const auto end = std::lower_bound(slots.begin(), slots.end(), dispatchCutoff,
                [](const SlotPtr& slot, std::uint64_t cutoff) { return slot->id < cutoff; });
            Slots current(slots.begin(), end);
            ++dispatches;
            return current;
        }

        void EndDispatch() noexcept {
            const std::lock_guard lock(mutex);
            --dispatches;
        }
    };

    template<typename Match>
    bool RemoveMatching(Match match) {
        const auto state = ReadState();
        if (!state) return false;
        for (;;) {
            auto snapshot = state->ReadCallbacks();
            const auto matching = match(snapshot.slots);
            if (state->RemoveMatching(matching, snapshot.revision)) return !matching.empty();
        }
    }

    static Slots CopyCallbacks(const StatePtr& state) {
        Slots copies;
        if (state) {
            const auto snapshot = state->ReadCallbacks();
            copies.reserve(snapshot.slots.size());
            for (const auto& slot : snapshot.slots) copies.push_back(std::make_shared<Slot>(slot->Copy()));
        }
        return copies;
    }

    static void Dispatch(const StatePtr& state, pxEventArg<TArgs>... args) {
        const auto slots = state->BeginDispatch();
        struct DispatchScope { State& state; ~DispatchScope() { state.EndDispatch(); } } scope{*state};
        for (const auto& slot : slots) {
            if (!slot->alive) continue;
            if (slot->isForward) {
                if (const auto target = slot->forward.lock(); target && !target->closed) Dispatch(target, args...);
            } else {
                slot->Call(args...);
            }
        }
    }

    static bool Link(const StatePtr& source, const StatePtr& target) {
        if (source == target) return false;
        // Lock the source and the target's reachable graph in address order. If traversal
        // finds more nodes, retry with those locks included before checking and inserting.
        const auto order = [](const StatePtr& a, const StatePtr& b) { return std::less<State*>{}(a.get(), b.get()); };
        std::vector<StatePtr> graph{source, target};
        for (;;) {
            std::sort(graph.begin(), graph.end(), order);
            graph.erase(std::unique(graph.begin(), graph.end()), graph.end());
            std::vector<StatePtr> missing;
            {
                std::vector<std::unique_lock<std::mutex>> locks;
                locks.reserve(graph.size());
                for (const auto& state : graph) locks.emplace_back(state->mutex);
                if (source->closed || target->closed) return false;
                std::set<State*> visited;
                std::vector<StatePtr> reachable{target};
                while (!reachable.empty()) {
                    auto state = std::move(reachable.back());
                    reachable.pop_back();
                    if (state == source) return false;
                    if (!visited.insert(state.get()).second) continue;
                    if (!std::binary_search(graph.begin(), graph.end(), state, order)) {
                        missing.push_back(std::move(state));
                        continue;
                    }
                    for (const auto& slot : state->slots) {
                        if (auto next = slot->forward.lock(); next && !next->closed) reachable.push_back(std::move(next));
                    }
                }
                if (missing.empty()) {
                    std::erase_if(source->slots, [](const SlotPtr& slot) { return slot->isForward && !slot->IsLive(); });
                    source->Insert(std::make_shared<Slot>(target));
                    return true;
                }
            }
            graph.insert(graph.end(), missing.begin(), missing.end());
        }
    }

    StatePtr ReadState() const noexcept {
        const std::lock_guard lock(m_stateMutex);
        return m_state;
    }

    StatePtr WriteState() {
        const std::lock_guard lock(m_stateMutex);
        if (!m_state) m_state = std::make_shared<State>();
        return m_state;
    }

    StatePtr ExchangeState(StatePtr next) noexcept {
        const std::lock_guard lock(m_stateMutex);
        return std::exchange(m_state, std::move(next));
    }

    StatePtr TakeState() noexcept {
        if (auto state = ReadState()) state->AssertIdle();
        return ExchangeState({});
    }

    mutable std::mutex m_stateMutex;
    StatePtr m_state;
};
