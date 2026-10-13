#include <array>
#include <string>
#include <type_traits>

#include <pixie/events/pxCallback.hpp>

#include "check.hpp"

namespace {
    int lastValue = 0;
    void Store(int v) { lastValue = v; }
    void StoreTwice(int v) { lastValue = 2 * v; }

    struct Counter {
        int total = 0;
        void Add(int v) { total += v; }
        void Sub(int v) { total -= v; }
        int Peek(int) const { return total; }
    };

    // Counts the copies alive, to see that every copy made is destroyed.
    struct Tracked {
        static inline int alive = 0;
        int* sum;
        Tracked(int* s) : sum(s) { ++alive; }
        Tracked(const Tracked& o) : sum(o.sum) { ++alive; }
        Tracked(Tracked&& o) noexcept : sum(o.sum) { ++alive; }
        ~Tracked() { --alive; }
        void operator()(int v) const { *sum += v; }
    };

    // Too big for the inline buffer, so it lives on the heap.
    struct Big {
        std::array<char, 128> bytes{};
        Tracked tracked;
        void operator()(int v) const { tracked(v + bytes[0]); }
    };

    pxCallback<int> CaptureLocal() {
        int local = 40;
        return [local](int v) { lastValue = local + v; };
    }
}

void TestCallback() {
    // Empty: calling does nothing.
    pxCallback<int> empty;
    PX_CHECK(!empty);
    empty(1);
    PX_CHECK(empty == pxCallback<int>());
    void (*nullFn)(int) = nullptr;
    PX_CHECK(!pxCallback<int>(nullFn));

    // Functions: the same function is the same callback.
    pxCallback<int> store = &Store;
    store(7);
    PX_CHECK(lastValue == 7);
    PX_CHECK(store == pxCallback<int>(Store));
    PX_CHECK(store != pxCallback<int>(&StoreTwice));
    PX_CHECK(store != empty);

    // Member functions: the same instance and the same member.
    Counter a, b;
    pxCallback<int> addA{ &a, &Counter::Add };
    addA(5);
    PX_CHECK(a.total == 5);
    PX_CHECK(addA == pxCallback<int>(&a, &Counter::Add));
    PX_CHECK(addA != pxCallback<int>(&b, &Counter::Add));
    PX_CHECK(addA != pxCallback<int>(&a, &Counter::Sub));
    const Counter& constA = a;
    pxCallback<int> peek{ &constA, &Counter::Peek };
    peek(0);
    PX_CHECK(!pxCallback<int>(static_cast<Counter*>(nullptr), &Counter::Add));

    // A lambda is kept by value: the local it captured is gone, the copy is not (the 2023 bug).
    pxCallback<int> captured = CaptureLocal();
    captured(2);
    PX_CHECK(lastValue == 42);

    // A lambda without captures is the same as itself; two different lambdas are not.
    auto stateless = [](int v) { lastValue = v; };
    PX_CHECK(pxCallback<int>(stateless) == pxCallback<int>(stateless));
    PX_CHECK(pxCallback<int>(stateless) != pxCallback<int>([](int v) { lastValue = v; }));

    // A lambda with captures is the same only as the copies of its own pxCallback, as a C#
    // delegate is the same only as itself.
    int sum = 0;
    auto adder = [&sum](int v) { sum += v; };
    pxCallback<int> c1 = adder;
    pxCallback<int> c2 = c1;
    PX_CHECK(c1 == c2);
    PX_CHECK(c1 != pxCallback<int>(adder));

    // The state of a mutable lambda stays in its pxCallback; a copy has its own.
    int seen = 0;
    pxCallback<int> counting = [n = 0, &seen](int) mutable { seen = ++n; };
    counting(0);
    counting(0);
    PX_CHECK(seen == 2);
    pxCallback<int> counting2 = counting;
    counting2(0);
    PX_CHECK(seen == 3);
    counting(0);
    PX_CHECK(seen == 3);

    // Every copy is destroyed, inline and on the heap; a move leaves the source empty.
    {
        int total = 0;
        pxCallback<int> small = Tracked(&total);
        pxCallback<int> big = Big{ {}, Tracked(&total) };
        pxCallback<int> bigCopy = big;
        pxCallback<int> smallCopy = small;
        PX_CHECK(Tracked::alive == 4);
        PX_CHECK(big == bigCopy && small == smallCopy && big != small);

        pxCallback<int> moved = std::move(big);
        PX_CHECK(!big && moved == bigCopy);
        moved(1);
        bigCopy(1);
        small(1);
        PX_CHECK(total == 3);

        small = bigCopy;
        PX_CHECK(small == bigCopy && Tracked::alive == 4);
        pxCallback<int>& smallAlias = small;
        pxCallback<int>& movedAlias = moved;
        small = smallAlias;
        moved = std::move(movedAlias);
        PX_CHECK(small && moved);
        smallCopy = pxCallback<int>();
        PX_CHECK(Tracked::alive == 3);
    }
    PX_CHECK(Tracked::alive == 0);

    // A value reaches the callback as a const reference, so a callback that could change it for
    // the next one is refused; one that takes a reference gets the caller's object.
    using Text = pxCallback<std::string>;
    static_assert(std::is_constructible_v<Text, void (*)(std::string)>);
    static_assert(std::is_constructible_v<Text, void (*)(const std::string&)>);
    static_assert(!std::is_constructible_v<Text, void (*)(std::string&)>);
    static_assert(std::is_constructible_v<pxCallback<std::string&>, void (*)(std::string&)>);
    static_assert(!std::is_constructible_v<Text, void (*)(int)>);
    static_assert(!std::is_constructible_v<pxCallback<int>, const Counter*, void (Counter::*)(int)>);
}
