// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/templates.h>
#include <primitives/debug.h>
#include <primitives/exceptions.h>
#include <primitives/thread.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <tuple>
#include <thread>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#ifdef _WIN32
#include <windows.h>
#endif

//#include "log.h"
//DECLARE_STATIC_LOGGER(logger, "executor");

//
using Task = std::function<void()>;

struct Executor;

template <class Ret>
struct Future;

template <class T>
struct SharedState;

inline size_t get_max_threads(int N = 4)
{
    return std::max<size_t>(N, std::thread::hardware_concurrency());
}

inline size_t select_number_of_threads(int N = 0)
{
    if (N > 0)
        return N;
    N = std::thread::hardware_concurrency();
    /*if (N == 1)
        N = 2; // probably bad too
    else if (N <= 8)
        N += 2; // bad on low memory configs
    else if (N <= 64) (8; 64] - make (32; 64]?
        N += 4;
    else
        N += 8; // too many extra jobs?
    */
    return N;
}

//SW_DECLARE_GLOBAL_STATIC_FUNCTION2(Executor, default_executor_ptr, primitives::executor)
//SW_DEFINE_GLOBAL_STATIC_FUNCTION2(Executor, getExecutor, primitives::executor::default_executor_ptr)

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
        if (done_)
            return false;
        {
            Lock lock(m, std::try_to_lock);
            if (!lock)
                return false;
            q.emplace_back(std::move(t));
        }
        cv.notify_one();
        return true;
    }

    bool try_pop(Task &t, std::atomic_bool &busy)
    {
        Lock lock(m, std::try_to_lock);
        if (!lock || q.empty() || done_)
            return false;
        t = std::move(q.front());
        busy = true;
        q.pop_front();
        return true;
    }

    void push(Task &&t)
    {
        if (done_)
            return;
        {
            Lock lock(m);
            q.emplace_back(std::move(t));
        }
        cv.notify_one();
    }

    bool pop(Task &t, std::atomic_bool &busy)
    {
        Lock lock(m);
        while (q.empty() && !done_)
            cv.wait(lock);
        if (q.empty() || done_)
            return false;
        t = std::move(q.front());
        busy = true;
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

template <class T>
using Futures = std::vector<Future<T>>;

template <class F, class ... ArgTypes>
struct PackagedTask
{
    using Ret = typename std::invoke_result_t<F, ArgTypes...>;
    using Task = std::function<Ret(ArgTypes...)>;
    using FutureType = Future<Ret>;

    PackagedTask(Executor &e, F f, ArgTypes && ...)
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

enum class WaitStatus
{
    Running,
    AllowIncoming,
    BlockIncoming,
    RejectIncoming,
};

struct Executor
{
    struct Thread
    {
        std::thread t;
        TaskQueue q;
        std::atomic_bool busy{ false };

        Thread() = default;
        Thread(const Thread &) {}

        bool empty() const { return q.empty(); }
    };

    using Threads = std::vector<Thread>;

public:
    Executor(size_t nThreads = std::thread::hardware_concurrency(), const std::string &name = "")
        : nThreads(nThreads)
    {
        // we keep this lock until all threads created and assigned to thread_pool var
        // this is to prevent races on data objects
        std::unique_lock<std::mutex> lk(m);

        std::atomic<decltype(nThreads)> barrier{ 0 };
        std::atomic<decltype(nThreads)> barrier2{ 0 };

        thread_pool.resize(nThreads);
        for (size_t i = 0; i < nThreads; i++)
        {
            thread_pool[i].t = make_thread([this, i, name = name, &barrier, &barrier2, nThreads]() mutable
            {
                // set tids early
                {
                    std::unique_lock<std::mutex> lk(m);
                    thread_ids[std::this_thread::get_id()] = i;
                    ++barrier;
                }

                // wait when all threads set their tids
                while (barrier != nThreads)
                    std::this_thread::sleep_for(std::chrono::microseconds(1));

                // allow to proceed main thread
                ++barrier2;

                // proceed
                run(i, name);
            });
        }

        // allow threads to run
        lk.unlock();

        // wait when all threads set their tids
        while (barrier2 != nThreads)
            std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    Executor(const std::string &name, size_t nThreads = std::thread::hardware_concurrency())
        : Executor(nThreads, name)
    {
    }
    ~Executor()
    {
        join();
    }

    size_t numberOfThreads() const { return nThreads; }

    template <class F, class ... ArgTypes>
    auto push(F &&f, ArgTypes && ... args)
    {
        static_assert(!std::is_lvalue_reference<F>::value, "Argument cannot be an lvalue");

        PackagedTask<F, ArgTypes...> pt(*this, f, std::forward<ArgTypes>(args)...);
        return push(pt);
    }

    template <class F>
    auto push(F &&f, size_t n_jobs)
    {
        static_assert(!std::is_lvalue_reference<F>::value, "Argument cannot be an lvalue");

        PackagedTask<F> pt(*this, f);
        auto fut = pt.getFuture();
        Task task([pt]() { pt(); });
        if (n_jobs == 1)
            task();
        else
            push(std::move(task));
        return fut;
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

    // wait & terminate all workers
    void join()
    {
        wait();
        stop();
        for (auto &t : thread_pool)
        {
            if (t.t.joinable())
                t.t.join();
        }
    }
    void stop()
    {
        stopped_ = true;
        for (auto &t : thread_pool)
            t.q.done();
    }
    void wait(WaitStatus p = WaitStatus::BlockIncoming)
    {
        std::unique_lock<std::mutex> lk(m_wait, std::try_to_lock);
        if (!lk.owns_lock())
        {
            std::unique_lock<std::mutex> lk(m_wait);
            return;
        }

        waiting_ = p;

        bool run_again = is_in_executor();

        // wait for empty queues
        for (auto &t : thread_pool)
        {
            while (!t.q.empty() && !stopped_)
            {
                if (run_again)
                {
                    // finish everything
                    for (auto &t : thread_pool)
                    {
                        Task ta;
                        if (t.q.try_pop(ta, thread_pool[get_n()].busy))
                            run_task(get_n(), ta);
                    }
                }
                else
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // free self
        if (run_again)
            thread_pool[get_n()].busy = false;

        // wait for end of execution
        for (auto &t : thread_pool)
        {
            while (t.busy && !stopped_)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        waiting_ = WaitStatus::Running;
        cv_wait.notify_all();
    }
    bool stopped() const { return stopped_; }

    bool is_in_executor() const
    {
        return thread_ids.find(std::this_thread::get_id()) != thread_ids.end();
    }
    bool try_run_one()
    {
        auto t = try_pop();
        if (t)
            run_task(t);
        return !!t;
    }
    bool empty() const
    {
        return std::all_of(thread_pool.begin(), thread_pool.end(), [](const auto &t)
        {
            return t.empty();
        });
    }

private:
    Threads thread_pool;
    size_t nThreads = std::thread::hardware_concurrency();
    std::atomic_size_t index{ 0 };
    std::atomic_bool stopped_{ false };
    std::mutex m_wait;
    std::condition_variable cv_wait;
    std::atomic<WaitStatus> waiting_{ WaitStatus::Running };
    std::unordered_map<std::thread::id, size_t> thread_ids;
    std::mutex m;

    void run(size_t i, const std::string &name)
    {
        auto n = name;
        if (!n.empty())
            n += " ";
        n += std::to_string(i);
        primitives::setThreadName(n);

        while (!stopped_)
        {
            if (!run_task())
                break;
        }
    }
    bool run_task()
    {
        return run_task(get_n());
    }
    bool run_task(Task &t)
    {
        run_task(get_n(), t);
        return true;
    }
    bool run_task(size_t i)
    {
        auto t = get_task(i);

        // double check
        if (stopped_)
            return false;

        run_task(i, t);
        return true;
    }
    void run_task(size_t i, Task &t) noexcept
    {
        primitives::ScopedThreadName stn(std::to_string(i) + " busy");

        auto &thr = thread_pool[i];
        t();
        thr.busy = false;
    }
    size_t get_n() const
    {
        return thread_ids.find(std::this_thread::get_id())->second;
    }
    Task get_task()
    {
        return get_task(get_n());
    }
    Task get_task(size_t i)
    {
        auto &thr = thread_pool[i];
        Task t = try_pop(i);
        // no task popped, probably shutdown command was issues
        if (!t && !thr.q.pop(t, thr.busy))
            return Task();

        return t;
    }
    Task get_task_non_stealing()
    {
        return get_task_non_stealing(get_n());
    }
    Task get_task_non_stealing(size_t i)
    {
        auto &thr = thread_pool[i];
        Task t;
        if (thr.q.pop(t, thr.busy))
            return t;
        return Task();
    }

    void push(Task &&t)
    {
        if (waiting_ == WaitStatus::RejectIncoming)
            throw SW_RUNTIME_ERROR("Executor is in the wait state and rejects new jobs");
        if (waiting_ == WaitStatus::BlockIncoming)
        {
            std::unique_lock<std::mutex> lk(m_wait);
            cv_wait.wait(lk, [this] { return waiting_ == WaitStatus::Running; });
        }

        auto i = index++;
        for (size_t n = 0; n != nThreads; ++n)
        {
            if (thread_pool[(i + n) % nThreads].q.try_push(std::move(t)))
                return;
        }
        thread_pool[i % nThreads].q.push(std::move(t));
    }
    Task try_pop()
    {
        return try_pop(get_n());
    }
    Task try_pop(size_t i)
    {
        Task t;
        const size_t spin_count = nThreads * 4;
        for (auto n = 0; n != spin_count; ++n)
        {
            if (thread_pool[(i + n) % nThreads].q.try_pop(t, thread_pool[i].busy))
                break;
        }
        return t;
    }
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
        // there is a deadlock somewhere related to this
        // so we workaround with wait_for()
        const auto max_delay = 1s;
        auto delay = 100ms;
        while (!set && !e.stopped())
        {
            std::unique_lock<std::mutex> lk(m);
            cv.wait_for(lk, delay, [this] { return set.load(); });
            delay = delay > max_delay ? max_delay : (delay + 100ms);
        }
        return;
    }

    // continue executor business
    const auto max_delay = 1s;
    auto delay = 100ms;
    while (!set && !e.stopped())
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
Future<void> whenAll(auto &&executor, const Futures<F> &futures)
{
    using Ret = void;

    SharedStatePtr<Ret> s;
    if (futures.empty())
    {
        s = makeSharedState<Ret>(executor);
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

/// return a future with an index of the finished future
template <class F>
Future<size_t> whenAny(auto &&executor, const Futures<F> &futures)
{
    using Ret = size_t;

    SharedStatePtr<Ret> s;
    if (futures.empty())
    {
        s = makeSharedState<Ret>(executor);
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
void waitAll(const Futures<F> &futures)
{
    whenAll(futures).get();
}

template <class F>
void waitAny(const Futures<F> &futures)
{
    whenAny(futures).get();
}

template <class F>
void waitAndGet(const Futures<F> &futures)
{
    for (auto &f : futures)
        f.wait();
    for (auto &f : futures)
        f.get();
}

template <class F>
std::vector<std::exception_ptr> waitAndGetAllExceptions(const Futures<F> &futures)
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
Future<void> whenAll(auto &&executor, Futures && ... futures)
{
    using Ret = void;

    auto t = std::make_tuple(std::forward<Futures>(futures)...);
    constexpr auto sz = sizeof...(futures);

    SharedStatePtr<Ret> s;
    if (sz == 0)
    {
        s = makeSharedState<Ret>(executor);
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
    for_each(t, [&s, &ac
#if defined(_MSC_VER) && !defined(__clang__)
        , &sz
#endif
    ](auto &f)
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

/// return a future with an index of the finished future
template <
    class ... Futures,
    typename = std::enable_if_t<
    all< is_future< std::decay_t<Futures> >::value... >::value
    >
>
Future<size_t> whenAny(auto &&executor, Futures && ... futures)
{
    using Ret = size_t;

    auto t = std::make_tuple(std::forward<Futures>(futures)...);
    //constexpr auto sz = sizeof...(futures);

    SharedStatePtr<Ret> s;
    if (sizeof...(futures) == 0)
    {
        s = makeSharedState<Ret>(executor);
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
        f.state->callbacks.push_back([s, i = i++]{ if (s->setExecuted()) s->data = i; });
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
