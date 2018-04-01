#include <primitives/cron.h>

#include <primitives/executor.h>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "cron");

Cron::Cron()
{
    t = std::move(std::thread([this] { run(); }));
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

Cron &get_cron(Cron *c)
{
    static Cron *cron;
    if (c)
        cron = c;
    return *cron;
}
