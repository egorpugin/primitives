#pragma once

#include "preprocessor.h"

#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#if !defined(__APPLE__)
#include <ranges>
#endif
#include <tuple>
#include <utility>

////////////////////////////////////////////////////////////////////////////////

#define ANONYMOUS_VARIABLE_COUNTER(s) CONCATENATE(CONCATENATE(s, _), __COUNTER__)
#define ANONYMOUS_VARIABLE_LINE(s) CONCATENATE(CONCATENATE(s, _), __LINE__)

#ifdef __COUNTER__
#define PRIMITIVES_ANONYMOUS_VARIABLE(s) ANONYMOUS_VARIABLE_COUNTER(s)
#else
#define PRIMITIVES_ANONYMOUS_VARIABLE(s) ANONYMOUS_VARIABLE_LINE(s)
#endif

////////////////////////////////////////////////////////////////////////////////

#define RUN_ONCE_IMPL(kv) \
    for (kv std::atomic<int> ANONYMOUS_VARIABLE_LINE(RUN_ONCE_FLAG){0}; !ANONYMOUS_VARIABLE_LINE(RUN_ONCE_FLAG).fetch_or(1); )

#define RUN_ONCE              RUN_ONCE_IMPL(static)
#define RUN_ONCE_THREAD_LOCAL RUN_ONCE_IMPL(thread_local)
#define RUN_ONCE_FOR_THREAD   RUN_ONCE_THREAD_LOCAL

////////////////////////////////////////////////////////////////////////////////

#define SCOPE_EXIT \
    auto PRIMITIVES_ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = SCOPE_EXIT_NAMED

#define SCOPE_EXIT_NAMED \
    ::detail::ScopeGuardOnExit() + [&]()

class ScopeGuard
{
    using Function = std::function<void()>;

public:
    ScopeGuard(Function f)
        : f(std::move(f))
    {}
    ScopeGuard(std::once_flag *flag)
        : flag(flag)
    {}
    ~ScopeGuard()
    {
        if (!active)
            return;

        // handle not in run, but here in dtor
        try
        {
            run();
        }
        catch (const std::exception &e)
        {
            std::cerr << "exception was thrown in scope guard: " << e.what() << std::endl;
            //throw;
        }
        catch (...)
        {
            std::cerr << "unknown exception was thrown in scope guard" << std::endl;
            //throw;
        }

        // in case if we want to run dtor manually
        // it won't shoot for the second time
        dismiss();
    }

    void dismiss()
    {
        active = false;
    }

    template <typename F>
    ScopeGuard &operator+(F &&fn)
    {
        f = std::move(fn);
        return *this;
    }

public:
    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;

    ScopeGuard(ScopeGuard &&rhs)
        : f(std::move(rhs.f)), active(rhs.active)
    {
        rhs.dismiss();
    }
    ScopeGuard &operator=(ScopeGuard &&) = delete;

private:
    Function f;
    bool active{ true };
    std::once_flag *flag{ nullptr };

    void run()
    {
        if (!f)
            return;

        if (flag)
            std::call_once(*flag, f);
        else
            f();
    }
};

namespace detail
{
    enum class ScopeGuardOnExit {};

    template <typename F>
    ScopeGuard operator+(ScopeGuardOnExit, F &&fn)
    {
        return ScopeGuard(std::forward<F>(fn));
    }
}

namespace primitives
{

struct ExtendedScopeExit
{
    using F = std::function<void(void)>;

    F on_error;
    F on_success;
    F finally;

    ExtendedScopeExit(F init = {})
    {
        n_exceptions = std::uncaught_exceptions();
        if (init)
            init();
    }
    ExtendedScopeExit(ExtendedScopeExit &&rhs)
    {
        n_exceptions = rhs.n_exceptions;
        on_error = std::move(rhs.on_error);
        on_success = std::move(rhs.on_success);
        finally = std::move(rhs.finally);
    }
    ExtendedScopeExit(const ExtendedScopeExit &rhs) = delete;
    ~ExtendedScopeExit()
    {
        if (std::uncaught_exceptions() > n_exceptions)
        {
            if (on_error)
                on_error();
        }
        else
        {
            if (on_success)
                on_success();
        }
        if (finally)
            finally();
    }

private:
    int n_exceptions;
};

}

////////////////////////////////////////////////////////////////////////////////

// tuple for_each
template <typename Tuple, typename F, std::size_t ...Indices>
constexpr void for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>)
{
    using swallow = int[];
    (void)swallow {
        1,
            (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...
    };
}

template <typename Tuple, typename F>
constexpr void for_each(Tuple&& tuple, F&& f)
{
    constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
    for_each_impl(std::forward<Tuple>(tuple), std::forward<F>(f),
        std::make_index_sequence<N>{});
}

////////////////////////////////////////////////////////////////////////////////

/// Initialises static object from outside or creates it itself.
template <class T>
struct StaticValueOrRef
{
    StaticValueOrRef(T *p)
    {
        if (p)
            ptr = p;
        else
        {
            static T t;
            ptr = &t;
        }
    }

    operator T&() { return *ptr; }

private:
    T *ptr;
};

////////////////////////////////////////////////////////////////////////////////

// from llvm.demangle
template <class T>
class SwapAndRestore
{
    T &Restore;
    T OriginalValue;
    bool ShouldRestore = true;

public:
    SwapAndRestore(T &Restore_)
        : SwapAndRestore(Restore_, Restore_)
    {
    }

    template <class U>
    SwapAndRestore(T &Restore_, U NewVal)
        : Restore(Restore_), OriginalValue(Restore)
    {
        Restore = std::move(NewVal);
    }
    ~SwapAndRestore()
    {
        if (ShouldRestore)
            Restore = std::move(OriginalValue);
    }

    void shouldRestore(bool ShouldRestore_)
    {
        ShouldRestore = ShouldRestore_;
    }

    void restoreNow(bool Force)
    {
        if (!Force && !ShouldRestore)
            return;

        Restore = std::move(OriginalValue);
        ShouldRestore = false;
    }

    SwapAndRestore(const SwapAndRestore &) = delete;
    SwapAndRestore &operator=(const SwapAndRestore &) = delete;
};

////////////////////////////////////////////////////////////////////////////////

// from http://www.reedbeta.com/blog/python-like-enumerate-in-cpp17/
template <typename T,
    typename TIter = decltype(std::begin(std::declval<T>())),
    typename = decltype(std::end(std::declval<T>()))>
    constexpr auto enumerate(T && iterable)
{
    struct iterator
    {
        size_t i;
        TIter iter;
        bool operator != (const iterator & other) const { return iter != other.iter; }
        void operator ++ () { ++i; ++iter; }
        auto operator * () const { return std::tie(i, *iter); }
    };
    struct iterable_wrapper
    {
        T iterable;
        auto begin() { return iterator{ 0, std::begin(iterable) }; }
        auto end() { return iterator{ 0, std::end(iterable) }; }
    };
    return iterable_wrapper{ std::forward<T>(iterable) };
}

////////////////////////////////////////////////////////////////////////////////

// globals

/*#define SW_DECLARE_GLOBAL_STATIC_FUNCTION(t, fn) \
    t &fn(t *in = nullptr)

#define SW_DEFINE_GLOBAL_STATIC_FUNCTION(t, fn)                \
    t &fn(t *in)                                               \
    {                                                          \
        static t *g = nullptr;                                 \
        if (in)                                                \
            g = in;                                            \
        if (g == nullptr)                                      \
            throw SW_RUNTIME_ERROR("missing initializer"); \
        return *g;                                             \
    }

#define SW_DECLARE_GLOBAL_STATIC_FUNCTION2(t, n, ns) \
    namespace ns { inline t *n; }
#define SW_DEFINE_GLOBAL_STATIC_FUNCTION2(t, fn, g)                \
    inline t &fn(t *in = nullptr)                                               \
    {                                                          \
        if (in)                                                \
            g = in;                                            \
        if (g == nullptr)                                      \
            throw SW_RUNTIME_ERROR("missing initializer"); \
        return *g;                                             \
    }*/

////////////////////////////////////////////////////////////////////////////////

namespace primitives::templates {

#if !defined(__APPLE__)

namespace detail {

// Type acts as a tag to find the correct operator| overload
template <typename C>
struct to_helper {};

// This actually does the work
template <typename Container, std::ranges::range R>
requires std::convertible_to<std::ranges::range_value_t<R>, typename Container::value_type>
Container operator|(R &&r, to_helper<Container>) {
    return Container{ r.begin(), r.end() };
    //return Container(std::begin(r), std::end(r));
}

}

// Couldn't find an concept for container, however a
// container is a range, but not a view.
template <std::ranges::range Container>
    requires (!std::ranges::view<Container>)
auto to() {
    return detail::to_helper<Container>{};
}

#endif

////////////////////////////////////////////////////////////////////////////////

#ifndef FWD
#define FWD(x) std::forward<decltype(x)>(x)
#endif

#define LIFT(foo)                                                                                                      \
    [](auto &&...x) noexcept(                                                                                          \
        noexcept(foo(std::forward<decltype(x)>(x)...))) -> decltype(foo(std::forward<decltype(x)>(x)...)) {            \
        return foo(std::forward<decltype(x)>(x)...); }

} // namespace primitives::templates
