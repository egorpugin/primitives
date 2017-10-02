#include <primitives/executor.h>

#include <algorithm>
#include <iostream>

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
    work = std::make_unique<boost::asio::io_service::work>(io_service);
    thread_pool.resize(nThreads);
    for (size_t i = 0; i < nThreads; i++)
    {
        thread_pool.emplace_back([this, i, name = name]() mutable
        {
            name += " " + std::to_string(i);
            set_thread_name(name, i);

            while (1)
            {
                run(i);
                if (wait_)
                {
                    std::unique_lock<std::mutex> lk(m);
                    cv.wait(lk, [this]() { return !wait_; });
                    continue;
                }
                break;
            }
        });
    }
}

Executor::~Executor()
{
    join();
}

void Executor::run(size_t i)
{
    while (!io_service.stopped())
    {
        std::string error;
        try
        {
            io_service.run();
        }
        catch (const std::exception &e)
        {
            if (!io_service.stopped())
            {
                error = e.what();
                eptr = std::current_exception();
            }
        }
        catch (...)
        {
            if (!io_service.stopped())
            {
                error = "unknown exception";
                eptr = std::current_exception();
            }
        }

        if (!error.empty())
        {
            if (throw_exceptions)
                io_service.stop();
            else
            {
                // clear
                eptr = std::current_exception();
                if (!silent)
                    std::cerr << "executor: " << this << ", thread #" << i + 1 << ", error: " << error << "\n";
                //LOG_ERROR(logger, "executor: " << this << ", thread #" << i + 1 << ", error: " << error);
            }
        }
    }
}

void Executor::join()
{
    stop();
    for (auto &t : thread_pool)
    {
        if (t.joinable())
            t.join();
    }
}

void Executor::stop()
{
    wait_ = false;
    work.reset();
    io_service.stop();
    wait_stop();
    try_throw();
}

void Executor::wait()
{
    if (io_service.stopped())
    {
        try_throw();
        return;
    }

    wait_ = true;
    work.reset();

    wait_stop();
    try_throw();

    wait_ = false;
    io_service.reset();
    work = std::make_unique<boost::asio::io_service::work>(io_service);
    cv.notify_all();
}

void Executor::wait_stop()
{
    while (!io_service.stopped())
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void Executor::try_throw()
{
    if (!throw_exceptions || !eptr)
        return;

    wait_ = false;
    cv.notify_all();

    decltype(eptr) e;
    std::swap(eptr, e);
    std::rethrow_exception(e);
}

void Executor::set_thread_name(const std::string &name, size_t i) const
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
