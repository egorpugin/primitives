#pragma once

#include <mutex>
#include <utility>

// scope guard
#define CONCATENATE_IMPL(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_IMPL(s1, s2)

#define ANONYMOUS_VARIABLE_COUNTER(s) CONCATENATE(s, __COUNTER__)
#define ANONYMOUS_VARIABLE_LINE(s) CONCATENATE(s, __LINE__)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(s) ANONYMOUS_VARIABLE_COUNTER(s)
#else
#define ANONYMOUS_VARIABLE(s) ANONYMOUS_VARIABLE_LINE(s)
#endif

#define SCOPE_EXIT \
    auto ANONYMOUS_VARIABLE(SCOPE_EXIT_STATE) = ::detail::ScopeGuardOnExit() + [&]()

#define RUN_ONCE_IMPL(kv) \
    kv std::once_flag ANONYMOUS_VARIABLE_LINE(RUN_ONCE_FLAG); ScopeGuard(&ANONYMOUS_VARIABLE_LINE(RUN_ONCE_FLAG)) + [&]()

#define RUN_ONCE              RUN_ONCE_IMPL(static)
#define RUN_ONCE_THREAD_LOCAL RUN_ONCE_IMPL(thread_local)

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
        run();
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
