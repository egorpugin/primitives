#pragma once

#include <iostream>
#include <thread>

template <class F, class Thread = std::thread>
auto make_thread(F &&f)
{
    return Thread([f = std::move(f)]() mutable
    {
        auto id = std::this_thread::get_id();
        try
        {
            f();
        }
        catch (std::exception &e)
        {
            std::cerr << "unrecoverable error in thread #" << id << ": " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "unknown unrecoverable error in thread #" << id << ": " << std::endl;
        }
    });
}

struct joining_thread : std::thread
{
    using std::thread::thread;

    ~joining_thread()
    {
        join();
    }
};

template <class F>
auto make_joining_thread(F &&f)
{
    return make_thread<F, joining_thread>(std::forward<F>(f));
}
