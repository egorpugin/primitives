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
            set_thread_name(name, i);
            run(i);
        }));
    }
}

Executor::~Executor()
{
    stop();
    for (auto &t : thread_pool)
        t.t.join();
}

void Executor::run(size_t i)
{
    while (!done)
    {
        std::string error;
        try
        {
            Task t;
            const size_t spin_count = nThreads * 4;
            for (auto n = 0; n != spin_count; ++n)
            {
                if (thread_pool[(i + n) % nThreads].q.try_pop(t))
                    break;
            }

            // no task popped, probably shutdown command was issues
            if (!t && !thread_pool[i].q.pop(t))
                break;

            thread_pool[i].busy = true;
            t();
            thread_pool[i].busy = false;
        }
        catch (const std::exception &e)
        {
            error = e.what();
            thread_pool[i].eptr = std::current_exception();
        }
        catch (...)
        {
            error = "unknown exception";
            thread_pool[i].eptr = std::current_exception();
        }
        if (!error.empty())
        {
            if (throw_exceptions)
                done = true;
            else
                std::cerr << "executor: " << this << ", thread #" << i + 1 << ", error: " << error;
                //LOG_ERROR(logger, "executor: " << this << ", thread #" << i + 1 << ", error: " << error);
        }
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
    // wait for empty queues
    for (auto &t : thread_pool)
        while (!t.q.empty() && !done)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // wait for end of execution
    for (auto &t : thread_pool)
        while (t.busy && !done)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (auto &t : thread_pool)
        if (t.eptr)
            std::rethrow_exception(t.eptr);
}

void Executor::set_thread_name(const std::string &name, size_t i) const
{
    if (name.empty())
        return;

#if defined(_MSC_VER)
    constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
    struct THREADNAME_INFO
    {
        DWORD dwType;     // Must be 0x1000.
        LPCSTR szName;    // Pointer to thread name
        DWORD dwThreadId; // Thread ID (-1 == current thread)
        DWORD dwFlags;    // Reserved.  Do not use.
    };
#pragma pack(pop)

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name.c_str();
    info.dwThreadId = *(DWORD *)&std::this_thread::get_id();
    info.dwFlags = 0;

    __try
    {
        ::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
#endif
}