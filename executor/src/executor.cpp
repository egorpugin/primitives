#include <primitives/executor.h>

#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

//#include "log.h"
//DECLARE_STATIC_LOGGER(logger, "executor");

size_t get_max_threads(size_t N)
{
    return std::max<size_t>(N, std::thread::hardware_concurrency());
}

Executor::Executor(const std::string &name, size_t nThreads)
    : Executor(nThreads, name)
{
}

Executor::Executor(size_t nThreads, const std::string &name)
    : nThreads(nThreads)
{
    thread_pool.resize(nThreads);
    for (size_t i = 0; i < nThreads; i++)
    {
        thread_pool[i].t = std::move(std::thread([this, i, name = name]() mutable
        {
            name += " " + std::to_string(i);
            set_thread_name(name);
            {
                std::unique_lock<std::mutex> lk(m);
                thread_ids[std::this_thread::get_id()] = i;
            }
            run();
        }));
    }
}

Executor::~Executor()
{
    // do not throw from destructor
    throw_exceptions = false;
    join();
}

size_t Executor::get_n() const
{
    return thread_ids.find(std::this_thread::get_id())->second;
}

void Executor::run()
{
    while (!done)
        if (!run_task())
            break;
}

bool Executor::run_task()
{
    return run_task(get_n());
}

bool Executor::run_task(size_t i)
{
    auto t = get_task(i);

    // double check
    if (done)
        return false;

    return run_task(i, t);
}

bool Executor::run_task(size_t i, Task t)
{
    auto &thr = thread_pool[i];

    std::string error;
    try
    {
        thr.busy = true;
        t();
    }
    catch (const std::exception &e)
    {
        error = e.what();
        thr.eptr = std::current_exception();
    }
    catch (...)
    {
        error = "unknown exception";
        thr.eptr = std::current_exception();
    }
    thr.busy = false;

    if (!error.empty())
    {
        if (throw_exceptions)
            done = true;
        else
        {
            // clear
            thr.eptr = std::current_exception();
            if (!silent)
                std::cerr << "executor: " << this << ", thread #" << i + 1 << ", error: " << error << "\n";
            //LOG_ERROR(logger, "executor: " << this << ", thread #" << i + 1 << ", error: " << error);
        }
    }

    return true;
}

Task Executor::get_task()
{
    return get_task(get_n());
}

Task Executor::get_task(size_t i)
{
    auto &thr = thread_pool[i];

    Task t;
    const size_t spin_count = nThreads * 4;
    for (auto n = 0; n != spin_count; ++n)
    {
        if (thread_pool[(i + n) % nThreads].q.try_pop(t))
            break;
    }

    // no task popped, probably shutdown command was issues
    if (!t && !thr.q.pop(t))
        return Task();

    return t;
}

Task Executor::get_task_non_stealing()
{
    return get_task_non_stealing(get_n());
}

Task Executor::get_task_non_stealing(size_t i)
{
    auto &thr = thread_pool[i];
    Task t;
    if (thr.q.pop(t))
        return t;
    return Task();
}

void Executor::join()
{
    wait();
    stop();
    for (auto &t : thread_pool)
    {
        if (t.t.joinable())
            t.t.join();
    }
}

void Executor::stop()
{
    done = true;
    for (auto &t : thread_pool)
        t.q.done();
}

void Executor::wait()
{
    bool run_again = thread_ids.find(std::this_thread::get_id()) != thread_ids.end();

    // wait for empty queues
    for (auto &t : thread_pool)
    {
        while (!t.q.empty() && !done)
        {
            if (run_again)
            {
                // finish everything
                for (auto &t : thread_pool)
                {
                    Task ta;
                    if (t.q.try_pop(ta))
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
        while (t.busy && !done)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (throw_exceptions)
    {
        for (auto &t : thread_pool)
        {
            if (t.eptr)
            {
                decltype(t.eptr) e;
                std::swap(t.eptr, e);
                std::rethrow_exception(e);
            }
        }
    }
}

void Executor::set_thread_name(const std::string &name) const
{
    if (name.empty())
        return;

#if defined(_MSC_VER)
#pragma pack(push, 8)
    struct THREADNAME_INFO
    {
        DWORD dwType = 0x1000;  // Must be 0x1000.
        LPCSTR szName;          // Pointer to thread name
        DWORD dwThreadId = -1;  // Thread ID (-1 == current thread)
        DWORD dwFlags = 0;      // Reserved.  Do not use.
    };
#pragma pack(pop)

    THREADNAME_INFO info;
    info.szName = name.c_str();

    __try
    {
        ::RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#endif
}
