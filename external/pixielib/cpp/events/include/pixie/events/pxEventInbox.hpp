#pragma once

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

// Many threads may post owned events while the consumer takes one batch at a time.
// Deliver the returned batch on the event handler's owning thread, outside the lock.
template<typename T>
class pxEventInbox {
public:
    void Post(T event) {
        const std::lock_guard lock(m_mutex);
        m_pending.push_back(std::move(event));
    }

    [[nodiscard]] bool TryPost(T event, std::size_t capacity) {
        const std::lock_guard lock(m_mutex);
        if (m_pending.size() >= capacity) return false;
        m_pending.push_back(std::move(event));
        return true;
    }

    [[nodiscard]] std::vector<T> Take() {
        std::vector<T> batch;
        {
            const std::lock_guard lock(m_mutex);
            batch.swap(m_pending);
        }
        return batch;
    }

private:
    std::mutex m_mutex;
    std::vector<T> m_pending;
};
