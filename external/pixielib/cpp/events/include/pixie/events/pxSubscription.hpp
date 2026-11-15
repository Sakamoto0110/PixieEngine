#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

namespace pxEventsDetail {

    // The handler's storage moves with the event. A token can lock this weak owner even if
    // destruction, assignment or an invocation is already in progress on another thread.
    class SubscriptionOwner {
    public:
        virtual bool KeepsSubscriptions(std::uint64_t epoch) const noexcept = 0;
        virtual void Remove(std::uint64_t id, std::uint64_t epoch) noexcept = 0;
        virtual ~SubscriptionOwner() = default;
    };
}

// Owns one subscription. An invocation that has already selected its callback may finish after
// Unsubscribe. Keep borrowed callback targets alive until their in-flight calls have finished.
class pxSubscription {
public:
    pxSubscription() noexcept = default;
    pxSubscription(pxSubscription&& other) noexcept : m_link(other.Take()) {}
    pxSubscription(const pxSubscription&) = delete;
    pxSubscription& operator=(const pxSubscription&) = delete;

    pxSubscription& operator=(pxSubscription&& other) noexcept {
        if (this != &other) {
            auto link = other.Take();
            {
                const std::lock_guard lock(m_mutex);
                std::swap(link, m_link);
            }
            Remove(link);
        }
        return *this;
    }

    ~pxSubscription() { Unsubscribe(); }

    void Unsubscribe() noexcept { Remove(Take()); }

    // Lets go of the callback, which stays subscribed until explicitly removed or the event ends.
    void Detach() noexcept { Take(); }

    // True while linked to the same live handler, even if the callback was explicitly removed.
    bool IsActive() const noexcept {
        Link link;
        {
            const std::lock_guard lock(m_mutex);
            link = m_link;
        }
        const auto owner = link.owner.lock();
        return owner && owner->KeepsSubscriptions(link.epoch);
    }

private:
    template<typename...> friend class pxEventHandler;

    struct Link {
        std::weak_ptr<pxEventsDetail::SubscriptionOwner> owner;
        std::uint64_t id = 0;
        std::uint64_t epoch = 0;
    };

    explicit pxSubscription(Link link) noexcept : m_link(std::move(link)) {}

    Link Take() noexcept {
        const std::lock_guard lock(m_mutex);
        return std::exchange(m_link, {});
    }

    static void Remove(const Link& link) noexcept {
        if (const auto owner = link.owner.lock()) owner->Remove(link.id, link.epoch);
    }

    mutable std::mutex m_mutex;
    Link m_link;
};
