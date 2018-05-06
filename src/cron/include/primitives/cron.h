// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

class PRIMITIVES_CRON_API Cron
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
    Cron();
    ~Cron();

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

    void run();
};

PRIMITIVES_CRON_API
Cron &get_cron(Cron *c = nullptr);
