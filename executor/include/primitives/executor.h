#pragma once

#include <primitives/templates.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <unordered_set>

using Task = std::function<void()>;

class Executor;

template <class Ret>
struct Future;

template <class T>
struct SharedState;

size_t get_max_threads(size_t N = 4);
Executor &getExecutor(size_t N = 0);

class TaskQueue
{
    using Tasks = std::deque<Task>;
    using Lock = std::unique_lock<std::mutex>;

public:
    TaskQueue() {}
    TaskQueue(TaskQueue &&rhs)
    {
        Lock lock(rhs.m);
        q = std::move(rhs.q);
    }

    bool try_push(Task &&t)
    {
        {
            Lock lock(m, std::try_to_lock);
            if (!lock)
                return false;
            q.emplace_back(std::move(t));
        }
        cv.notify_one();
        return true;
    }

    bool try_pop(Task &t)
    {
        Lock lock(m, std::try_to_lock);
        if (!lock || q.empty() || done_)
            return false;
        t = std::move(q.front());
        q.pop_front();
        return true;
    }

    void push(Task &&t)
    {
        {
            Lock lock(m);
            q.emplace_back(std::move(t));
        }
        cv.notify_one();
    }

    bool pop(Task &t)
    {
        Lock lock(m);
        while (q.empty() && !done_)
            cv.wait(lock);
        if (q.empty() || done_)
            return false;
        t = std::move(q.front());
        q.pop_front();
        return true;
    }

    void done()
    {
        {
            Lock lock(m);
            done_ = true;
        }
        cv.notify_all();
    }

    bool empty() const
    {
        return q.empty();
    }

private:
    Tasks q;
    std::mutex m;
    std::condition_variable cv;
    bool done_ = false;
};

template <class T>
using SharedStatePtr = std::shared_ptr<SharedState<T>>;

template <class T>
struct SharedState
{
    using FutureType = Future<T>;
    using VoidDataType = int;
    using DataType = std::conditional_t<std::is_same_v<T, void>, VoidDataType, T>;

    Executor &e;
    std::condition_variable cv;
    std::vector<std::condition_variable*> cv_notifiers;
    std::vector<Task> callbacks;
    std::mutex m;
    typename SharedStatePtr<T>::weak_type w;

    std::atomic_bool set{ false };
    DataType data;
    std::exception_ptr eptr;

    SharedState(Executor &e) : e(e) {}
    SharedState(const SharedState &rhs)
        : e(rhs.e), set(rhs.set), data(rhs.data), eptr(rhs.eptr)
    {
    }

    FutureType getFuture()
    {
        FutureType f(w.lock());
        return f;
    }

    void wait();

    bool setExecuted()
    {
        bool n = false;
        if (!set.compare_exchange_strong(n, true))
            return false;
        notice();
        return true;
    }

    void notice()
    {
        cv.notify_all();
        // lock after not locked own notify
        {
            std::unique_lock<std::mutex> lk(m);
            for (auto &cv : cv_notifiers)
                cv->notify_all();
            for (auto &cb : callbacks)
            {
                if (cb)
                    cb();
            }
        }
        cv_notifiers.clear();
        callbacks.clear();
    }
};

template <class T, class ... Args>
auto makeSharedState(Args && ... args)
{
    auto s = std::make_shared<SharedState<T>>(std::forward<Args>(args)...);
    s->w = s;
    return s;
}

template <class Ret>
struct Future
{
    using return_type = Ret;

    SharedStatePtr<Ret> state;

    Future(const SharedStatePtr<Ret> &s)
        : state(s)
    {
    }

    Future(const Future &f) = default;
    Future &operator=(const Future &f) = default;
    Future(Future &&rhs) = default;

    std::conditional_t<std::is_same_v<Ret, void>, void, Ret> get() const
    {
        state->wait();
        if (state->eptr)
            std::rethrow_exception(state->eptr);

        if constexpr (std::is_same_v<Ret, void>)
            return;
        else
            return std::move(state->data);
    }

    void wait() const
    {
        state->wait();
    }

    template <class F2, class ... ArgTypes2>
    Future<std::invoke_result_t<F2, ArgTypes2...>>
        then(F2 &&f, ArgTypes2 && ... args);
};

template <class F, class ... ArgTypes>
struct PackagedTask
{
    using Ret = std::invoke_result_t<F, ArgTypes...>;
    using Task = std::function<Ret(ArgTypes...)>;
    using FutureType = Future<Ret>;

    PackagedTask(Executor &e, F f, ArgTypes && ... args)
        : t(f)
    {
        s = makeSharedState<Ret>(e);
    }

    PackagedTask(const PackagedTask &rhs)
        : t(rhs.t), s(rhs.s)
    {
    }
    PackagedTask(PackagedTask &&rhs) = default;

    FutureType getFuture()
    {
        return s->getFuture();
    }

    void operator()(ArgTypes && ... args) const noexcept
    {
        try
        {
            if constexpr (std::is_same_v<Ret, void>)
                t(std::forward<ArgTypes>(args)...);
            else
                s->data = std::move(t(std::forward<ArgTypes>(args)...));
        }
        catch (...)
        {
            s->eptr = std::current_exception();
        }
        s->setExecuted();
    }

private:
    Task t;
    mutable SharedStatePtr<Ret> s;
};

class Executor
{
    struct Thread
    {
        std::thread t;
        TaskQueue q;
        volatile bool busy = false;
    };

    using Threads = std::vector<Thread>;

public:
    Executor(size_t nThreads = std::thread::hardware_concurrency(), const std::string &name = "");
    Executor(const std::string &name, size_t nThreads = std::thread::hardware_concurrency());
    ~Executor();

    size_t numberOfThreads() const { return nThreads; }

    template <class F, class ... ArgTypes>
    auto push(F &&f, ArgTypes && ... args)
    {
        static_assert(!std::is_lvalue_reference<F>::value, "Argument cannot be an lvalue");

        PackagedTask<F, ArgTypes...> pt(*this, f, std::forward<ArgTypes>(args)...);
        return push(pt);
    }

    template <class F, class ... ArgTypes>
    auto push(PackagedTask<F, ArgTypes...> pt)
    {
        static_assert(!std::is_lvalue_reference<F>::value, "Argument cannot be an lvalue");

        auto fut = pt.getFuture();
        Task task([pt]() { pt(); });
        push(std::move(task));
        return fut;
    }

    void join();
    void stop();
    bool stopped() const { return stopped_; }
    void wait();

    bool is_in_executor() const;
    bool try_run_one();

private:
    Threads thread_pool;
    size_t nThreads = std::thread::hardware_concurrency();
    std::atomic_size_t index{ 0 };
    std::atomic_bool stopped_{ false };
    std::unordered_map<std::thread::id, size_t> thread_ids;
    std::mutex m;

    void run(size_t i, const std::string &name);
    void run2(size_t i, const std::string &name);
    bool run_task();
    bool run_task(Task t);
    bool run_task(size_t i);
    void run_task(size_t i, Task t) noexcept;
    size_t get_n() const;
    Task get_task();
    Task get_task(size_t i);
    Task get_task_non_stealing();
    Task get_task_non_stealing(size_t i);
    void set_thread_name(const std::string &name) const;

    void push(Task &&t);
    Task try_pop();
    Task try_pop(size_t i);
    // add Task pop();
};

template <class T>
void SharedState<T>::wait()
{
    using namespace std::literals::chrono_literals;

    if (set)
        return;

    if (!e.is_in_executor())
    {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [this] { return set.load(); });
        return;
    }

    // continue executor business
    const auto max_delay = 1s;
    auto delay = 100ms;
    while (!set)
    {
        if (!e.try_run_one())
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait_for(lk, delay, [this] { return set.load(); });
            delay = delay > max_delay ? max_delay : (delay + 100ms);
        }
        else
            delay = 100ms;
    }
}

template <class Ret>
template <class F2, class ... ArgTypes2>
Future<std::invoke_result_t<F2, ArgTypes2...>>
Future<Ret>::then(F2 &&f, ArgTypes2 && ... args)
{
    if (state->set)
        return state->e.push(std::forward<F2>(f), std::forward<ArgTypes2>(args)...);

    std::unique_lock<std::mutex> lk(state->m);
    if (state->set)
        return state->e.push(std::forward<F2>(f), std::forward<ArgTypes2>(args)...);
    PackagedTask<F2, ArgTypes2...> pt(state->e, f, std::forward<ArgTypes2>(args)...);
    state->callbacks.push_back([this, pt]() { state->e.push(pt); });
    return pt.getFuture();
}

// vector versions
template <typename F>
Future<void> whenAll(const std::vector<F> &futures)
{
    using Ret = void;

    SharedStatePtr<Ret> s;
    if (futures.empty())
    {
        s = makeSharedState<Ret>(getExecutor());
        s->set = true;
    }
    else
        s = makeSharedState<Ret>(futures.begin()->state->e);

    // initial check
    if (std::all_of(futures.begin(), futures.end(),
        [](const auto &f) { return f.state->set.load(); }))
    {
        s->set = true;
        return s->getFuture();
    }

    // lock
    for (auto &f : futures)
        f.state->m.lock();

    auto unlock = [&futures]()
    {
        for (auto &f : futures)
            f.state->m.unlock();
    };

    // second check
    if (std::all_of(futures.begin(), futures.end(),
        [](auto &f) { return f.state->set.load(); }))
    {
        s->set = true;
        unlock();
        return s->getFuture();
    }

    // set cb
    auto ac = std::make_shared<std::atomic_size_t>();
    for (auto &f : futures)
    {
        f.state->callbacks.push_back([s, max = futures.size() - 1, ac]
        {
            if ((*ac)++ == max)
            s->setExecuted();
        });
    }

    unlock();

    return s->getFuture();
}

template <class F>
Future<size_t> whenAny(const std::vector<F> &futures)
{
    using Ret = size_t;

    SharedStatePtr<Ret> s;
    if (futures.empty())
    {
        s = makeSharedState<Ret>(getExecutor());
        s->set = true;
    }
    else
        s = makeSharedState<Ret>(futures.begin()->state->e);

    // initial check
    size_t i = 0;
    for (auto &f : futures)
    {
        if (f.state->set)
        {
            s->data = i;
            s->set = true;
            return s->getFuture();
        }
        i++;
    }

    // lock
    for (auto &f : futures)
        f.state->m.lock();

    auto unlock = [&futures]()
    {
        for (auto &f : futures)
            f.state->m.unlock();
    };

    // second check
    i = 0;
    for (auto &f : futures)
    {
        if (f.state->set)
        {
            s->data = i;
            s->set = true;
            unlock();
            return s->getFuture();
        }
        i++;
    }

    // set cb
    i = 0;
    for (auto &f : futures)
        f.state->callbacks.push_back([s, i = i++]{ if (s->setExecuted()) s->data = i; });

    unlock();

    return s->getFuture();
}

template <class F>
void waitAll(const std::vector<F> &futures)
{
    whenAll(futures).get();
}

template <class F>
void waitAny(const std::vector<F> &futures)
{
    whenAny(futures).get();
}

// variadic versions
template < bool... > struct all;
template < > struct all<> : std::true_type {};
template < bool B, bool... Rest > struct all<B, Rest...>
{
    constexpr static bool value = B && all<Rest...>::value;
};

template<class T> struct is_future : public std::false_type {};

template<class T>
struct is_future<Future<T>> : public std::true_type {};

template <
    class ... Futures,
    typename = std::enable_if_t<
    all< is_future< std::decay_t<Futures> >::value... >::value
    >
>
Future<void> whenAll(Futures && ... futures)
{
    using Ret = void;

    auto t = std::make_tuple(std::forward<Futures>(futures)...);
    constexpr auto sz = sizeof...(futures);

    SharedStatePtr<Ret> s;
    if (sizeof...(futures) == 0)
    {
        s = makeSharedState<Ret>(getExecutor());
        s->set = true;
    }
    else
        s = makeSharedState<Ret>(std::get<0>(t).state->e);

    // initial check
    bool set = true;
    for_each(t, [&set](const auto &f) { set &= f.state->set.load(); });
    if (set)
    {
        s->set = true;
        return s->getFuture();
    }

    // lock
    for_each(t, [](auto &f) { f.state->m.lock(); });

    auto unlock = [&t]()
    {
        for_each(t, [](auto &f) { f.state->m.unlock(); });
    };

    // second check
    set = true;
    for_each(t, [&set](const auto &f) { set &= f.state->set.load(); });
    if (set)
    {
        s->set = true;
        unlock();
        return s->getFuture();
    }

    // set cb
    auto ac = std::make_shared<std::atomic_size_t>();
    for_each(t, [&sz, &s, &ac](auto &f)
    {
        f.state->callbacks.push_back([s, max = sz - 1, ac]
        {
            if ((*ac)++ == max)
            s->setExecuted();
        });
    });

    unlock();

    return s->getFuture();
}

template <
    class ... Futures,
    typename = std::enable_if_t<
    all< is_future< std::decay_t<Futures> >::value... >::value
    >
>
Future<size_t> whenAny(Futures && ... futures)
{
    using Ret = size_t;

    auto t = std::make_tuple(std::forward<Futures>(futures)...);
    constexpr auto sz = sizeof...(futures);

    SharedStatePtr<Ret> s;
    if (sizeof...(futures) == 0)
    {
        s = makeSharedState<Ret>(getExecutor());
        s->set = true;
    }
    else
        s = makeSharedState<Ret>(std::get<0>(t).state->e);

    // initial check
    size_t i = 0;
    size_t n = -1;
    bool set = false;
    for_each(t, [&set, &i, &n](const auto &f)
    {
        set |= f.state->set.load();
        if (set && n == -1)
            n = i;
        i++;
    });
    if (set)
    {
        s->data = n;
        s->set = true;
        return s->getFuture();
    }

    // lock
    for_each(t, [](auto &f) { f.state->m.lock(); });

    auto unlock = [&t]()
    {
        for_each(t, [](auto &f) { f.state->m.unlock(); });
    };

    // second check
    i = 0;
    n = -1;
    set = false;
    for_each(t, [&set, &i, &n](const auto &f)
    {
        set |= f.state->set.load();
        if (set && n == -1)
            n = i;
        i++;
    });
    if (set)
    {
        s->data = n;
        s->set = true;
        unlock();
        return s->getFuture();
    }

    // set cb
    i = 0;
    for_each(t, [&s, &i](auto &f)
    {
        f.state->callbacks.push_back([s, i = i++]
        {
            if (s->setExecuted())
            s->data = i;
        });
    });

    unlock();

    return s->getFuture();
}

template <class ... Futures>
void waitAll(Futures && ... futures)
{
    whenAll(std::forward<Futures>(futures)...).get();
}

template <class ... Futures>
void waitAny(Futures && ... futures)
{
    whenAny(std::forward<Futures>(futures)...).get();
}

template <class F>
void waitAndGet(const std::vector<F> &futures)
{
    for (auto &f : futures)
        f.wait();
    for (auto &f : futures)
        f.get();
}

template <class F>
std::vector<std::exception_ptr> waitAndGetAllExceptions(const std::vector<F> &futures)
{
    for (auto &f : futures)
        f.wait();
    std::vector<std::exception_ptr> eptrs;
    for (auto &f : futures)
    {
        if (f.state->eptr)
            eptrs.push_back(f.state->eptr);
    }
    return eptrs;
}

#ifdef _WIN32
namespace primitives::executor
{

extern bool bExecutorUseSEH;

}
#endif
