#pragma once

#include <iostream>
#include <thread>

template <class F>
std::thread make_thread(F &&f)
{
    return std::thread([f = std::move(f)]() mutable
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
