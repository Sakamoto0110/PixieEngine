#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

template<typename... TArgs> class pxEventHandler;
template<typename TOwner, typename... TArgs> class pxEvent;

// How an argument reaches each callback: a value as a const reference, so every callback sees what
// the caller passed and none can change it for the next one; a reference as it is.
template<typename T>
using pxEventArg = std::conditional_t<std::is_reference_v<T>, T, const T&>;

namespace pxEventsDetail {

    // Callables up to this size live inside the pxCallback; bigger ones go to the heap.
    inline constexpr std::size_t InlineSize = 4 * sizeof(void*);
    inline constexpr std::size_t InlineAlign = alignof(std::max_align_t);

    template<typename F>
    inline constexpr bool StoredInline =
        sizeof(F) <= InlineSize && alignof(F) <= InlineAlign && std::is_nothrow_move_constructible_v<F>;

    struct alignas(InlineAlign) Storage {
        unsigned char bytes[InlineSize];
    };

    // One address per type. It is not const, so no linker folds two of them into one, as the
    // identical COMDAT folding of MSVC (/OPT:ICF) may do with equal read-only data.
    template<typename F>
    inline char TypeTag = 0;

    template<typename T> inline constexpr bool IsEvent = false;
    template<typename... Tx> inline constexpr bool IsEvent<pxEventHandler<Tx...>> = true;
    template<typename O, typename... Tx> inline constexpr bool IsEvent<pxEvent<O, Tx...>> = true;

    // Two callables of the same type are the same callback when they hold no state or compare
    // equal. A lambda with captures is only the same as the copies of its own pxCallback (origin).
    template<typename F>
    bool SameCallable(const F& a, const F& b) {
        if constexpr (std::is_empty_v<F>)
            return true;
        else if constexpr (std::equality_comparable<F>)
            return static_cast<bool>(a == b);
        else
            return false;
    }

    // An instance bound to one of its member functions, what a C# delegate to a method holds.
    template<typename T, typename TMember>
    struct MemberBinding {
        T* instance;
        TMember member;

        template<typename... Tx>
        decltype(auto) operator()(Tx&&... args) const {
            return std::invoke(member, instance, std::forward<Tx>(args)...);
        }

        // The standard leaves the comparison of pointers to virtual members unspecified; GCC,
        // Clang and MSVC compare them as expected.
        bool operator==(const MemberBinding&) const = default;
    };

    inline std::uint64_t NewOrigin() noexcept {
        static std::atomic<std::uint64_t> next{ 1 };
        return next.fetch_add(1, std::memory_order_relaxed);
    }
}

// One callable kept by value, what a C# delegate with a single target is. Callables up to four
// pointers in size live inside it, with no allocation; bigger ones live on the heap, owned by it.
// A copy copies the callable, so it never depends on the original.
//
// Ported from pxEvents.hpp of 2023 (indev, b961405), where the callback held a pointer to a
// parameter that died with its constructor.
template<typename... TArgs>
class pxCallback {
    static_assert((!std::is_rvalue_reference_v<TArgs> && ...),
        "pxCallback: every callback receives the same arguments, so none can be an rvalue reference");

    using Storage = pxEventsDetail::Storage;

    struct VTable {
        const void* type;
        void (*invoke)(Storage&, pxEventArg<TArgs>...);
        void (*copy)(const Storage& from, Storage& to);
        void (*move)(Storage& from, Storage& to) noexcept; // leaves no object in `from`
        void (*destroy)(Storage&) noexcept;
        bool (*equals)(const Storage&, const Storage&);
    };

    template<typename F, bool Inline = pxEventsDetail::StoredInline<F>>
    struct Ops;

    template<typename F>
    struct Ops<F, true> {
        static F& Get(Storage& s) noexcept { return *std::launder(reinterpret_cast<F*>(s.bytes)); }
        static const F& Get(const Storage& s) noexcept { return *std::launder(reinterpret_cast<const F*>(s.bytes)); }

        template<typename U>
        static void Create(Storage& s, U&& f) { ::new (static_cast<void*>(s.bytes)) F(std::forward<U>(f)); }

        static void Invoke(Storage& s, pxEventArg<TArgs>... args) { static_cast<void>(std::invoke(Get(s), args...)); }
        static void Copy(const Storage& from, Storage& to) { Create(to, Get(from)); }
        static void Move(Storage& from, Storage& to) noexcept { Create(to, std::move(Get(from))); Get(from).~F(); }
        static void Destroy(Storage& s) noexcept { Get(s).~F(); }
        static bool Equals(const Storage& a, const Storage& b) { return pxEventsDetail::SameCallable(Get(a), Get(b)); }

        static constexpr VTable Table{ &pxEventsDetail::TypeTag<F>, &Invoke, &Copy, &Move, &Destroy, &Equals };
    };

    template<typename F>
    struct Ops<F, false> {
        static F*& Ptr(Storage& s) noexcept { return *std::launder(reinterpret_cast<F**>(s.bytes)); }
        static F* Ptr(const Storage& s) noexcept { return *std::launder(reinterpret_cast<F* const*>(s.bytes)); }

        template<typename U>
        static void Create(Storage& s, U&& f) {
            F* p = new F(std::forward<U>(f));
            ::new (static_cast<void*>(s.bytes)) F*(p);
        }

        static void Invoke(Storage& s, pxEventArg<TArgs>... args) { static_cast<void>(std::invoke(*Ptr(s), args...)); }
        static void Copy(const Storage& from, Storage& to) { Create(to, *Ptr(from)); }
        static void Move(Storage& from, Storage& to) noexcept { ::new (static_cast<void*>(to.bytes)) F*(Ptr(from)); }
        static void Destroy(Storage& s) noexcept { delete Ptr(s); }
        static bool Equals(const Storage& a, const Storage& b) { return pxEventsDetail::SameCallable(*Ptr(a), *Ptr(b)); }

        static constexpr VTable Table{ &pxEventsDetail::TypeTag<F>, &Invoke, &Copy, &Move, &Destroy, &Equals };
    };

public:
    pxCallback() noexcept = default;

    // Any callable that takes the arguments: a function, a lambda, a functor. A null pointer to
    // function gives an empty pxCallback.
    template<typename F>
        requires (!std::is_same_v<std::remove_cvref_t<F>, pxCallback> && !pxEventsDetail::IsEvent<std::remove_cvref_t<F>>
            && std::is_invocable_v<std::decay_t<F>&, pxEventArg<TArgs>...>)
    pxCallback(F&& f) {
        using D = std::decay_t<F>;
        static_assert(std::is_copy_constructible_v<D>,
            "pxCallback: callables are copied, as C# delegates are, so they must be copyable");

        if constexpr (std::is_pointer_v<D> || std::is_member_pointer_v<D>) {
            D d(std::forward<F>(f));
            if (d == nullptr)
                return;
            Emplace<D>(d);
        } else {
            Emplace<D>(std::forward<F>(f));
        }
    }

    // A member function bound to an instance: e += { &button, &Button::OnClick }.
    template<typename T, typename TMember>
        requires (std::is_member_function_pointer_v<TMember>
            && std::is_invocable_v<TMember, T*, pxEventArg<TArgs>...>)
    pxCallback(T* instance, TMember member) {
        using D = pxEventsDetail::MemberBinding<T, TMember>;
        if (instance == nullptr || member == nullptr)
            return;
        Emplace<D>(D{ instance, member });
    }

    pxCallback(const pxCallback& o) : m_origin(o.m_origin) {
        if (o.m_vt != nullptr) {
            o.m_vt->copy(o.m_storage, m_storage);
            m_vt = o.m_vt;
        }
    }

    pxCallback(pxCallback&& o) noexcept : m_origin(o.m_origin) { TakeFrom(o); }

    pxCallback& operator=(const pxCallback& o) {
        if (this != &o) {
            pxCallback copy(o);
            *this = std::move(copy);
        }
        return *this;
    }

    pxCallback& operator=(pxCallback&& o) noexcept {
        if (this != &o) {
            Reset();
            m_origin = o.m_origin;
            TakeFrom(o);
        }
        return *this;
    }

    ~pxCallback() { Reset(); }

    // Calls the callable; an empty pxCallback does nothing.
    void operator()(TArgs... args) {
        if (m_vt != nullptr)
            m_vt->invoke(m_storage, args...);
    }

    explicit operator bool() const noexcept { return m_vt != nullptr; }

    // The identity of a C# delegate: the same function, the same instance and member, the same
    // lambda without captures, or a copy of the same pxCallback.
    friend bool operator==(const pxCallback& a, const pxCallback& b) {
        if (a.m_vt == nullptr || b.m_vt == nullptr)
            return a.m_vt == b.m_vt;
        if (a.m_vt->type != b.m_vt->type)
            return false;
        return a.m_origin == b.m_origin || a.m_vt->equals(a.m_storage, b.m_storage);
    }

private:
    template<typename...> friend class pxEventHandler;

    template<typename D, typename U>
    void Emplace(U&& f) {
        Ops<D>::Create(m_storage, std::forward<U>(f));
        m_vt = &Ops<D>::Table;
        m_origin = pxEventsDetail::NewOrigin();
    }

    void TakeFrom(pxCallback& o) noexcept {
        if (o.m_vt != nullptr) {
            o.m_vt->move(o.m_storage, m_storage);
            m_vt = std::exchange(o.m_vt, nullptr);
        }
    }

    void Reset() noexcept {
        if (m_vt != nullptr)
            std::exchange(m_vt, nullptr)->destroy(m_storage);
    }

    // What the handler calls: the arguments go through as references, with no copy per callback.
    void Call(pxEventArg<TArgs>... args) { m_vt->invoke(m_storage, args...); }

    // The callable, when it is an F: how the handler finds the ones it stored itself.
    template<typename F>
    F* Target() noexcept {
        if (m_vt == nullptr || m_vt->type != &pxEventsDetail::TypeTag<F>)
            return nullptr;
        if constexpr (pxEventsDetail::StoredInline<F>)
            return &Ops<F>::Get(m_storage);
        else
            return Ops<F>::Ptr(m_storage);
    }

    template<typename F>
    const F* Target() const noexcept { return const_cast<pxCallback*>(this)->template Target<F>(); }

    const VTable* m_vt = nullptr;
    std::uint64_t m_origin = 0;
    Storage m_storage;
};
