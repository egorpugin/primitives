#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using Task = std::function<void()>;

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

    bool try_push(Task &t)
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

    void push(Task &t)
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

class Executor
{
    struct ThreadData
    {
        std::thread t;
        TaskQueue q;
        volatile bool busy = false;
        std::exception_ptr eptr;
    };

    using Threads = std::vector<ThreadData>;

public:
    bool throw_exceptions = true;

public:
    Executor(size_t nThreads = std::thread::hardware_concurrency(), const std::string &name = "");
    Executor(const std::string &name, size_t nThreads = std::thread::hardware_concurrency());
    ~Executor();

    size_t numberOfThreads() const { return nThreads; }

    template <class T>
    void push(T &&t)
    {
        static_assert(!std::is_lvalue_reference<T>::value, "Argument cannot be an lvalue");

        Task task([t = std::move(t)]{ t(); });
        auto i = index++;
        for (size_t n = 0; n != nThreads; ++n)
        {
            if (thread_pool[(i + n) % nThreads].q.try_push(task))
                return;
        }
        thread_pool[i % nThreads].q.push(task);
    }

    void stop();
    void wait();

private:
    Threads thread_pool;
    size_t nThreads = std::thread::hardware_concurrency();
    std::atomic_size_t index{ 0 };
    bool done = false;

    void run(size_t i);
    void set_thread_name(const std::string &name, size_t i) const;
};

size_t get_max_threads(size_t N = 4);
