#pragma once

#include <cstdint>
#include <utility>

class pxSubscription;

namespace pxEventsDetail {

    // The subscriptions linked to one event handler, as an intrusive list: the handler tells them
    // when it dies or moves, with no allocation and no shared ownership.
    class SubscriptionList {
    public:
        SubscriptionList(const SubscriptionList&) = delete;
        SubscriptionList& operator=(const SubscriptionList&) = delete;

    protected:
        using RemoveFn = void (*)(SubscriptionList&, std::uint64_t id) noexcept;

        explicit SubscriptionList(RemoveFn remove) noexcept : m_remove(remove) {}
        ~SubscriptionList() = default;

        void Link(pxSubscription& s, std::uint64_t id) noexcept;
        void DetachAll() noexcept;
        void AdoptFrom(SubscriptionList& o) noexcept;

    private:
        friend class ::pxSubscription;

        void Unlink(pxSubscription& s) noexcept;

        pxSubscription* m_head = nullptr;
        RemoveFn m_remove;
    };
}

// Keeps one callback subscribed while it lives and removes it when it dies, so an instance that
// subscribes one of its member functions does not leave it behind. If the handler dies first, the
// subscription only becomes inactive.
class pxSubscription {
public:
    pxSubscription() noexcept = default;
    pxSubscription(pxSubscription&& o) noexcept { TakeFrom(o); }
    pxSubscription(const pxSubscription&) = delete;
    pxSubscription& operator=(const pxSubscription&) = delete;

    pxSubscription& operator=(pxSubscription&& o) noexcept {
        if (this != &o) {
            Unsubscribe();
            TakeFrom(o);
        }
        return *this;
    }

    ~pxSubscription() { Unsubscribe(); }

    // Removes the callback from its handler, if the handler is still alive.
    void Unsubscribe() noexcept;

    // Lets go of the callback, which stays subscribed until a -= or the end of the handler.
    void Detach() noexcept;

    // True while linked to a living handler.
    bool IsActive() const noexcept { return m_list != nullptr; }

private:
    friend class pxEventsDetail::SubscriptionList;
    template<typename...> friend class pxEventHandler;

    void TakeFrom(pxSubscription& o) noexcept;

    pxEventsDetail::SubscriptionList* m_list = nullptr;
    std::uint64_t m_id = 0;
    pxSubscription* m_prev = nullptr;
    pxSubscription* m_next = nullptr;
};

inline void pxSubscription::Unsubscribe() noexcept {
    if (m_list == nullptr)
        return;
    pxEventsDetail::SubscriptionList& list = *m_list;
    const std::uint64_t id = m_id;
    list.Unlink(*this);
    list.m_remove(list, id);
}

inline void pxSubscription::Detach() noexcept {
    if (m_list != nullptr)
        m_list->Unlink(*this);
}

inline void pxSubscription::TakeFrom(pxSubscription& o) noexcept {
    m_list = std::exchange(o.m_list, nullptr);
    m_id = o.m_id;
    m_prev = std::exchange(o.m_prev, nullptr);
    m_next = std::exchange(o.m_next, nullptr);
    if (m_list == nullptr)
        return;
    (m_prev != nullptr ? m_prev->m_next : m_list->m_head) = this;
    if (m_next != nullptr)
        m_next->m_prev = this;
}

namespace pxEventsDetail {

    inline void SubscriptionList::Link(pxSubscription& s, std::uint64_t id) noexcept {
        s.m_list = this;
        s.m_id = id;
        s.m_prev = nullptr;
        s.m_next = m_head;
        if (m_head != nullptr)
            m_head->m_prev = &s;
        m_head = &s;
    }

    inline void SubscriptionList::Unlink(pxSubscription& s) noexcept {
        (s.m_prev != nullptr ? s.m_prev->m_next : m_head) = s.m_next;
        if (s.m_next != nullptr)
            s.m_next->m_prev = s.m_prev;
        s.m_list = nullptr;
        s.m_prev = nullptr;
        s.m_next = nullptr;
    }

    inline void SubscriptionList::DetachAll() noexcept {
        pxSubscription* s = std::exchange(m_head, nullptr);
        while (s != nullptr) {
            pxSubscription* next = s->m_next;
            s->m_list = nullptr;
            s->m_prev = nullptr;
            s->m_next = nullptr;
            s = next;
        }
    }

    inline void SubscriptionList::AdoptFrom(SubscriptionList& o) noexcept {
        DetachAll();
        m_head = std::exchange(o.m_head, nullptr);
        for (pxSubscription* s = m_head; s != nullptr; s = s->m_next)
            s->m_list = this;
    }
}
