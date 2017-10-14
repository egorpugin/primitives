#pragma once

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
struct SharedState
{
    Executor &ex;
    std::condition_variable cv;
    std::mutex m;

    std::atomic_bool set{ false };
    std::conditional_t<std::is_same_v<T, void>, int, T> data;
    std::exception_ptr e;

    SharedState(Executor &ex) : ex(ex) {}
    SharedState(const SharedState &rhs)
        : ex(rhs.ex), set(rhs.set), data(rhs.data), e(rhs.e)
    {
    }

    void wait()
    {
        using namespace std::literals::chrono_literals;

        if (set)
            return;

        if (!ex.is_in_executor())
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
            if (!ex.try_run_one())
            {
                std::unique_lock<std::mutex> lk(m);
                cv.wait_for(lk, delay, [this] { return set.load(); });
                delay = delay > max_delay ? max_delay : (delay + 100ms);
            }
            else
                delay = 100ms;
        }
    }
};

template <class T>
using SharedStatePtr = std::shared_ptr<SharedState<T>>;

template <class Ret>
struct Future
{
    Future(Executor &e, const SharedStatePtr<Ret> &s)
        : e(e), s(s)
    {
    }

    Future(const Future &rhs)
        : e(rhs.e), s(rhs.s)
    {
    }
    Future(Future &&rhs) = default;

    std::conditional_t<std::is_same_v<Ret, void>, void, Ret> get() const
    {
        s->wait();
        if (s->e)
            std::rethrow_exception(s->e);

        if constexpr (std::is_same_v<Ret, void>)
            return;
        else
            return std::move(s->data);
    }

    void wait() const
    {
        s->wait();
    }

    template <class F2, class ... ArgTypes2>
    Future<std::invoke_result_t<F2, ArgTypes2...>>
        then(F2 &&f, ArgTypes2 && ... args)
    {
        return e.push(f, std::forward<ArgTypes>(args)...);
    }

private:
    Executor &e;
    SharedStatePtr<Ret> s;
};

template <class F, class ... ArgTypes>
struct PackagedTask
{
    using Ret = std::invoke_result_t<F, ArgTypes...>;
    using Task = std::function<Ret(ArgTypes...)>;
    using FutureType = Future<Ret>;

    PackagedTask(Executor &e, F f, ArgTypes && ... args)
        : e(e), t(f)
    {
        s = std::make_shared<SharedState<Ret>>(e);
    }

    PackagedTask(const PackagedTask &rhs)
        : e(rhs.e), t(rhs.t), s(rhs.s)
    {
    }
    PackagedTask(PackagedTask &&rhs) = default;

    FutureType getFuture()
    {
        FutureType f(e, s);
        return f;
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
            s->e = std::current_exception();
        }
        s->set = true;
        s->cv.notify_all();
    }

private:
    Executor &e;
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

        PackagedTask<F, ArgTypes...> pt2(*this, f, std::forward<ArgTypes>(args)...);
        auto fut = pt2.getFuture();
        Task task2([pt2]() mutable { pt2(); });
        push(std::move(task2));
        return fut;
    }

    void join();
    void stop();
    bool stopped() const { return done; }
    void wait();

    bool is_in_executor() const;
    bool try_run_one();

private:
    Threads thread_pool;
    size_t nThreads = std::thread::hardware_concurrency();
    std::atomic_size_t index{ 0 };
    std::atomic_bool done{ false };
    std::unordered_map<std::thread::id, size_t> thread_ids;
    std::mutex m;

    void run();
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

size_t get_max_threads(size_t N = 4);

template <class Future>
void whenAll(const std::vector<Future> &futures)
{
    for (auto &f : futures)
        f.wait();
}

template <class F1, class ... Rest>
void whenAll(const F1 &f1, const Rest & ... futures)
{
    f1.wait();
    if constexpr (sizeof...(futures) > 0)
        whenAll(futures...);
}
