// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

Executor &getExecutor(size_t N)
{
    static size_t threads = [&N]()
    {
        if (N > 0)
            return N;
        N = std::thread::hardware_concurrency();
        if (N == 1)
            N = 2;
        else if (N <= 8)
            N += 2;
        else if (N <= 64)
            N += 4;
        else
            N += 8;
        return N;
    }();
    static Executor e(threads);
    return e;
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
        thread_pool[i].t = std::thread([this, i, name = name]() mutable
        {
            run(i, name);
        });
    }
}

Executor::~Executor()
{
    join();
}

size_t Executor::get_n() const
{
    return thread_ids.find(std::this_thread::get_id())->second;
}

bool Executor::is_in_executor() const
{
    return thread_ids.find(std::this_thread::get_id()) != thread_ids.end();
}

bool Executor::try_run_one()
{
    auto t = try_pop();
    if (t)
        run_task(t);
    return !!t;
}

void Executor::run(size_t i, const std::string &name)
{
    auto n = name + " " + std::to_string(i);
    set_thread_name(n);
    {
        std::unique_lock<std::mutex> lk(m);
        thread_ids[std::this_thread::get_id()] = i;
    }

    while (!stopped_)
    {
        if (!run_task())
            break;
    }
}

bool Executor::run_task()
{
    return run_task(get_n());
}

bool Executor::run_task(Task &t)
{
    run_task(get_n(), t);
    return true;
}

bool Executor::run_task(size_t i)
{
    auto t = get_task(i);

    // double check
    if (stopped_)
        return false;

    run_task(i, t);
    return true;
}

void Executor::run_task(size_t i, Task &t) noexcept
{
    auto &thr = thread_pool[i];
    t();
    thr.busy = false;
}

Task Executor::get_task()
{
    return get_task(get_n());
}

Task Executor::get_task(size_t i)
{
    auto &thr = thread_pool[i];
    Task t = try_pop(i);
    // no task popped, probably shutdown command was issues
    if (!t && !thr.q.pop(t, thr.busy))
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
    if (thr.q.pop(t, thr.busy))
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
    stopped_ = true;
    for (auto &t : thread_pool)
        t.q.done();
}

void Executor::wait(WaitStatus p)
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

void Executor::push(Task &&t)
{
    if (waiting_ == WaitStatus::RejectIncoming)
        throw std::runtime_error("Executor is in the wait state and rejects new jobs");
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

Task Executor::try_pop()
{
    return try_pop(get_n());
}

Task Executor::try_pop(size_t i)
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
