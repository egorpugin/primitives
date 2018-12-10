// Copyright (C) 2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <primitives/cron.h>

#include <primitives/exceptions.h>
#include <primitives/executor.h>
#include <primitives/thread.h>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "cron");

Cron::Cron()
{
    t = make_thread([this] { run(); });
}

Cron::~Cron()
{
    stop();
    t.join();
}

void Cron::run()
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
                    getExecutor().push([t = std::move(t.task)] { t(); });
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

SW_DEFINE_GLOBAL_STATIC_FUNCTION(Cron, get_cron)
