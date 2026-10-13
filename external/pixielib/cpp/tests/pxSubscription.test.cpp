#include <utility>
#include <vector>

#include <pixie/events/pxEventHandler.hpp>

#include "check.hpp"

namespace {
    int hits = 0;
    void Hit() { ++hits; }

    int Fire(pxEventHandler<>& e) {
        hits = 0;
        e.Invoke();
        return hits;
    }

    // An instance that subscribes one of its member functions for as long as it lives.
    struct Listener {
        int count = 0;
        pxSubscription subscription;
        explicit Listener(pxEventHandler<>& e) : subscription(e.Subscribe({ this, &Listener::OnFire })) {}
        void OnFire() { ++count; }
    };
}

void TestSubscription() {
    // Gone with its scope.
    {
        pxEventHandler<> e;
        {
            pxSubscription s = e.Subscribe(&Hit);
            PX_CHECK(s.IsActive() && Fire(e) == 1);
        }
        PX_CHECK(e.IsEmpty() && Fire(e) == 0);

        // Detach leaves the callback subscribed.
        {
            pxSubscription s = e.Subscribe(&Hit);
            s.Detach();
            PX_CHECK(!s.IsActive());
        }
        PX_CHECK(Fire(e) == 1);
        e.Clear();

        // An empty callback gives an inactive subscription.
        PX_CHECK(!e.Subscribe(pxEventHandler<>::Callback()).IsActive());
    }

    // The handler dies first: the subscription only becomes inactive.
    {
        pxSubscription s;
        {
            pxEventHandler<> e;
            s = e.Subscribe(&Hit);
        }
        PX_CHECK(!s.IsActive());
        s.Unsubscribe();
    }

    // Several subscriptions, removed out of order, and moved around.
    {
        pxEventHandler<> e;
        std::vector<pxSubscription> subs;
        for (int i = 0; i < 4; ++i)
            subs.push_back(e.Subscribe(&Hit));
        PX_CHECK(Fire(e) == 4);
        subs.erase(subs.begin() + 1);
        PX_CHECK(Fire(e) == 3);
        pxSubscription moved = std::move(subs.back());
        subs.pop_back();
        PX_CHECK(Fire(e) == 3 && moved.IsActive());
        moved = std::move(subs.front());
        PX_CHECK(Fire(e) == 2);
        subs.clear();
        PX_CHECK(Fire(e) == 1);
        moved.Unsubscribe();
        PX_CHECK(e.IsEmpty());
    }

    // The subscriptions follow the handler when it moves, and stay with the original on a copy.
    {
        pxEventHandler<> a;
        pxSubscription s = a.Subscribe(&Hit);
        pxEventHandler<> copy = a;
        pxEventHandler<> b = std::move(a);
        PX_CHECK(Fire(b) == 1);
        s.Unsubscribe();
        PX_CHECK(b.IsEmpty() && Fire(copy) == 1);

        // Moved over: the target's own subscriptions end with its callbacks.
        pxSubscription mine = b.Subscribe(&Hit);
        pxSubscription theirs = copy.Subscribe(&Hit);
        b = std::move(copy);
        PX_CHECK(!mine.IsActive() && theirs.IsActive() && Fire(b) == 2);
        theirs.Unsubscribe();
        PX_CHECK(Fire(b) == 1);

        // Copied over: the same, and the copy gets no subscription.
        pxSubscription before = b.Subscribe(&Hit);
        pxEventHandler<> source;
        source += &Hit;
        pxSubscription kept = source.Subscribe(&Hit);
        b = source;
        PX_CHECK(!before.IsActive() && Fire(b) == 2);
        kept.Unsubscribe();
        PX_CHECK(Fire(b) == 2 && Fire(source) == 1);
    }

    // An instance that dies takes its member function out of the handler.
    {
        pxEventHandler<> e;
        {
            Listener listener(e);
            e.Invoke();
            PX_CHECK(listener.count == 1);
        }
        PX_CHECK(e.IsEmpty());
        e.Invoke();
    }

    // Unsubscribing from inside the callback: it runs once.
    {
        pxEventHandler<> e;
        pxSubscription once;
        once = e.Subscribe([&once] { ++hits; once.Unsubscribe(); });
        e += &Hit;
        PX_CHECK(Fire(e) == 2 && Fire(e) == 1 && e.Count() == 1);
    }
}
