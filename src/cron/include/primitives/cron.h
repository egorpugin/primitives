// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <primitives/templates.h>
#include <primitives/exceptions.h>
#include <primitives/executor.h>
#include <primitives/thread.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER2(cron);

class Cron
{
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    using Task = std::function<void()>;

    struct CronTask
    {
        struct Repeat
        {
            int day;
            int hour;
            int min;
            int second;

            Repeat(int d = 0, int h = 0, int m = 0, int s = 0)
                : day(d), hour(h), min(m), second(s)
            {}

            bool need_repeat()
            {
                return second > 0 || min > 0 || hour > 0 || day > 0;
            }

            TimePoint next_repeat(const TimePoint &from = Clock::now())
            {
                auto tp = from;
                tp += std::chrono::hours(24 * day) +
                    std::chrono::hours(hour) +
                    std::chrono::minutes(min) +
                    std::chrono::seconds(second);
                return tp;
            }
        };

        Task task;
        Repeat repeat;
        int skip;
    };

    using TaskQueue = std::map<TimePoint, CronTask>;
    using Lock = std::unique_lock<std::mutex>;

public:
    Cron(auto &&executor)
    {
        t = make_thread([&] { run(executor); });
    }
    ~Cron()
    {
        stop();
        t.join();
    }

    void push(const TimePoint &p, const Task &t)
    {
        CronTask::Repeat r;
        push(p, t, r);
    }

    void push(const TimePoint &p, const Task &t, const CronTask::Repeat &r, int skip = 1)
    {
        CronTask ct;
        ct.repeat = r;
        ct.task = t;
        ct.skip = skip;

        {
            Lock lock(m);
            tasks.emplace(p, std::move(ct));
        }
        cv.notify_one();
    }

    void stop()
    {
        {
            Lock lock(m);
            done = true;
        }
        cv.notify_all();
    }

private:
    std::thread t;
    std::mutex m;
    std::condition_variable cv;
    TaskQueue tasks;
    bool done = false;

    void run(auto &&executor)
    {
        while (!done)
        {
            std::string error;
            try
            {
                Lock lock(m);
                if (tasks.empty())
                    cv.wait(lock, [this] { return !tasks.empty() || done; });
                if (done)
                    break;
                auto i = tasks.begin();
                auto p = i->first;
                if (p <= Clock::now())
                {
                    auto t = std::move(i->second);
                    tasks.erase(i);

                    bool fire = t.skip <= 0;
                    if (!fire)
                        t.skip--;

                    if (t.repeat.need_repeat())
                    {
                        // repeat not from last fire time, but from now() to prevent multiple instant calls
                        auto p2 = t.repeat.next_repeat();
                        tasks.emplace(p2, t);
                    }
                    if (fire)
                        executor.push([t = std::move(t.task)] { t(); });
                }
                if (!tasks.empty() || !done)
                    cv.wait_until(lock, tasks.begin()->first, [this] { return tasks.begin()->first <= Clock::now() || done; });
            }
            catch (const std::exception &e)
            {
                error = e.what();
            }
            catch (...)
            {
                error = "unknown exception";
            }
            if (!error.empty())
            {
                LOG_ERROR(logger, error);
            }
        }
    }
};
